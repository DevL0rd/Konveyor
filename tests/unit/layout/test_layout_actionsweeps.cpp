#include "fuzzevents.h"

using namespace LayoutTest;

namespace
{

QString sweep(Layout::Engine &engine, Fixture *fixture, std::optional<Layout::WindowId> target, quint32 seed)
{
    Dice dice(seed);
    for (const QString &name : actionNames()) {
        const Config::Action act {name, actionArguments(dice, name), actionProperties(dice, name)};
        QList<std::pair<QString, QString>> properties = act.properties;
        for (auto &property : properties) {
            if (property.second == WindowPlaceholder) {
                property.second = QString::number(target.value_or(0));
            }
        }
        engine.perform(Config::Action {name, act.arguments, properties}, target);
        if (fixture) {
            fixture->settle();
        }
        const QString error = engine.checkConsistency();
        if (!error.isEmpty()) {
            return QStringLiteral("%1: %2").arg(name, error);
        }
    }
    return {};
}

}

class TestLayoutActionSweeps : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everyActionIsHarmlessWithoutOutputs()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        Layout::Engine engine(clock);
        engine.setConfig(instantConfig());
        for (quint32 seed = 1; seed <= 3; ++seed) {
            QCOMPARE(sweep(engine, nullptr, std::nullopt, seed), QString());
            QCOMPARE(sweep(engine, nullptr, 999, seed), QString());
        }
    }

    void everyActionIsHarmlessOnEmptyOutputs()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720));
        for (quint32 seed = 1; seed <= 3; ++seed) {
            QCOMPARE(sweep(fixture.engine(), &fixture, std::nullopt, seed), QString());
        }
    }

    void everyActionIsHarmlessForUnknownWindows()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        for (quint32 seed = 1; seed <= 3; ++seed) {
            QCOMPARE(sweep(fixture.engine(), &fixture, 999, seed), QString());
        }
    }
};

QTEST_GUILESS_MAIN(TestLayoutActionSweeps)
#include "test_layout_actionsweeps.moc"
