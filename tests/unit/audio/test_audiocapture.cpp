#include "audio/audiocapture.h"
#include "audio/windowaudio.h"
#include "pwharness.h"
#include "render/audioanalyzer.h"

#include <QElapsedTimer>
#include <QTest>

#include <unistd.h>

#include <memory>

using namespace Konveyor::Audio;
using namespace PwHarness;

namespace
{

constexpr int kSettleMs = 20000;
constexpr float kAudible = 0.05F;
constexpr qint64 kStuckDaemonBudgetMs = 2000;

class EnvironmentOverride
{
public:
    EnvironmentOverride(const char *name, const char *value)
        : m_name(name)
        , m_previous(qgetenv(name))
        , m_wasSet(qEnvironmentVariableIsSet(name))
    {
        qputenv(name, value);
    }

    ~EnvironmentOverride()
    {
        if (m_wasSet) {
            qputenv(m_name, m_previous);
        } else {
            qunsetenv(m_name);
        }
    }

    EnvironmentOverride(const EnvironmentOverride &) = delete;
    EnvironmentOverride &operator=(const EnvironmentOverride &) = delete;

private:
    const char *m_name;
    QByteArray m_previous;
    bool m_wasSet;
};

WindowKey processWindow()
{
    return {getpid(), {}};
}

WindowKey portalWindow()
{
    return {0, QStringLiteral("org.test.player.desktop")};
}

WindowKey binaryWindow()
{
    return {0, QStringLiteral("fakeplayer.desktop")};
}

WindowKey strangerWindow()
{
    return {0, QStringLiteral("stranger.desktop")};
}

qint64 slowestCall(WindowAudio &audio, const QList<QList<WindowKey>> &wantedInTurn, int rounds)
{
    qint64 slowest = 0;
    QElapsedTimer timer;
    for (int round = 0; round < rounds; ++round) {
        timer.start();
        audio.setWanted(wantedInTurn.at(round % wantedInTurn.size()));
        audio.setBeatThreshold(1.4F);
        audio.frame(processWindow());
        slowest = std::max(slowest, timer.elapsed());
        QTest::qWait(2);
    }
    return slowest;
}

}

class TestAudioCapture : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void capturesTheWindowThatOwnsTheStream();
    void matchesStreamsByPortalAppIdAndBinary();
    void aStoppedStreamFadesToSilence();
    void followsTheWantedWindowsAsTheyChange();
    void streamsNobodyWantsAreNotCaptured();
    void aCaptureEndsWhenItsWindowStopsPlaying();
    void streamsThatVanishWhileBeingWatchedAreSurvived();
    void wantedWindowsStaySilentWithoutADaemon();
    void wantedWindowsStaySilentWhenPipeWireHasNoConfiguration();
    void reconnectsWhenTheDaemonStartsLate();
    void reconnectsAfterTheDaemonRestarts();
    void theBeatThresholdAppliesWhileCapturing();
    void aCaptureNobodyAsksForStaysIdleUntilStopped();
    void aCaptureStoppedAtOnceNeverRunsItsLoop();
    void aDaemonThatFreezesWhileCapturingNeverHoldsUpTheCaller();
    void aDaemonFrozenFromTheStartNeverHoldsUpTheCaller();

private:
    bool heard(const WindowAudio &audio, const WindowKey &key, bool relink = true);
    bool startDaemon();

    QTemporaryDir m_directory;
    Daemon m_daemon;
    SessionStub m_session;
    Player m_player;
    bool m_linked = false;
};

void TestAudioCapture::initTestCase()
{
    const QString problem = isolate(m_directory);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

bool TestAudioCapture::startDaemon()
{
    m_linked = false;
    return m_daemon.start(m_directory.path()) && m_session.start();
}

void TestAudioCapture::init()
{
    m_linked = false;
}

void TestAudioCapture::cleanup()
{
    m_player.stop();
    m_session.stop();
    m_daemon.stop();
}

bool TestAudioCapture::heard(const WindowAudio &audio, const WindowKey &key, bool relink)
{
    if (relink && !m_linked) {
        m_linked = linkPlayerToCapture();
    }
    return audio.frame(key).level > kAudible;
}

void TestAudioCapture::capturesTheWindowThatOwnsTheStream()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
    const Konveyor::Render::AudioFrame frame = audio.frame(processWindow());
    QVERIFY(frame.mid > frame.bass);
    QVERIFY(frame.mid > frame.treble);
}

void TestAudioCapture::matchesStreamsByPortalAppIdAndBinary()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({portalWindow(), binaryWindow(), strangerWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, portalWindow()), kSettleMs);
    QTRY_VERIFY_WITH_TIMEOUT(audio.frame(binaryWindow()).level > kAudible, kSettleMs);
    QCOMPARE(audio.frame(strangerWindow()).level, 0.0F);
}

void TestAudioCapture::aStoppedStreamFadesToSilence()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
    m_player.stop();
    QTRY_COMPARE_WITH_TIMEOUT(audio.frame(processWindow()).level, 0.0F, kSettleMs);
}

void TestAudioCapture::followsTheWantedWindowsAsTheyChange()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({strangerWindow()});
    audio.setWanted({processWindow(), strangerWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
    QCOMPARE(audio.frame(strangerWindow()).level, 0.0F);

    audio.setWanted({strangerWindow()});
    QCOMPARE(audio.frame(processWindow()).level, 0.0F);
    audio.setWanted({});
    audio.setWanted({processWindow()});
    audio.setWanted({processWindow()});
    m_linked = false;
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
    audio.setWanted({});
    audio.setWanted({});
}

void TestAudioCapture::streamsNobodyWantsAreNotCaptured()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({strangerWindow()});
    QTest::qWait(1500);
    QVERIFY(portStartingWith(QStringLiteral("-i"), QLatin1String(kCaptureNode) + QLatin1Char(':')).isEmpty());
    QCOMPARE(audio.frame(strangerWindow()).level, 0.0F);

    audio.setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
    QCOMPARE(audio.frame(strangerWindow()).level, 0.0F);
}

void TestAudioCapture::aCaptureEndsWhenItsWindowStopsPlaying()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
    QVERIFY(!portStartingWith(QStringLiteral("-i"), QLatin1String(kCaptureNode) + QLatin1Char(':')).isEmpty());

    audio.setWanted({strangerWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(portStartingWith(QStringLiteral("-i"), QLatin1String(kCaptureNode) + QLatin1Char(':')).isEmpty(), kSettleMs);
    QCOMPARE(audio.frame(processWindow()).level, 0.0F);
}

void TestAudioCapture::streamsThatVanishWhileBeingWatchedAreSurvived()
{
    QVERIFY(startDaemon());
    WindowAudio audio;
    audio.setWanted({processWindow()});
    for (int round = 0; round < 15; ++round) {
        QVERIFY(m_player.start(Player::Mode::Tone));
        m_player.stop();
    }
    QVERIFY(m_player.start(Player::Mode::Tone));
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
}

void TestAudioCapture::wantedWindowsStaySilentWithoutADaemon()
{
    {
        WindowAudio audio;
        audio.setWanted({processWindow(), portalWindow()});
        QTest::qWait(300);
        QCOMPARE(audio.frame(processWindow()).level, 0.0F);
        QCOMPARE(audio.frame(portalWindow()).level, 0.0F);
        audio.setWanted({});
    }
    WindowAudio again;
    again.setBeatThreshold(2.0F);
    again.setWanted({binaryWindow()});
    QTest::qWait(100);
    QCOMPARE(again.frame(binaryWindow()).beat, 0.0F);
}

void TestAudioCapture::wantedWindowsStaySilentWhenPipeWireHasNoConfiguration()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    const EnvironmentOverride configuration("PIPEWIRE_CONFIG_DIR", "/nonexistent/konveyor-pipewire");
    WindowAudio audio;
    audio.setWanted({processWindow()});
    QTest::qWait(1500);
    QCOMPARE(audio.frame(processWindow()).level, 0.0F);
    QVERIFY(portStartingWith(QStringLiteral("-i"), QLatin1String(kCaptureNode) + QLatin1Char(':')).isEmpty());
}

void TestAudioCapture::reconnectsWhenTheDaemonStartsLate()
{
    WindowAudio audio;
    audio.setWanted({processWindow()});
    QTest::qWait(300);
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
}

void TestAudioCapture::reconnectsAfterTheDaemonRestarts()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    WindowAudio audio;
    audio.setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);

    m_daemon.stop();
    m_player.stop();
    m_session.stop();
    QTRY_COMPARE_WITH_TIMEOUT(audio.frame(processWindow()).level, 0.0F, kSettleMs);

    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);
}

void TestAudioCapture::theBeatThresholdAppliesWhileCapturing()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Clicks));
    WindowAudio audio;
    audio.setBeatThreshold(1000.0F);
    audio.setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(audio, processWindow()), kSettleMs);

    const auto countBeats = [&audio](int milliseconds) {
        int beats = 0;
        bool armed = true;
        for (int waited = 0; waited < milliseconds; waited += 10) {
            QTest::qWait(10);
            const float beat = audio.frame(processWindow()).beat;
            if (beat > 0.8F && armed) {
                ++beats;
                armed = false;
            } else if (beat < 0.3F) {
                armed = true;
            }
        }
        return beats;
    };
    QVERIFY(countBeats(4000) <= 1);
    audio.setBeatThreshold(1.4F);
    QTest::qWait(500);
    QVERIFY(countBeats(4000) >= 4);
}

void TestAudioCapture::aCaptureNobodyAsksForStaysIdleUntilStopped()
{
    QVERIFY(startDaemon());
    AudioCapture capture(Konveyor::Render::AudioAnalyzer::kDefaultBeatThreshold);
    const QString binary = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    QTRY_VERIFY_WITH_TIMEOUT(clientConnected(binary), kSettleMs);
    QVERIFY(!capture.hasFinished());
    QVERIFY(portStartingWith(QStringLiteral("-i"), QLatin1String(kCaptureNode) + QLatin1Char(':')).isEmpty());
    capture.requestStop();
    QTRY_VERIFY_WITH_TIMEOUT(capture.hasFinished(), kSettleMs);
}

void TestAudioCapture::aCaptureStoppedAtOnceNeverRunsItsLoop()
{
    AudioCapture capture(Konveyor::Render::AudioAnalyzer::kDefaultBeatThreshold);
    capture.requestStop();
    QTRY_VERIFY_WITH_TIMEOUT(capture.hasFinished(), kSettleMs);
}

void TestAudioCapture::aDaemonThatFreezesWhileCapturingNeverHoldsUpTheCaller()
{
    QVERIFY(startDaemon());
    QVERIFY(m_player.start(Player::Mode::Tone));
    auto audio = std::make_unique<WindowAudio>();
    audio->setWanted({processWindow()});
    QTRY_VERIFY_WITH_TIMEOUT(heard(*audio, processWindow()), kSettleMs);

    m_daemon.setFrozen(true);
    QTest::qWait(200);
    QVERIFY(slowestCall(*audio, {{strangerWindow()}, {processWindow()}}, 500) < kStuckDaemonBudgetMs);
    QElapsedTimer timer;
    timer.start();
    audio->setWanted({});
    audio->setWanted({processWindow()});
    audio.reset();
    QVERIFY(timer.elapsed() < kStuckDaemonBudgetMs);
}

void TestAudioCapture::aDaemonFrozenFromTheStartNeverHoldsUpTheCaller()
{
    QVERIFY(startDaemon());
    m_daemon.setFrozen(true);
    auto audio = std::make_unique<WindowAudio>();
    QVERIFY(slowestCall(*audio, {{processWindow()}, {}}, 100) < kStuckDaemonBudgetMs);
    audio->setWanted({processWindow()});
    QTest::qWait(300);
    QElapsedTimer timer;
    timer.start();
    audio.reset();
    QVERIFY(timer.elapsed() < kStuckDaemonBudgetMs);
}

QTEST_GUILESS_MAIN(TestAudioCapture)
#include "test_audiocapture.moc"
