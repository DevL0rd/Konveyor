#include "fuzzrunner.h"

#include <QHashSeed>
#include <QRegularExpression>

using namespace LayoutTest;

namespace
{

struct Scenario
{
    quint32 arrangement = 0;
    bool animated = false;
};

FuzzFailure replay(const QList<Event> &events, Scenario scenario, std::map<QString, int> *successes = nullptr)
{
    FuzzRun run(scenario.arrangement, scenario.animated, successes);
    return run.run(events);
}

QString signature(QString error)
{
    return error.remove(QRegularExpression(QStringLiteral("[-0-9.]+")));
}

QList<Event> minimized(QList<Event> events, Scenario scenario, const QString &wanted)
{
    qsizetype chunk = std::max<qsizetype>(1, events.size() / 2);
    while (true) {
        bool removed = false;
        for (qsizetype start = 0; start < events.size();) {
            QList<Event> candidate = events;
            candidate.remove(start, std::min(chunk, events.size() - start));
            const FuzzFailure failure = replay(candidate, scenario);
            if (failure.failed() && signature(failure.error) == wanted) {
                events = candidate.first(failure.index + 1);
                removed = true;
            } else {
                start += chunk;
            }
        }
        if (!removed && chunk == 1) {
            return events;
        }
        chunk = removed ? chunk : chunk / 2;
    }
}

QString report(const QList<Event> &events, Scenario scenario, const FuzzFailure &failure)
{
    QList<Event> failing = events.first(failure.index + 1);
    const QList<Event> trace = minimized(failing, scenario, signature(failure.error));
    QStringList lines {QStringLiteral("%1 (arrangement %2, animations %3)")
                           .arg(failure.error)
                           .arg(scenario.arrangement)
                           .arg(scenario.animated ? QStringLiteral("on") : QStringLiteral("off")),
        QStringLiteral("minimal trace:")};
    for (const Event &event : trace) {
        lines.append(QStringLiteral("  ") + describe(event));
    }
    return lines.join(QLatin1Char('\n'));
}

QList<Event> everyActionEvents(quint32 seed, const QString &name)
{
    QList<Event> events = generateEvents(seed, 40);
    events.removeIf([](const Event &event) { return event.step == Step::Act || event.step == Step::RemoveWindow; });
    Dice dice(seed ^ static_cast<quint32>(qHash(name, 0)));
    for (int i = 0; i < 4; ++i) {
        events.append(Event {Step::Act, 0, 0, 0, Config::Action {name, actionArguments(dice, name), actionProperties(dice, name)}});
        events.append(Event {Step::Advance, dice.below(200) * 5 + 1, 0, 0, {}});
    }
    return events;
}

QStringList removeAll(Fixture &fixture, const QList<Layout::WindowId> &windows)
{
    QStringList errors;
    for (const Layout::WindowId id : windows) {
        fixture.remove(id);
        fixture.advance(1);
        errors.append(fixture.invariants());
    }
    errors.removeAll(QString());
    return errors;
}

QString cycleSecondOutput(Fixture &fixture)
{
    fixture.addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
    QString error = fixture.invariants();
    fixture.perform(QStringLiteral("move-window-to-monitor-right"));
    error = error.isEmpty() ? fixture.invariants() : error;
    fixture.removeOutput(QStringLiteral("DP-2"));
    return error.isEmpty() ? fixture.invariants() : error;
}

}

class TestLayoutInvariants : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QHashSeed::setDeterministicGlobalSeed(); }

    void randomEventsKeepInvariants_data()
    {
        QTest::addColumn<quint32>("seed");
        for (quint32 seed = 1; seed <= 64; ++seed) {
            QTest::newRow(qPrintable(QStringLiteral("seed-%1").arg(seed))) << seed;
        }
    }

    void randomEventsKeepInvariants()
    {
        QFETCH(quint32, seed);
        const Scenario scenario {seed / 2, seed % 2 == 0};
        const QList<Event> events = generateEvents(seed, 300);
        const FuzzFailure failure = replay(events, scenario, &m_successes);
        QVERIFY2(!failure.failed(),
            qPrintable(QStringLiteral("seed %1: %2").arg(seed).arg(failure.failed() ? report(events, scenario, failure) : QString())));
    }

    void everyActionKeepsInvariants_data()
    {
        QTest::addColumn<QString>("name");
        for (const QString &name : actionNames()) {
            QTest::newRow(qPrintable(name)) << name;
        }
    }

    void everyActionKeepsInvariants()
    {
        QFETCH(QString, name);
        for (quint32 seed = 1; seed <= 4; ++seed) {
            const Scenario scenario {seed, seed % 2 == 1};
            const QList<Event> events = everyActionEvents(seed, name);
            const FuzzFailure failure = replay(events, scenario, &m_successes);
            QVERIFY2(!failure.failed(), qPrintable(failure.failed() ? report(events, scenario, failure) : QString()));
        }
    }

    void everyActionSucceededSomewhere()
    {
        QStringList never;
        for (const QString &name : actionNames()) {
            if (m_successes[name] == 0) {
                never.append(name);
            }
        }
        QCOMPARE(never, QStringList());
    }

    void removingEveryWindowLeavesOneEmptyWorkspacePerOutput()
    {
        Fixture fixture;
        QList<Layout::WindowId> windows;
        for (int i = 0; i < 6; ++i) {
            windows.append(fixture.add(QStringLiteral("app")));
        }
        fixture.perform(QStringLiteral("move-window-to-workspace-down"));
        fixture.advance(1);
        QCOMPARE(removeAll(fixture, windows), QStringList());
        QCOMPARE(fixture.engine().windowStates().size(), 0);
        QVERIFY(fixture.engine().workspaceStates().size() <= 2);
        QVERIFY(fixture.engine().workspaceStates().first().isActive);
        QCOMPARE(fixture.engine().focusedWindow(), std::optional<Layout::WindowId>());
    }

    void outputRemovalAndReadditionKeepInvariants()
    {
        Fixture fixture;
        for (int i = 0; i < 4; ++i) {
            fixture.add(QStringLiteral("app"));
        }
        QStringList errors;
        for (int round = 0; round < 5; ++round) {
            errors.append(cycleSecondOutput(fixture));
        }
        errors.removeAll(QString());
        QCOMPARE(errors, QStringList());
        QCOMPARE(fixture.engine().windowStates().size(), 4);
    }

private:
    std::map<QString, int> m_successes;
};

QTEST_GUILESS_MAIN(TestLayoutInvariants)
#include "test_layout_invariants.moc"
