#include "audio/windowaudio.h"
#include "pwharness.h"

#include <QTest>

#include <unistd.h>

using namespace Konveyor::Audio;
using namespace PwHarness;

namespace
{

constexpr int kSettleMs = 20000;
constexpr float kAudible = 0.05F;

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
    void reconnectsWhenTheDaemonStartsLate();
    void reconnectsAfterTheDaemonRestarts();
    void theBeatThresholdAppliesWhileCapturing();

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

QTEST_GUILESS_MAIN(TestAudioCapture)
#include "test_audiocapture.moc"
