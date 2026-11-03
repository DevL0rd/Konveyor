#include "render/signals.h"

#include <QTest>

#include <cmath>

using namespace Konveyor;
using Render::Signal;
using Render::SignalSource;

namespace
{

std::size_t sourceIndex(SignalSource source)
{
    return static_cast<std::size_t>(source);
}

}

class TestSignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourcelessSignalsAreAlwaysPresent()
    {
        const Render::SignalFrame frame;
        QCOMPARE(frame.presence(SignalSource::None), 1.0F);
        QCOMPARE(frame.presence(SignalSource::Audio), 0.0F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 0.0F);
    }

    void hearingMarksASourceAsJustSeen()
    {
        Render::SignalFrame frame;
        frame.presences[sourceIndex(SignalSource::View)] = {0.0F, 9.0};
        frame.hear(SignalSource::View);
        QCOMPARE(frame.presence(SignalSource::View), 1.0F);
        QCOMPARE(frame.presences[sourceIndex(SignalSource::View)].idleSeconds, 0.0);
        frame.hear(SignalSource::None);
        QCOMPARE(frame.presence(SignalSource::None), 1.0F);
    }

    void aRisingInputValueWakesItsSource()
    {
        Render::SignalFrame frame;
        frame.set(Signal::ScrollSpeed, 0.5F);
        QCOMPARE(frame.value(Signal::ScrollSpeed), 0.5F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 1.0F);
        frame.presences[sourceIndex(SignalSource::Scroll)] = {0.0F, 9.0};
        frame.set(Signal::ScrollSpeed, 0.5F);
        frame.set(Signal::ScrollSpeed, 0.2F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 0.0F);
        frame.set(Signal::ScrollSpeed, 0.6F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 1.0F);
    }

    void aNegativeValueCountsByItsSize()
    {
        Render::SignalFrame frame;
        frame.set(Signal::ScrollDirection, -1.0F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 1.0F);
        QCOMPARE(frame.value(Signal::ScrollDirection), -1.0F);
    }

    void audioValuesDoNotWakeThemselves()
    {
        Render::SignalFrame frame;
        frame.set(Signal::AudioLevel, 0.9F);
        QCOMPARE(frame.value(Signal::AudioLevel), 0.9F);
        QCOMPARE(frame.presence(SignalSource::Audio), 0.0F);
    }

    void firingCountsAndMaxesAnImpulse()
    {
        Render::SignalFrame frame;
        frame.fire(Signal::ScrollStep);
        frame.fire(Signal::ScrollStep);
        QCOMPARE(frame.fireCount(Signal::ScrollStep), quint32(2));
        QCOMPARE(frame.value(Signal::ScrollStep), 1.0F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 1.0F);
        frame.fire(Signal::Click);
        QCOMPARE(frame.fireCount(Signal::Click), quint32(1));
        QCOMPARE(frame.fireCount(Signal::Keypress), quint32(0));
    }

    void firingRemembersWhereItHappened()
    {
        Render::SignalFrame frame;
        frame.fire(Signal::Click, {{10, 20}, {1, 0}, true});
        QCOMPARE(frame.point.position, QPointF(10, 20));
        QVERIFY(frame.point.valid);
        frame.fire(Signal::Click, {{99, 99}, {0, 1}, false});
        QCOMPARE(frame.point.position, QPointF(10, 20));
        QCOMPARE(frame.fireCount(Signal::Click), quint32(2));
    }

    void audioFramesFillTheAudioSignals()
    {
        Render::AudioFrame audio;
        audio.level = 0.5F;
        audio.bass = 0.4F;
        audio.mid = 0.3F;
        audio.treble = 0.2F;
        audio.bands[3] = 0.7F;
        audio.wave[10] = -0.4F;
        Render::SignalFrame frame;
        frame.setAudio(audio);
        QCOMPARE(frame.value(Signal::AudioLevel), 0.5F);
        QCOMPARE(frame.value(Signal::AudioBass), 0.4F);
        QCOMPARE(frame.value(Signal::AudioMid), 0.3F);
        QCOMPARE(frame.value(Signal::AudioTreble), 0.2F);
        QCOMPARE(frame.bands[3], 0.7F);
        QCOMPARE(frame.wave[10], -0.4F);
        QCOMPARE(frame.presence(SignalSource::Audio), 1.0F);
        QCOMPARE(frame.fireCount(Signal::AudioBeat), quint32(0));
    }

    void quietAudioIsNotHeard()
    {
        Render::AudioFrame audio;
        audio.level = 0.005F;
        Render::SignalFrame frame;
        frame.setAudio(audio);
        QCOMPARE(frame.presence(SignalSource::Audio), 0.0F);
    }

    void aBeatFiresOnlyOnItsRisingEdge()
    {
        Render::AudioFrame audio;
        audio.beat = 1.0F;
        Render::SignalFrame frame;
        frame.setAudio(audio);
        QCOMPARE(frame.fireCount(Signal::AudioBeat), quint32(1));
        frame.setAudio(audio);
        QCOMPARE(frame.fireCount(Signal::AudioBeat), quint32(1));
        audio.beat = 0.3F;
        frame.setAudio(audio);
        QCOMPARE(frame.value(Signal::AudioBeat), 1.0F);
        frame.decayImpulses(5.0);
        audio.beat = 1.0F;
        frame.setAudio(audio);
        QCOMPARE(frame.fireCount(Signal::AudioBeat), quint32(2));
    }

    void aSmallBeatRiseIsNoNewBeat()
    {
        Render::AudioFrame audio;
        audio.beat = 0.5F;
        Render::SignalFrame frame;
        frame.setAudio(audio);
        QCOMPARE(frame.fireCount(Signal::AudioBeat), quint32(1));
        frame.decayImpulses(0.25);
        audio.beat = 0.45F;
        frame.setAudio(audio);
        QCOMPARE(frame.fireCount(Signal::AudioBeat), quint32(1));
        QCOMPARE(frame.value(Signal::AudioBeat), 0.45F);
    }

    void impulsesDecayButLevelsStay()
    {
        Render::SignalFrame frame;
        frame.set(Signal::AudioLevel, 0.8F);
        frame.set(Signal::Time, 0.6F);
        frame.fire(Signal::Click);
        frame.fire(Signal::Urgent);
        frame.decayImpulses(0.25);
        QVERIFY(std::abs(frame.value(Signal::Click) - std::exp(-1.0F)) < 1e-6F);
        QVERIFY(std::abs(frame.value(Signal::Urgent) - std::exp(-1.0F)) < 1e-6F);
        QCOMPARE(frame.value(Signal::AudioLevel), 0.8F);
        QCOMPARE(frame.value(Signal::ScrollSpeed), 0.0F);
        QCOMPARE(frame.value(Signal::Time), 0.6F);
    }

    void inputPresenceHoldsThenFades()
    {
        Render::SignalFrame frame;
        frame.hear(SignalSource::Pointer);
        frame.fadePresences(1.0);
        QCOMPARE(frame.presence(SignalSource::Pointer), 1.0F);
        frame.fadePresences(0.25);
        QCOMPARE(frame.presence(SignalSource::Pointer), 0.5F);
        frame.fadePresences(0.25);
        QCOMPARE(frame.presence(SignalSource::Pointer), 0.0F);
        frame.fadePresences(5.0);
        QCOMPARE(frame.presence(SignalSource::Pointer), 0.0F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 0.0F);
    }

    void audioPresenceHoldsLongerThanInput()
    {
        Render::SignalFrame frame;
        frame.hear(SignalSource::Audio);
        frame.hear(SignalSource::Scroll);
        frame.fadePresences(1.5);
        QCOMPARE(frame.presence(SignalSource::Audio), 1.0F);
        QCOMPARE(frame.presence(SignalSource::Scroll), 0.0F);
        frame.fadePresences(1.0);
        QCOMPARE(frame.presence(SignalSource::Audio), 0.5F);
        frame.fadePresences(1.0);
        QCOMPARE(frame.presence(SignalSource::Audio), 0.0F);
    }

    void simulatedSignalsAreAllPresent()
    {
        const Render::SignalFrame frame = Render::simulatedSignals(1.0);
        for (std::size_t index = 0; index < Render::kSignalSourceCount; ++index) {
            QCOMPARE(frame.presences[index].level, 1.0F);
        }
        QVERIFY(frame.value(Signal::AudioLevel) > 0.0F);
    }

    void simulatedImpulsesCountTheirCycles()
    {
        const Render::SignalFrame start = Render::simulatedSignals(0.0);
        QCOMPARE(start.fireCount(Signal::Click), quint32(0));
        QCOMPARE(start.value(Signal::Click), 1.0F);
        const Render::SignalFrame later = Render::simulatedSignals(3.6);
        QCOMPARE(later.fireCount(Signal::Click), quint32(1));
        QVERIFY(later.value(Signal::Click) < 0.1F);
        QCOMPARE(Render::simulatedSignals(1.25).fireCount(Signal::AudioBeat), quint32(2));
        QVERIFY(std::abs(Render::simulatedSignals(0.25).value(Signal::AudioBeat) - std::exp(-1.0F)) < 1e-5F);
    }

    void simulatedTravelFlipsDirection()
    {
        QCOMPARE(Render::simulatedSignals(1.0).value(Signal::ScrollDirection), 1.0F);
        QCOMPARE(Render::simulatedSignals(4.0).value(Signal::ScrollDirection), -1.0F);
        QCOMPARE(Render::simulatedSignals(2.5).value(Signal::ViewDirection), 1.0F);
        QCOMPARE(Render::simulatedSignals(7.5).value(Signal::ViewDirection), -1.0F);
        QVERIFY(std::abs(Render::simulatedSignals(3.0).value(Signal::ScrollDistance) - 0.5F) < 1e-6F);
        QVERIFY(std::abs(Render::simulatedSignals(5.0).value(Signal::ViewDistance) - 0.5F) < 1e-6F);
        QVERIFY(std::abs(Render::simulatedSignals(4.0).value(Signal::Time) - 0.5F) < 1e-6F);
    }

    void simulatedSpeedsWobbleInRange()
    {
        for (double seconds = 0.0; seconds < 12.0; seconds += 0.37) {
            const Render::SignalFrame frame = Render::simulatedSignals(seconds);
            for (const Signal signal : {Signal::ScrollSpeed, Signal::PointerSpeed, Signal::TypingRate, Signal::ViewSpeed}) {
                QVERIFY(frame.value(signal) >= 0.0F && frame.value(signal) <= 1.0F);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestSignals)
#include "test_signals.moc"
