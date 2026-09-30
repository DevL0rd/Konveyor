#include "store/configfile.h"
#include "store/edithistory.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace Konveyor::Settings;
using namespace std::chrono_literals;

namespace
{

constexpr std::chrono::milliseconds NoBurst = 0ms;
constexpr std::chrono::milliseconds LongBurst = std::chrono::hours(24);

}

class TestSettingsHistory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void undoReturnsSnapshotsNewestFirst();
    void burstKeepsFirstSnapshot();
    void skipsRepeatedSnapshot();
    void keepsAtMostHundred();
    void clearForgetsEverything();
    void undoEndsBurst();
    void fileRoundTripKeepsBytes();
    void missingFileReadsNothing();
    void writeCreatesFolders();
    void writeFailureIsReported();
    void writeKeepsASymlinkedConfigAndItsPermissions();
};

void TestSettingsHistory::undoReturnsSnapshotsNewestFirst()
{
    EditHistory history(NoBurst);
    QVERIFY(!history.canUndo());
    QVERIFY(!history.undo().has_value());
    history.record(QStringLiteral("a"));
    history.record(QStringLiteral("b"));
    QVERIFY(history.canUndo());
    QCOMPARE(history.undo().value(), QStringLiteral("b"));
    QCOMPARE(history.undo().value(), QStringLiteral("a"));
    QVERIFY(!history.canUndo());
}

void TestSettingsHistory::burstKeepsFirstSnapshot()
{
    EditHistory history(LongBurst);
    history.record(QStringLiteral("a"));
    history.record(QStringLiteral("b"));
    history.record(QStringLiteral("c"));
    QCOMPARE(history.undo().value(), QStringLiteral("a"));
    QVERIFY(!history.canUndo());
}

void TestSettingsHistory::skipsRepeatedSnapshot()
{
    EditHistory history(NoBurst);
    history.record(QStringLiteral("a"));
    history.record(QStringLiteral("a"));
    QCOMPARE(history.undo().value(), QStringLiteral("a"));
    QVERIFY(!history.canUndo());
}

void TestSettingsHistory::keepsAtMostHundred()
{
    EditHistory history(NoBurst);
    for (int index = 0; index < 105; ++index) {
        history.record(QString::number(index));
    }
    QStringList undone;
    while (const std::optional<QString> snapshot = history.undo()) {
        undone.append(*snapshot);
    }
    QCOMPARE(undone.size(), 100);
    QCOMPARE(undone.first(), QStringLiteral("104"));
    QCOMPARE(undone.last(), QStringLiteral("5"));
}

void TestSettingsHistory::clearForgetsEverything()
{
    EditHistory history(LongBurst);
    history.record(QStringLiteral("a"));
    history.clear();
    QVERIFY(!history.canUndo());
    history.record(QStringLiteral("b"));
    QCOMPARE(history.undo().value(), QStringLiteral("b"));
}

void TestSettingsHistory::undoEndsBurst()
{
    EditHistory history(LongBurst);
    history.record(QStringLiteral("a"));
    history.record(QStringLiteral("b"));
    QCOMPARE(history.undo().value(), QStringLiteral("a"));
    history.record(QStringLiteral("c"));
    QCOMPARE(history.undo().value(), QStringLiteral("c"));
}

void TestSettingsHistory::fileRoundTripKeepsBytes()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("config.kdl"));
    const QString text = QStringLiteral("layout {\r\n    gaps 4 // ünï 🎮\r\n}\r\n");
    QVERIFY(writeConfigText(path, text).has_value());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), text.toUtf8());
    QCOMPARE(readConfigText(path).value(), text);
}

void TestSettingsHistory::missingFileReadsNothing()
{
    QTemporaryDir dir;
    QVERIFY(!readConfigText(dir.filePath(QStringLiteral("missing.kdl"))).has_value());
}

void TestSettingsHistory::writeCreatesFolders()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("a/b/config.kdl"));
    QVERIFY(writeConfigText(path, QStringLiteral("x\n")).has_value());
    QCOMPARE(readConfigText(path).value(), QStringLiteral("x\n"));
}

void TestSettingsHistory::writeFailureIsReported()
{
    QTemporaryDir dir;
    const QString blocker = dir.filePath(QStringLiteral("file"));
    QVERIFY(writeConfigText(blocker, QString()).has_value());
    const auto written = writeConfigText(blocker + QStringLiteral("/config.kdl"), QStringLiteral("x\n"));
    QVERIFY(!written.has_value());
    QVERIFY2(written.error().startsWith(QStringLiteral("Could not write ") + blocker), qPrintable(written.error()));
}

void TestSettingsHistory::writeKeepsASymlinkedConfigAndItsPermissions()
{
    QTemporaryDir dir;
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("dotfiles")));
    const QString target = dir.filePath(QStringLiteral("dotfiles/config.kdl"));
    const QString link = dir.filePath(QStringLiteral("config.kdl"));
    QVERIFY(writeConfigText(target, QStringLiteral("a\n")).has_value());
    const QFileDevice::Permissions permissions = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    QVERIFY(QFile::setPermissions(target, permissions));
    QVERIFY(QFile::link(target, link));
    QVERIFY(writeConfigText(link, QStringLiteral("b\n")).has_value());
    QVERIFY(QFileInfo(link).isSymLink());
    QCOMPARE(readConfigText(target).value(), QStringLiteral("b\n"));
    QCOMPARE(QFile::permissions(target) & ~(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser), permissions);
}

QTEST_GUILESS_MAIN(TestSettingsHistory)
#include "test_settings_history.moc"
