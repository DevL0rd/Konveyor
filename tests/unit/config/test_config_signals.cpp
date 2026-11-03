#include "config/types/signal.h"

#include <QTest>

using namespace Konveyor::Config;

class TestConfigSignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namesRoundTripForEverySignal();
    void unknownNamesAreNotSignals();
    void listsEveryNameInOrder();
    void impulsesAreTheEventSignals();
    void groupsSignalsBySource();
    void sourcesFollowTheSignal();
    void anySetHelpersLookAtTheMembers();
};

void TestConfigSignals::namesRoundTripForEverySignal()
{
    for (std::size_t index = 0; index < kSignalCount; ++index) {
        const auto signal = static_cast<Signal>(index);
        const QString name = signalName(signal);
        QVERIFY(!name.isEmpty());
        QCOMPARE(signalNamed(name), std::optional<Signal>(signal));
    }
    QCOMPARE(signalName(Signal::AudioBeat), QStringLiteral("audio.beat"));
    QCOMPARE(signalName(Signal::Urgent), QStringLiteral("urgent"));
}

void TestConfigSignals::unknownNamesAreNotSignals()
{
    QVERIFY(!signalNamed(QStringLiteral("audio.nope")).has_value());
    QVERIFY(!signalNamed(QString()).has_value());
    QVERIFY(!signalNamed(QStringLiteral("Click")).has_value());
}

void TestConfigSignals::listsEveryNameInOrder()
{
    const QStringList names = signalNames();
    QCOMPARE(names.size(), qsizetype(kSignalCount));
    QCOMPARE(names.first(), QStringLiteral("audio.level"));
    QCOMPARE(names.last(), QStringLiteral("urgent"));
    for (qsizetype index = 0; index < names.size(); ++index) {
        QCOMPARE(signalName(static_cast<Signal>(index)), names.at(index));
    }
}

void TestConfigSignals::impulsesAreTheEventSignals()
{
    const QList<Signal> impulses {Signal::AudioBeat, Signal::Click, Signal::ScrollStep, Signal::Keypress, Signal::ViewStep, Signal::Focus,
        Signal::Unfocus, Signal::Urgent};
    for (std::size_t index = 0; index < kSignalCount; ++index) {
        const auto signal = static_cast<Signal>(index);
        QCOMPARE(isImpulse(signal), impulses.contains(signal));
    }
}

void TestConfigSignals::groupsSignalsBySource()
{
    QVERIFY(isAudioSignal(Signal::AudioLevel));
    QVERIFY(isAudioSignal(Signal::AudioTreble));
    QVERIFY(isAudioSignal(Signal::AudioBeat));
    QVERIFY(!isAudioSignal(Signal::ScrollSpeed));
    QVERIFY(!isAudioSignal(Signal::Click));

    for (const Signal signal : {Signal::ScrollSpeed, Signal::ScrollDistance, Signal::ScrollDirection, Signal::PointerSpeed,
             Signal::TypingRate, Signal::Click, Signal::ScrollStep, Signal::Keypress}) {
        QVERIFY(isInputSignal(signal));
    }
    for (const Signal signal : {Signal::AudioLevel, Signal::ViewSpeed, Signal::Time, Signal::AudioBeat, Signal::ViewStep, Signal::Focus}) {
        QVERIFY(!isInputSignal(signal));
    }

    for (const Signal signal : {Signal::ViewSpeed, Signal::ViewDistance, Signal::ViewDirection, Signal::ViewStep}) {
        QVERIFY(isViewSignal(signal));
    }
    QVERIFY(!isViewSignal(Signal::ScrollStep));
    QVERIFY(!isViewSignal(Signal::Time));
}

void TestConfigSignals::sourcesFollowTheSignal()
{
    QCOMPARE(sourceOf(Signal::AudioBass), SignalSource::Audio);
    QCOMPARE(sourceOf(Signal::AudioBeat), SignalSource::Audio);
    QCOMPARE(sourceOf(Signal::ScrollDirection), SignalSource::Scroll);
    QCOMPARE(sourceOf(Signal::ScrollStep), SignalSource::Scroll);
    QCOMPARE(sourceOf(Signal::ViewDistance), SignalSource::View);
    QCOMPARE(sourceOf(Signal::ViewStep), SignalSource::View);
    QCOMPARE(sourceOf(Signal::PointerSpeed), SignalSource::Pointer);
    for (const Signal signal : {Signal::TypingRate, Signal::Time, Signal::Click, Signal::Keypress, Signal::Focus, Signal::Urgent}) {
        QCOMPARE(sourceOf(signal), SignalSource::None);
    }
}

void TestConfigSignals::anySetHelpersLookAtTheMembers()
{
    SignalSet empty;
    QVERIFY(!anyAudioSignal(empty));
    QVERIFY(!anyInputSignal(empty));

    SignalSet audio;
    audio.set(static_cast<std::size_t>(Signal::AudioMid));
    QVERIFY(anyAudioSignal(audio));
    QVERIFY(!anyInputSignal(audio));

    SignalSet input;
    input.set(static_cast<std::size_t>(Signal::Keypress));
    input.set(static_cast<std::size_t>(Signal::Time));
    QVERIFY(anyInputSignal(input));
    QVERIFY(!anyAudioSignal(input));

    SignalSet other;
    other.set(static_cast<std::size_t>(Signal::ViewSpeed));
    other.set(static_cast<std::size_t>(Signal::Focus));
    QVERIFY(!anyAudioSignal(other));
    QVERIFY(!anyInputSignal(other));
}

QTEST_APPLESS_MAIN(TestConfigSignals)
#include "test_config_signals.moc"
