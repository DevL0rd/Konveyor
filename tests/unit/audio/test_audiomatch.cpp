#include "audio/audiomatch.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QTest>

#include <signal.h>
#include <unistd.h>

using namespace Konveyor::Audio;

namespace
{

QString ownBinary()
{
    return QFileInfo(QCoreApplication::applicationFilePath()).fileName();
}

WindowKey windowNamed(const QString &desktopFile)
{
    return {0, desktopFile};
}

}

class TestAudioMatch : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aProcessDescendsFromItself();
    void aChildAndGrandchildDescendFromTheirAncestors();
    void unrelatedAndMissingProcessesDoNot();
    void theWalkStopsAtAnAncestorCycle();
    void theWalkStopsAtTheDeepestProcessTree();
    void theWalkEndsAtTheInitProcess();
    void aStreamWithoutAUsableProcessFallsBackToNames();
    void theProcessTreeIsPreferredOverNames();
    void portalAppIdMatchesIgnoringCaseAndSuffix();
    void binaryMatchesTheApplicationOrItsLastNameSection();
    void aHostProcessBinaryNeverMatchesByName();
    void emptyNamesMatchNothing();
    void aStreamFallsBackToItsClientForWhatItLacks();
    void theParentComesFromTheFieldAfterTheState_data();
    void theParentComesFromTheFieldAfterTheState();
};

void TestAudioMatch::aProcessDescendsFromItself()
{
    QVERIFY(processDescendsFrom(getpid(), getpid()));
}

void TestAudioMatch::aChildAndGrandchildDescendFromTheirAncestors()
{
    QProcess shell;
    shell.start(QStringLiteral("sh"), {QStringLiteral("-c"), QStringLiteral("sleep 30 & echo $!; wait")});
    QVERIFY(shell.waitForStarted());
    QVERIFY(shell.waitForReadyRead(10000));
    const qint64 grandchild = shell.readLine().trimmed().toLongLong();
    QVERIFY(grandchild > 1);

    QVERIFY(processDescendsFrom(shell.processId(), getpid()));
    QVERIFY(processDescendsFrom(grandchild, shell.processId()));
    QVERIFY(processDescendsFrom(grandchild, getpid()));
    QVERIFY(!processDescendsFrom(shell.processId(), grandchild));

    kill(static_cast<pid_t>(grandchild), SIGTERM);
    shell.terminate();
    QVERIFY(shell.waitForFinished(10000));
}

void TestAudioMatch::unrelatedAndMissingProcessesDoNot()
{
    QVERIFY(!processDescendsFrom(getpid(), 1));
    QVERIFY(!processDescendsFrom(getppid(), getpid()));
    QVERIFY(!processDescendsFrom(0, getpid()));
    QVERIFY(!processDescendsFrom(-5, getpid()));
    QVERIFY(!processDescendsFrom(0x7ffffff0, getpid()));
}

void TestAudioMatch::theWalkStopsAtAnAncestorCycle()
{
    const ParentLookup cycle = [](qint64 pid) { return pid == 10 ? 20 : 10; };
    QVERIFY(processDescendsFrom(10, 20, cycle));
    QVERIFY(!processDescendsFrom(10, 30, cycle));
}

void TestAudioMatch::theWalkStopsAtTheDeepestProcessTree()
{
    const ParentLookup chain = [](qint64 pid) { return pid + 1; };
    QVERIFY(processDescendsFrom(100, 150, chain));
    QVERIFY(!processDescendsFrom(100, 400, chain));
}

void TestAudioMatch::theWalkEndsAtTheInitProcess()
{
    const ParentLookup toInit = [](qint64 pid) { return pid == 5 ? 1 : 5; };
    QVERIFY(processDescendsFrom(9, 5, toInit));
    QVERIFY(!processDescendsFrom(9, 1, toInit));
}

void TestAudioMatch::aStreamWithoutAUsableProcessFallsBackToNames()
{
    const WindowKey namesake {getpid(), QStringLiteral("player.desktop")};
    const WindowKey stranger {2, QStringLiteral("player.desktop")};
    const StreamIdentity noProcess {0, QStringLiteral("player"), {}};
    QCOMPARE(windowsPlaying(noProcess, {namesake}), (QList<WindowKey> {namesake}));
    const StreamIdentity unrelated {getpid(), QStringLiteral("player"), {}};
    QCOMPARE(windowsPlaying(unrelated, {stranger}), (QList<WindowKey> {stranger}));
}

void TestAudioMatch::theProcessTreeIsPreferredOverNames()
{
    const WindowKey owner {getpid(), QStringLiteral("other.desktop")};
    const WindowKey namesake {0, QStringLiteral("player.desktop")};
    const StreamIdentity stream {getpid(), QStringLiteral("player"), QStringLiteral("player")};
    QCOMPARE(windowsPlaying(stream, {namesake, owner}), (QList<WindowKey> {owner}));
}

void TestAudioMatch::portalAppIdMatchesIgnoringCaseAndSuffix()
{
    const StreamIdentity stream {0, {}, QStringLiteral("org.Example.Player")};
    const WindowKey match {0, QStringLiteral("ORG.example.player.desktop")};
    const WindowKey bare {0, QStringLiteral("org.example.player")};
    const WindowKey other {0, QStringLiteral("org.example.other.desktop")};
    QCOMPARE(windowsPlaying(stream, {other, match, bare}), (QList<WindowKey> {match, bare}));
}

void TestAudioMatch::binaryMatchesTheApplicationOrItsLastNameSection()
{
    const StreamIdentity stream {0, QStringLiteral("Firefox"), {}};
    const WindowKey plain {0, QStringLiteral("firefox.desktop")};
    const WindowKey dotted {0, QStringLiteral("org.mozilla.firefox.desktop")};
    const WindowKey other {0, QStringLiteral("org.mozilla.thunderbird.desktop")};
    QCOMPARE(windowsPlaying(stream, {plain, dotted, other}), (QList<WindowKey> {plain, dotted}));
}

void TestAudioMatch::aHostProcessBinaryNeverMatchesByName()
{
    const StreamIdentity stream {getpid(), ownBinary(), {}};
    QVERIFY(windowsPlaying(stream, {windowNamed(ownBinary() + QStringLiteral(".desktop"))}).isEmpty());

    const StreamIdentity portal {getpid(), ownBinary(), ownBinary()};
    QCOMPARE(windowsPlaying(portal, {windowNamed(ownBinary())}).size(), 1);

    const StreamIdentity detached {1, ownBinary(), {}};
    QCOMPARE(windowsPlaying(detached, {windowNamed(ownBinary())}).size(), 1);
}

void TestAudioMatch::emptyNamesMatchNothing()
{
    const StreamIdentity stream {0, QStringLiteral("player"), QStringLiteral("player")};
    QVERIFY(windowsPlaying(stream, {windowNamed({})}).isEmpty());
    QVERIFY(windowsPlaying(stream, {windowNamed(QStringLiteral(".desktop"))}).isEmpty());
    QVERIFY(windowsPlaying({0, {}, {}}, {windowNamed(QStringLiteral("player.desktop"))}).isEmpty());
    QVERIFY(windowsPlaying(stream, {}).isEmpty());
}

void TestAudioMatch::aStreamFallsBackToItsClientForWhatItLacks()
{
    const StreamIdentity client {42, QStringLiteral("client-binary"), QStringLiteral("client.app")};
    const StreamIdentity bare = withFallback({}, client);
    QCOMPARE(bare.pid, 42);
    QCOMPARE(bare.binary, client.binary);
    QCOMPARE(bare.portalAppId, client.portalAppId);

    const StreamIdentity own {7, QStringLiteral("own-binary"), QStringLiteral("own.app")};
    const StreamIdentity kept = withFallback(own, client);
    QCOMPARE(kept.pid, 7);
    QCOMPARE(kept.binary, own.binary);
    QCOMPARE(kept.portalAppId, own.portalAppId);

    const StreamIdentity partial = withFallback({-1, QStringLiteral("own-binary"), {}}, client);
    QCOMPARE(partial.pid, 42);
    QCOMPARE(partial.binary, QStringLiteral("own-binary"));
    QCOMPARE(partial.portalAppId, client.portalAppId);
}

void TestAudioMatch::theParentComesFromTheFieldAfterTheState_data()
{
    QTest::addColumn<QByteArray>("stat");
    QTest::addColumn<qint64>("parent");
    QTest::newRow("plain") << QByteArray("42 (sleep) S 7 42 42 0 -1") << qint64(7);
    QTest::newRow("a name with parentheses and spaces") << QByteArray("42 (a) b (c d)) R 9 42") << qint64(9);
    QTest::newRow("no name") << QByteArray("42 sleep S 7") << qint64(0);
    QTest::newRow("cut after the name") << QByteArray("42 (sleep)") << qint64(0);
    QTest::newRow("cut after the state") << QByteArray("42 (sleep) S") << qint64(0);
    QTest::newRow("empty") << QByteArray() << qint64(0);
}

void TestAudioMatch::theParentComesFromTheFieldAfterTheState()
{
    QFETCH(QByteArray, stat);
    QFETCH(qint64, parent);
    QCOMPARE(parentPidFromStat(stat), parent);
}

QTEST_GUILESS_MAIN(TestAudioMatch)
#include "test_audiomatch.moc"
