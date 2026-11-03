#include "audio/audiosignals.h"
#include "pwharness.h"

#include <QTest>

#include <unistd.h>

using namespace Konveyor::Audio;
using namespace Konveyor::Render;
using namespace PwHarness;

namespace
{

constexpr quint64 kPlaying = 7;
constexpr quint64 kStranger = 8;
constexpr int kSettleMs = 20000;
constexpr float kAudible = 0.05F;

SignalSet audioWanted()
{
    SignalSet set;
    set.set(static_cast<std::size_t>(Signal::AudioLevel));
    return set;
}

}

class TestAudioSignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void wantedWindowsGetTheirOwnAudio();
    void nothingIsCapturedWithoutALookupOrAWantedSignal();
    void fullscreenAndUnknownWindowsAreNotCaptured();
    void reducedMotionAndBatteryStopTheCapture();
    void windowsThatNoLongerWantAudioAreCleared();

private:
    void startPlayback();
    bool heard(quint64 window);
    void wantPlayingWindow(AudioSignals &captured);

    QTemporaryDir m_directory;
    Daemon m_daemon;
    SessionStub m_session;
    Player m_player;
    SignalBus m_bus;
    bool m_linked = false;
};

void TestAudioSignals::initTestCase()
{
    const QString problem = isolate(m_directory);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void TestAudioSignals::init()
{
    m_linked = false;
    m_bus = SignalBus();
    QVERIFY(m_daemon.start(m_directory.path()));
    QVERIFY(m_session.start());
    QVERIFY(m_player.start(Player::Mode::Tone));
}

void TestAudioSignals::cleanup()
{
    m_player.stop();
    m_session.stop();
    m_daemon.stop();
}

bool TestAudioSignals::heard(quint64 window)
{
    if (!m_linked) {
        m_linked = linkPlayerToCapture();
    }
    return m_bus.frame(window).value(Signal::AudioLevel) > kAudible;
}

void TestAudioSignals::wantPlayingWindow(AudioSignals &captured)
{
    captured.setWindowLookup([](quint64) -> std::optional<CapturedWindow> { return CapturedWindow {{getpid(), {}}, false}; });
    m_bus.setWanted(SignalClient::Borders, kPlaying, audioWanted());
}

void TestAudioSignals::wantedWindowsGetTheirOwnAudio()
{
    AudioSignals captured(m_bus);
    captured.setWindowLookup([](quint64 id) -> std::optional<CapturedWindow> {
        return CapturedWindow {{id == kPlaying ? getpid() : 0, id == kPlaying ? QString() : QStringLiteral("stranger.desktop")}, false};
    });
    m_bus.setWanted(SignalClient::Borders, kPlaying, audioWanted());
    m_bus.setWanted(SignalClient::Borders, kStranger, audioWanted());
    captured.setBeatThreshold(1.4F);
    QTRY_VERIFY_WITH_TIMEOUT(heard(kPlaying), kSettleMs);
    QCOMPARE(m_bus.frame(kStranger).value(Signal::AudioLevel), 0.0F);
}

void TestAudioSignals::nothingIsCapturedWithoutALookupOrAWantedSignal()
{
    AudioSignals captured(m_bus);
    m_bus.setWanted(SignalClient::Borders, kPlaying, audioWanted());
    captured.refresh();
    QTest::qWait(500);
    QCOMPARE(m_bus.frame(kPlaying).value(Signal::AudioLevel), 0.0F);

    captured.setWindowLookup([](quint64) -> std::optional<CapturedWindow> { return CapturedWindow {{getpid(), {}}, false}; });
    m_bus.clearWanted(SignalClient::Borders);
    SignalSet unrelated;
    unrelated.set(static_cast<std::size_t>(Signal::Click));
    m_bus.setWanted(SignalClient::Borders, kPlaying, unrelated);
    QTest::qWait(500);
    QVERIFY(!m_linked);
    QVERIFY(!heard(kPlaying));
}

void TestAudioSignals::fullscreenAndUnknownWindowsAreNotCaptured()
{
    AudioSignals captured(m_bus);
    captured.setWindowLookup([](quint64 id) -> std::optional<CapturedWindow> {
        if (id == kStranger) {
            return std::nullopt;
        }
        return CapturedWindow {{getpid(), {}}, true};
    });
    m_bus.setWanted(SignalClient::Borders, kPlaying, audioWanted());
    m_bus.setWanted(SignalClient::Borders, kStranger, audioWanted());
    QTest::qWait(1000);
    QVERIFY(!heard(kPlaying));
    QVERIFY(!heard(kStranger));
}

void TestAudioSignals::reducedMotionAndBatteryStopTheCapture()
{
    AudioSignals captured(m_bus);
    wantPlayingWindow(captured);
    QTRY_VERIFY_WITH_TIMEOUT(heard(kPlaying), kSettleMs);

    captured.setReduceMotion(true);
    QCOMPARE(m_bus.frame(kPlaying).value(Signal::AudioLevel), 0.0F);
    captured.setReduceMotion(false);
    m_linked = false;
    QTRY_VERIFY_WITH_TIMEOUT(heard(kPlaying), kSettleMs);

    captured.setOnBattery(true);
    QCOMPARE(m_bus.frame(kPlaying).value(Signal::AudioLevel), 0.0F);
    captured.setOnBattery(false);
    m_linked = false;
    QTRY_VERIFY_WITH_TIMEOUT(heard(kPlaying), kSettleMs);
}

void TestAudioSignals::windowsThatNoLongerWantAudioAreCleared()
{
    AudioSignals captured(m_bus);
    wantPlayingWindow(captured);
    QTRY_VERIFY_WITH_TIMEOUT(heard(kPlaying), kSettleMs);

    m_bus.setWanted(SignalClient::Borders, kPlaying, {});
    m_bus.setWanted(SignalClient::Borders, kPlaying, audioWanted());
    QCOMPARE(m_bus.frame(kPlaying).value(Signal::AudioLevel), 0.0F);
}

QTEST_GUILESS_MAIN(TestAudioSignals)
#include "test_audiosignals.moc"
