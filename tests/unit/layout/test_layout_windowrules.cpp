#include "helpers.h"
#include "layout/rules/windowrules.h"

using namespace LayoutTest;
using Konveyor::Layout::EffectiveWindowRules;
using Konveyor::Layout::matchApplies;
using Konveyor::Layout::MatchContext;
using Konveyor::Layout::resolveWindowRules;

namespace
{

MatchContext context(const QString &appId, const QString &title)
{
    MatchContext result;
    result.appId = appId;
    result.title = title;
    return result;
}

Config::Match appIdMatch(const QString &pattern)
{
    Config::Match match;
    match.appId = QRegularExpression(pattern);
    return match;
}

}

namespace
{

Config::Config profiledConfig()
{
    Config::Config config = LayoutTest::instantConfig();
    Config::MonitorProfile wide;
    wide.name = QStringLiteral("ultrawide");
    Config::MonitorMatch aspect;
    aspect.aspectRatioAbove = 2.0;
    wide.matches.append(aspect);
    wide.layout = config.layout;
    wide.layout->defaultColumnWidth = Config::Proportion {0.25};
    Config::MonitorProfile standard;
    standard.name = QStringLiteral("standard");
    standard.layout = config.layout;
    standard.layout->defaultColumnWidth = Config::Proportion {0.5};
    config.monitorProfiles = {wide, standard};
    return config;
}

}

class TestLayoutWindowRules : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void manageRuleDecidesByTitleAndAppId()
    {
        Config::Config config = LayoutTest::instantConfig();
        Config::WindowRule rule;
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^steam$"));
        match.title = QRegularExpression(QStringLiteral("^(SteamWebhelper|.+ Menu|notificationtoasts_.*)$"));
        rule.matches.append(match);
        rule.manage = false;
        config.windowRules.append(rule);
        LayoutTest::Fixture fixture(config);
        QVERIFY(!fixture.engine().wantsWindow(LayoutTest::makeWindow(QStringLiteral("steam"), QStringLiteral("SteamWebhelper"))));
        QVERIFY(!fixture.engine().wantsWindow(LayoutTest::makeWindow(QStringLiteral("steam"), QStringLiteral("Friends Root Menu"))));
        QVERIFY(
            !fixture.engine().wantsWindow(LayoutTest::makeWindow(QStringLiteral("steam"), QStringLiteral("notificationtoasts_2_desktop"))));
        QVERIFY(fixture.engine().wantsWindow(LayoutTest::makeWindow(QStringLiteral("steam"), QStringLiteral("Steam"))));
        QVERIFY(fixture.engine().wantsWindow(LayoutTest::makeWindow(QStringLiteral("konsole"), QStringLiteral("Account Menu"))));
    }

    void appRulesCanDependOnTheMonitorProfile()
    {
        Config::Config config = profiledConfig();

        Config::WindowRule browserEverywhere;
        Config::Match browser;
        browser.appId = QRegularExpression(QStringLiteral("^browser$"));
        browserEverywhere.matches.append(browser);
        browserEverywhere.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Proportion {1.0});
        Config::WindowRule browserOnWide;
        Config::Match browserWide = browser;
        browserWide.monitorProfile = QRegularExpression(QStringLiteral("^ultrawide$"));
        browserOnWide.matches.append(browserWide);
        browserOnWide.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Proportion {0.5});
        config.windowRules = {browserEverywhere, browserOnWide};

        const auto columnWidth = [](double view, double proportion) { return proportion * (view - 16.0) - 16.0; };

        Fixture narrow(config, QRectF(0, 0, 1920, 1080));
        QCOMPARE(narrow.frame(narrow.add(QStringLiteral("browser"))).width(), columnWidth(1920.0, 1.0));
        QCOMPARE(narrow.frame(narrow.add(QStringLiteral("other"))).width(), columnWidth(1920.0, 0.5));

        Fixture ultrawide(config, QRectF(0, 0, 5120, 1440));
        QCOMPARE(ultrawide.frame(ultrawide.add(QStringLiteral("browser"))).width(), columnWidth(5120.0, 0.5));
        QCOMPARE(ultrawide.frame(ultrawide.add(QStringLiteral("other"))).width(), columnWidth(5120.0, 0.25));

        const auto browserWindow = narrow.add(QStringLiteral("browser"));
        const auto otherWindow = narrow.add(QStringLiteral("other"));
        narrow.engine().updateOutput(LayoutTest::makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 5120, 1440)));
        narrow.settle();
        QCOMPARE(narrow.frame(browserWindow).width(), columnWidth(5120.0, 0.5));
        QCOMPARE(narrow.frame(otherWindow).width(), columnWidth(5120.0, 0.25));
        VERIFY_INVARIANTS(narrow);
    }

    void monitorProfileSetsSpawnWidthByAspectRatio()
    {
        Config::Config config = profiledConfig();

        const auto columnWidth = [](double view, double proportion) { return proportion * (view - 16.0) - 16.0; };

        Fixture narrow(config, QRectF(0, 0, 1920, 1080));
        const auto onNarrow = narrow.add();
        QCOMPARE(narrow.frame(onNarrow).width(), columnWidth(1920.0, 0.5));
        VERIFY_INVARIANTS(narrow);

        Fixture ultrawide(config, QRectF(0, 0, 5120, 1440));
        const auto onWide = ultrawide.add();
        QCOMPARE(ultrawide.frame(onWide).width(), columnWidth(5120.0, 0.25));
        VERIFY_INVARIANTS(ultrawide);
    }

    void columnPositionStartKeepsWindowLeftmost()
    {
        Config::Config config = LayoutTest::instantConfig();
        Config::WindowRule rule;
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^pinned$"));
        rule.matches.append(match);
        rule.columnPosition = Config::ColumnPosition::Start;
        config.windowRules.append(rule);

        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("other"));
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        QCOMPARE(fixture.state(pinned).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 1);

        const auto third = fixture.add(QStringLiteral("another"));
        QCOMPARE(fixture.state(pinned).columnIndex, 0);

        fixture.engine().activateWindow(pinned);
        fixture.perform(QStringLiteral("move-column-right"));
        QCOMPARE(fixture.state(pinned).columnIndex, 0);
        QVERIFY(fixture.state(third).columnIndex > 0);
        VERIFY_INVARIANTS(fixture);
    }

    void emptyMatchMatchesEverything() { QVERIFY(matchApplies(Config::Match(), context(QStringLiteral("a"), QStringLiteral("b")), false)); }

    void appIdRegexMatches()
    {
        const Config::Match match = appIdMatch(QStringLiteral("^fire"));
        QVERIFY(matchApplies(match, context(QStringLiteral("firefox"), QString()), false));
        QVERIFY(!matchApplies(match, context(QStringLiteral("chromium"), QString()), false));
    }

    void titleRegexMatches()
    {
        Config::Match match;
        match.title = QRegularExpression(QStringLiteral("Private"));
        QVERIFY(matchApplies(match, context(QString(), QStringLiteral("Private Browsing")), false));
        QVERIFY(!matchApplies(match, context(QString(), QStringLiteral("Normal")), false));
    }

    void atStartupIsHonored()
    {
        Config::Match match;
        match.atStartup = true;
        QVERIFY(matchApplies(match, context(QString(), QString()), true));
        QVERIFY(!matchApplies(match, context(QString(), QString()), false));
    }

    void flagsAreHonored()
    {
        Config::Match match;
        match.isFloating = true;
        MatchContext floating = context(QStringLiteral("a"), QString());
        floating.isFloating = true;
        QVERIFY(matchApplies(match, floating, false));
        QVERIFY(!matchApplies(match, context(QStringLiteral("a"), QString()), false));
    }

    void excludesTakePrecedence()
    {
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^fire")));
        rule.excludes.append(appIdMatch(QStringLiteral("dev$")));
        rule.opacity = 0.5;
        const EffectiveWindowRules matched = resolveWindowRules({rule}, context(QStringLiteral("firefox"), QString()), false);
        QCOMPARE(matched.opacity.value_or(1.0), 0.5);
        const EffectiveWindowRules excluded = resolveWindowRules({rule}, context(QStringLiteral("firefoxdev"), QString()), false);
        QCOMPARE(excluded.opacity.value_or(1.0), 1.0);
    }

    void laterRulesOverrideEarlier()
    {
        Config::WindowRule first;
        first.opacity = 0.5;
        Config::WindowRule second;
        second.opacity = 0.8;
        const EffectiveWindowRules resolved = resolveWindowRules({first, second}, context(QString(), QString()), false);
        QCOMPARE(resolved.opacity.value_or(1.0), 0.8);
    }

    void minMaxSizeRulesAreApplied()
    {
        EffectiveWindowRules rules;
        rules.minWidth = 300;
        rules.maxWidth = 800;
        QCOMPARE(rules.limitMinSize(QSize(100, 100)), QSize(300, 100));
        QCOMPARE(rules.limitMaxSize(QSize(0, 0)), QSize(800, 0));
        QCOMPARE(rules.limitMaxSize(QSize(1000, 0)), QSize(800, 0));
    }

    void borderRulesAreMerged()
    {
        Config::Border border;
        border.enabled = false;
        border.width = 4;
        Config::BorderRule rule;
        rule.enabled = true;
        rule.width = 10;
        const Config::Border merged = Konveyor::Layout::mergeBorder(border, rule);
        QVERIFY(merged.enabled);
        QCOMPARE(merged.width, 10.0);
    }

    void opacityRuleReachesWindowState()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^dim$")));
        rule.opacity = 0.4;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("dim"));
        QCOMPARE(fixture.state(id).ruleOpacity, 0.4);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-rule-opacity")).ok);
        QCOMPARE(fixture.state(id).ruleOpacity, 1.0);
        VERIFY_INVARIANTS(fixture);
    }

    void borderRuleReachesWindowState()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^thick$")));
        rule.border.width = 12;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("thick"));
        QVERIFY(fixture.state(id).border.enabled);
        QCOMPARE(fixture.state(id).border.width, 12.0);
        VERIFY_INVARIANTS(fixture);
    }

    void cornerRadiusAndClippingReachWindowState()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.geometryCornerRadius = Config::CornerRadius {8, 8, 8, 8};
        rule.clipToGeometry = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("round"));
        QCOMPARE(fixture.state(id).cornerRadius.topLeft, 8.0);
        QVERIFY(fixture.state(id).clipToGeometry);
        VERIFY_INVARIANTS(fixture);
    }

    void accentColorSourceIsPassedThrough()
    {
        Config::Config config = instantConfig();
        config.layout.focusRing.enabled = true;
        config.layout.focusRing.active.source = Config::ColorSource::SystemAccent;
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.state(id).focusRing.paint.source, Config::ColorSource::SystemAccent);
        VERIFY_INVARIANTS(fixture);
    }

    void urgentWindowUsesUrgentPaint()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.urgent.color = QColor(255, 0, 0);
        config.layout.border.active.color = QColor(0, 255, 0);
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        fixture.engine().setWindowUrgent(first, true);
        fixture.settle();
        QCOMPARE(fixture.state(first).border.paint.color, QColor(255, 0, 0));
        QCOMPARE(fixture.state(second).border.paint.color, QColor(0, 255, 0));
        VERIFY_INVARIANTS(fixture);
    }

    void openOnWorkspaceRule()
    {
        Config::Config config = instantConfig();
        Config::NamedWorkspace named;
        named.name = QStringLiteral("chat");
        config.workspaces.append(named);
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^messenger$")));
        rule.openOnWorkspace = QStringLiteral("chat");
        config.windowRules.append(rule);
        Fixture fixture(config);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto id = fixture.add(QStringLiteral("messenger"));
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void openOnOutputRule()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^second$")));
        rule.openOnOutput = QStringLiteral("DP-2");
        config.windowRules.append(rule);
        Fixture fixture(config);
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        const auto id = fixture.add(QStringLiteral("second"));
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void defaultColumnWidthRuleOverridesGlobal()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^wide$")));
        rule.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Fixed {700});
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("wide"));
        QCOMPARE(fixture.frame(id).width(), 700.0);
        VERIFY_INVARIANTS(fixture);
    }

    void emptyDefaultColumnWidthRuleKeepsWindowSize()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^own$")));
        rule.defaultColumnWidth = std::optional<Config::PresetSize>();
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("own"), QSizeF(345, 200));
        QCOMPARE(fixture.frame(id).width(), 345.0);
        VERIFY_INVARIANTS(fixture);
    }

    void openFocusedFalseKeepsFocus()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.matches.append(appIdMatch(QStringLiteral("^background$")));
        rule.openFocused = false;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("normal"));
        fixture.add(QStringLiteral("background"));
        QCOMPARE(fixture.focused(), first);
        VERIFY_INVARIANTS(fixture);
    }

    void liveConfigChangeReresolvesRules()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(id).ruleOpacity, 1.0);

        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.opacity = 0.25;
        config.windowRules.append(rule);
        fixture.setConfig(config);
        fixture.settle();
        QCOMPARE(fixture.state(id).ruleOpacity, 0.25);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWindowRules)
#include "test_layout_windowrules.moc"
