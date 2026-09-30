#include "helpers.h"

using namespace LayoutTest;

namespace
{

constexpr qint64 PastStartupMs = 61000;

Config::Config withHeightRule(std::optional<Config::PresetSize> height)
{
    Config::Config config = instantConfig();
    Config::WindowRule rule = ruleFor(QStringLiteral("app"));
    rule.defaultWindowHeight = height;
    config.windowRules.append(rule);
    return config;
}

Config::WindowRule startupRule()
{
    Config::WindowRule rule;
    Config::Match match;
    match.atStartup = true;
    rule.matches.append(match);
    return rule;
}

}

class TestLayoutRuleEffects : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultWindowHeightSizesALoneWindow()
    {
        Fixture fixture(withHeightRule(Config::Proportion {0.25}));
        QCOMPARE(fixture.frame(fixture.add(QStringLiteral("app"))), QRectF(16, 16, 936, 250));
        QCOMPARE(fixture.frame(fixture.add(QStringLiteral("other"))).height(), 1048.0);
    }

    void defaultWindowHeightSizesAStackedRow()
    {
        Config::Config config = withHeightRule(Config::Fixed {300});
        config.layout.newWindowPlacement = Config::NewWindowPlacement::Stack;
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("other"));
        const auto second = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(second).columnIndex, fixture.state(first).columnIndex);
        QCOMPARE(fixture.frame(second).height(), 300.0);
        QCOMPARE(fixture.frame(first).height(), 732.0);
        VERIFY_INVARIANTS(fixture);
    }

    void defaultWindowHeightSizesAFloatingWindow()
    {
        Config::Config config = withHeightRule(Config::Fixed {333});
        config.windowRules[0].openFloating = true;
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 200));
        QVERIFY(fixture.state(id).isFloating);
        QCOMPARE(fixture.frame(id).height(), 333.0);
    }

    void emptyDefaultWindowHeightKeepsTheAutomaticHeight()
    {
        Config::Config config = withHeightRule(std::nullopt);
        Fixture fixture(config);
        QCOMPARE(fixture.frame(fixture.add(QStringLiteral("app"))).height(), 1048.0);
    }

    void atStartupRulesStopMatchingNewWindowsAfterAMinute()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = startupRule();
        rule.openFloating = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto early = fixture.add(QStringLiteral("early"));
        QVERIFY(fixture.state(early).isFloating);
        fixture.advance(PastStartupMs);
        const auto late = fixture.add(QStringLiteral("late"));
        QVERIFY(!fixture.state(late).isFloating);
        QVERIFY(fixture.state(early).isFloating);
    }

    void atStartupAppearanceRulesLetGoOfOpenWindows()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = startupRule();
        rule.opacity = 0.5;
        rule.focusRing.width = 12;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(id).ruleOpacity, 0.5);
        fixture.advance(PastStartupMs);
        QCOMPARE(fixture.state(id).ruleOpacity, 1.0);
        QCOMPARE(fixture.state(id).focusRing.width, 4.0);
    }

    void refreshingRulesAtTheEndOfStartupDropsStartupRules()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = startupRule();
        rule.opacity = 0.5;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.engine().timeUntilStartupEnds(), Konveyor::Anim::Duration(std::chrono::seconds(60)));
        fixture.clock().setRawNow(std::chrono::seconds(60));
        QCOMPARE(fixture.engine().timeUntilStartupEnds(), Konveyor::Anim::Duration::zero());
        QCOMPARE(fixture.state(id).ruleOpacity, 0.5);
        fixture.engine().refreshRules();
        QCOMPARE(fixture.state(id).ruleOpacity, 1.0);
    }

    void focusRingRuleReachesTheWindowState()
    {
        Config::Config config = instantConfig();
        config.layout.focusRing.enabled = true;
        config.layout.focusRing.width = 4;
        Config::WindowRule rule = ruleFor(QStringLiteral("thick"));
        rule.focusRing.width = 9;
        config.windowRules.append(rule);
        Config::WindowRule off = ruleFor(QStringLiteral("plain"));
        off.focusRing.enabled = false;
        config.windowRules.append(off);
        Fixture fixture(config);
        const auto thick = fixture.add(QStringLiteral("thick"));
        QVERIFY(fixture.state(thick).focusRing.enabled);
        QCOMPARE(fixture.state(thick).focusRing.width, 9.0);
        QCOMPARE(fixture.frame(thick).width(), 936.0);
        const auto plain = fixture.add(QStringLiteral("plain"));
        QVERIFY(!fixture.state(plain).focusRing.enabled);
        fixture.engine().activateWindow(thick);
        QVERIFY(!fixture.state(plain).focusRing.enabled);
        QVERIFY(fixture.state(thick).focusRing.enabled);
    }

    void focusRingFollowsFocus()
    {
        Config::Config config = instantConfig();
        config.layout.focusRing.enabled = true;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.state(b).focusRing.enabled);
        QVERIFY(!fixture.state(a).focusRing.enabled);
        fixture.engine().setLayoutFocused(false);
        QVERIFY(!fixture.state(b).focusRing.enabled);
    }

    void borderRuleWidthChangesTheGeometryOfThatWindowOnly()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("framed"));
        rule.border.enabled = true;
        rule.border.width = 6;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto plain = fixture.add(QStringLiteral("plain"));
        const auto framed = fixture.add(QStringLiteral("framed"));
        QCOMPARE(fixture.frame(plain), QRectF(16, 16, 936, 1048));
        QCOMPARE(fixture.frame(framed), QRectF(974, 22, 924, 1036));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutRuleEffects)
#include "test_layout_ruleeffects.moc"
