#include "render/signals.h"

#include <QTest>

#include <cmath>

using namespace Konveyor;
using Render::Signal;
using Render::SignalClient;
using Render::SignalSource;

namespace
{

Render::SignalSet setOf(std::initializer_list<Signal> wanted)
{
    Render::SignalSet found;
    for (const Signal signal : wanted) {
        found.set(static_cast<std::size_t>(signal));
    }
    return found;
}

}

class TestSignalBus : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unknownWindowsAreSilent()
    {
        const Render::SignalBus bus;
        QCOMPARE(bus.frame(42).value(Signal::AudioLevel), 0.0F);
        QCOMPARE(bus.frame(42).fireCount(Signal::Click), quint32(0));
        QVERIFY(bus.wantedAnywhere().none());
        QVERIFY(bus.windowsWanting(Signal::Click).empty());
        QVERIFY(!bus.wanted(42, Signal::Click));
    }

    void onlyTheWantingWindowReceives()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click, Signal::ScrollSpeed}));
        bus.fire(1, Signal::Click, {{5, 6}, {}, true});
        bus.fire(2, Signal::Click);
        bus.fire(1, Signal::Keypress);
        QCOMPARE(bus.frame(1).fireCount(Signal::Click), quint32(1));
        QCOMPARE(bus.frame(1).point.position, QPointF(5, 6));
        QCOMPARE(bus.frame(2).fireCount(Signal::Click), quint32(0));
        QCOMPARE(bus.frame(1).fireCount(Signal::Keypress), quint32(0));
        QVERIFY(bus.wanted(1, Signal::Click));
        QVERIFY(!bus.wanted(2, Signal::Click));
    }

    void audioNeedsTheLevelSignal()
    {
        Render::SignalBus bus;
        Render::AudioFrame audio;
        audio.level = 0.5F;
        audio.bands[2] = 0.9F;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::AudioBass}));
        bus.setAudio(1, audio);
        QCOMPARE(bus.frame(1).value(Signal::AudioLevel), 0.0F);
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::AudioLevel}));
        bus.setAudio(1, audio);
        QCOMPARE(bus.frame(1).value(Signal::AudioLevel), 0.5F);
        QCOMPARE(bus.frame(1).bands[2], 0.9F);
    }

    void everyWindowWantingGetsAValue()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::PointerSpeed}));
        bus.setWanted(SignalClient::Events, 2, setOf({Signal::PointerSpeed}));
        bus.setWanted(SignalClient::Borders, 3, setOf({Signal::Click}));
        bus.setOnEveryWindow(Signal::PointerSpeed, 0.4F);
        QCOMPARE(bus.frame(1).value(Signal::PointerSpeed), 0.4F);
        QCOMPARE(bus.frame(2).value(Signal::PointerSpeed), 0.4F);
        QCOMPARE(bus.frame(3).value(Signal::PointerSpeed), 0.0F);
        QCOMPARE(bus.windowsWanting(Signal::PointerSpeed), (std::vector<quint64> {1, 2}));
        QCOMPARE(bus.windowsWanting(Signal::Click), (std::vector<quint64> {3}));
    }

    void clientsClaimIndependently()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        bus.setWanted(SignalClient::Events, 1, setOf({Signal::Keypress}));
        QVERIFY(bus.wanted(1, Signal::Click));
        QVERIFY(bus.wanted(1, Signal::Keypress));
        QCOMPARE(bus.wantedAnywhere(), setOf({Signal::Click, Signal::Keypress}));
        bus.setWanted(SignalClient::Borders, 1, {});
        QVERIFY(!bus.wanted(1, Signal::Click));
        QVERIFY(bus.wanted(1, Signal::Keypress));
        QCOMPARE(bus.wantedAnywhere(), setOf({Signal::Keypress}));
        QCOMPARE(bus.windowsWanting(Signal::Keypress), (std::vector<quint64> {1}));
        bus.setWanted(SignalClient::Events, 1, {});
        QVERIFY(bus.wantedAnywhere().none());
        QVERIFY(bus.windowsWanting(Signal::Keypress).empty());
    }

    void listenersHearOnlyRealChanges()
    {
        Render::SignalBus bus;
        int changes = 0;
        bus.onWantedChanged([&changes] { ++changes; });
        bus.setWanted(SignalClient::Borders, 1, {});
        QCOMPARE(changes, 0);
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        QCOMPARE(changes, 1);
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        QCOMPARE(changes, 1);
        bus.setWanted(SignalClient::Events, 1, setOf({Signal::Click}));
        QCOMPARE(changes, 2);
        bus.setWantedEverywhere(SignalClient::Events, setOf({Signal::Focus}));
        QCOMPARE(changes, 3);
        bus.setWantedEverywhere(SignalClient::Events, setOf({Signal::Focus}));
        QCOMPARE(changes, 3);
        bus.remove(7);
        QCOMPARE(changes, 3);
        bus.remove(1);
        QCOMPARE(changes, 4);
        QVERIFY(!bus.wanted(1, Signal::Click));
        bus.clearWanted(SignalClient::Events);
        QCOMPARE(changes, 5);
    }

    void everywhereClaimsReachUnclaimedWindows()
    {
        Render::SignalBus bus;
        bus.setWantedEverywhere(SignalClient::Borders, setOf({Signal::Focus}));
        QVERIFY(bus.wanted(99, Signal::Focus));
        QVERIFY(!bus.wanted(99, Signal::Click));
        QCOMPARE(bus.wantedAnywhere(), setOf({Signal::Focus}));
        bus.fire(99, Signal::Focus);
        QCOMPARE(bus.frame(99).fireCount(Signal::Focus), quint32(1));
        bus.setWanted(SignalClient::Events, 5, setOf({Signal::Click}));
        QCOMPARE(bus.frame(99).fireCount(Signal::Focus), quint32(1));
        QVERIFY(bus.windowsWanting(Signal::Focus).empty());
        bus.setWantedEverywhere(SignalClient::Borders, {});
        QCOMPARE(bus.frame(99).fireCount(Signal::Focus), quint32(0));
    }

    void everywhereClaimsOfEitherClientKeepUnclaimedFrames_data()
    {
        QTest::addColumn<SignalClient>("client");
        QTest::newRow("borders") << SignalClient::Borders;
        QTest::newRow("events") << SignalClient::Events;
    }

    void everywhereClaimsOfEitherClientKeepUnclaimedFrames()
    {
        QFETCH(SignalClient, client);
        Render::SignalBus bus;
        bus.setWantedEverywhere(client, setOf({Signal::Focus}));
        QVERIFY(bus.wanted(99, Signal::Focus));
        bus.fire(99, Signal::Focus);
        bus.setWanted(SignalClient::Borders, 5, setOf({Signal::Click}));
        QCOMPARE(bus.frame(99).fireCount(Signal::Focus), quint32(1));
        bus.setWantedEverywhere(client, {});
        QVERIFY(!bus.wanted(99, Signal::Focus));
        QCOMPARE(bus.frame(99).fireCount(Signal::Focus), quint32(0));
    }

    void claimedWindowsKeepTheirFramesWhenOtherClaimsChange()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        bus.fire(1, Signal::Click);
        bus.setWanted(SignalClient::Events, 2, setOf({Signal::Keypress}));
        QCOMPARE(bus.frame(1).fireCount(Signal::Click), quint32(1));
    }

    void clearingAClientKeepsTheOther()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        bus.setWanted(SignalClient::Events, 1, setOf({Signal::Keypress}));
        bus.setWanted(SignalClient::Borders, 2, setOf({Signal::Click}));
        bus.setWantedEverywhere(SignalClient::Borders, setOf({Signal::Focus}));
        bus.clearWanted(SignalClient::Borders);
        QVERIFY(!bus.wanted(1, Signal::Click));
        QVERIFY(!bus.wanted(2, Signal::Click));
        QVERIFY(!bus.wanted(3, Signal::Focus));
        QVERIFY(bus.wanted(1, Signal::Keypress));
        QCOMPARE(bus.wantedAnywhere(), setOf({Signal::Keypress}));
    }

    void clearingOneClientKeepsTheWindowsOfTheOther()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        bus.setWanted(SignalClient::Events, 1, setOf({Signal::Keypress}));
        bus.setWanted(SignalClient::Events, 2, setOf({Signal::Keypress}));
        bus.clearWanted(SignalClient::Events);
        QVERIFY(bus.wanted(1, Signal::Click));
        QVERIFY(!bus.wanted(1, Signal::Keypress));
        QVERIFY(!bus.wanted(2, Signal::Keypress));
        QCOMPARE(bus.wantedAnywhere(), setOf({Signal::Click}));
    }

    void removingAWindowDropsItsFrame()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        bus.fire(1, Signal::Click);
        bus.remove(1);
        QCOMPARE(bus.frame(1).fireCount(Signal::Click), quint32(0));
        QVERIFY(bus.wantedAnywhere().none());
    }

    void advancingDecaysAndFades()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click, Signal::ScrollSpeed}));
        bus.fire(1, Signal::Click);
        bus.set(1, Signal::ScrollSpeed, 0.5F);
        bus.advance(0.25);
        QVERIFY(std::abs(bus.frame(1).value(Signal::Click) - std::exp(-1.0F)) < 1e-6F);
        QCOMPARE(bus.frame(1).presence(SignalSource::Scroll), 1.0F);
        for (int step = 2; step <= 8; ++step) {
            bus.advance(step * 0.25);
        }
        QCOMPARE(bus.frame(1).presence(SignalSource::Scroll), 0.0F);
    }

    void advancingNeverStepsMoreThanAQuarterSecond()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Click}));
        bus.fire(1, Signal::Click);
        bus.advance(100.0);
        QVERIFY(std::abs(bus.frame(1).value(Signal::Click) - std::exp(-1.0F)) < 1e-6F);
        bus.fire(1, Signal::Click);
        bus.advance(50.0);
        QCOMPARE(bus.frame(1).value(Signal::Click), 1.0F);
    }

    void timeSignalFollowsTheClock()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::Time}));
        bus.set(1, Signal::Time, 0.0F);
        bus.advance(20.0);
        QVERIFY(std::abs(bus.frame(1).value(Signal::Time) - 0.5F) < 1e-6F);
    }

    void aSilentSourceWakesOnce()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::ScrollStep, Signal::AudioLevel, Signal::ScrollSpeed}));
        std::vector<quint64> woken;
        bus.onSourceReturned([&woken](quint64 window) { woken.push_back(window); });
        bus.fire(1, Signal::ScrollStep);
        bus.fire(1, Signal::ScrollStep);
        QCOMPARE(woken, (std::vector<quint64> {1}));
        Render::AudioFrame audio;
        audio.level = 0.5F;
        bus.setAudio(1, audio);
        QCOMPARE(woken, (std::vector<quint64> {1, 1}));
        bus.setAudio(1, audio);
        QCOMPARE(woken.size(), std::size_t(2));
        for (int step = 1; step <= 8; ++step) {
            bus.advance(step * 0.5);
        }
        bus.set(1, Signal::ScrollSpeed, 0.3F);
        QCOMPARE(woken.size(), std::size_t(3));
    }

    void anUnchangedSilentSourceDoesNotWake()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::ScrollSpeed, Signal::AudioLevel}));
        int woken = 0;
        bus.onSourceReturned([&woken](quint64) { ++woken; });
        bus.set(1, Signal::ScrollSpeed, 0.0F);
        bus.setAudio(1, {});
        QCOMPARE(woken, 0);
        QCOMPARE(bus.frame(1).presence(SignalSource::Scroll), 0.0F);
    }

    void sourcesWithoutAListenerStillReturn()
    {
        Render::SignalBus bus;
        bus.setWanted(SignalClient::Borders, 1, setOf({Signal::ScrollSpeed}));
        bus.set(1, Signal::ScrollSpeed, 0.3F);
        QCOMPARE(bus.frame(1).presence(SignalSource::Scroll), 1.0F);
    }
};

QTEST_GUILESS_MAIN(TestSignalBus)
#include "test_signalbus.moc"
