#include "helpers.h"

#include "layout/rules/windowrules.h"

using namespace LayoutTest;

namespace
{

Layout::MatchContext busyContext()
{
    Layout::MatchContext context;
    context.appId = QStringLiteral("app");
    context.title = QStringLiteral("title");
    context.monitorProfile = QStringLiteral("profile");
    context.output = QStringLiteral("DP-1");
    context.isActive = true;
    context.isFocused = true;
    context.isActiveInColumn = true;
    context.isFloating = true;
    context.isUrgent = true;
    return context;
}

}

class TestLayoutRuleMatching : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everyMatchFlagMustAgree()
    {
        const Layout::MatchContext context = busyContext();
        Config::Match all;
        all.isActive = true;
        all.isFocused = true;
        all.isActiveInColumn = true;
        all.isFloating = true;
        all.isUrgent = true;
        all.atStartup = false;
        QVERIFY(Layout::matchApplies(all, context, false));

        Config::Match mismatch = all;
        mismatch.atStartup = true;
        QVERIFY(!Layout::matchApplies(mismatch, context, false));
        mismatch = all;
        mismatch.isFocused = false;
        QVERIFY(!Layout::matchApplies(mismatch, context, false));
        mismatch = all;
        mismatch.isUrgent = false;
        QVERIFY(!Layout::matchApplies(mismatch, context, false));
        mismatch = all;
        mismatch.isActive = false;
        QVERIFY(!Layout::matchApplies(mismatch, context, false));
        mismatch = all;
        mismatch.isActiveInColumn = false;
        QVERIFY(!Layout::matchApplies(mismatch, context, false));
        mismatch = all;
        mismatch.isFloating = false;
        QVERIFY(!Layout::matchApplies(mismatch, context, false));
    }

    void patternsMatchTheirOwnFields()
    {
        const Layout::MatchContext context = busyContext();
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^app$"));
        match.title = QRegularExpression(QStringLiteral("^title$"));
        match.monitorProfile = QRegularExpression(QStringLiteral("^profile$"));
        match.output = QRegularExpression(QStringLiteral("^DP-1$"));
        QVERIFY(Layout::matchApplies(match, context, false));
        Layout::MatchContext unset = context;
        unset.title = QString();
        QVERIFY(!Layout::matchApplies(match, unset, false));
        unset = context;
        unset.monitorProfile = QString();
        QVERIFY(!Layout::matchApplies(match, unset, false));
        unset = context;
        unset.output = QStringLiteral("HDMI-1");
        QVERIFY(!Layout::matchApplies(match, unset, false));
    }

    void excludesRemoveMatchingRules()
    {
        Config::WindowRule rule;
        Config::Match exclude;
        exclude.appId = QRegularExpression(QStringLiteral("^app$"));
        rule.excludes.append(exclude);
        rule.openFloating = true;
        QVERIFY(!Layout::ruleApplies(rule, busyContext(), false));
        Layout::MatchContext other = busyContext();
        other.appId = QStringLiteral("other");
        QVERIFY(Layout::ruleApplies(rule, other, false));
        Config::Match unrelated;
        unrelated.appId = QRegularExpression(QStringLiteral("^nothing$"));
        rule.matches.append(unrelated);
        QVERIFY(!Layout::ruleApplies(rule, other, false));
    }

    void laterRulesOverrideEarlierOnes()
    {
        Config::WindowRule first;
        first.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Proportion {0.4});
        first.defaultWindowHeight = std::optional<Config::PresetSize>(Config::Fixed {300});
        first.openOnOutput = QStringLiteral("DP-1");
        first.openOnWorkspace = QStringLiteral("web");
        first.openOnWorkspaceIndex = 2;
        first.openAtColumn = 3;
        first.openOnAllWorkspaces = true;
        first.openMaximized = true;
        first.openMaximizedToEdges = true;
        first.openFullscreen = true;
        first.openFloating = true;
        first.openFocused = true;
        first.manage = false;
        first.columnPosition = Config::ColumnPosition::End;
        first.groupAppWindows = Config::GroupAppWindows::Stack;
        first.newWindowPlacement = Config::NewWindowPlacement::Stack;
        first.maxRowsPerColumn = 3;
        first.floatChildWindows = true;
        first.forceResizable = true;
        first.opacity = 0.5;
        first.minWidth = 100;
        first.maxWidth = 900;
        first.minHeight = 50;
        first.maxHeight = 800;
        first.geometryCornerRadius = Config::CornerRadius {1, 2, 3, 4};
        first.clipToGeometry = true;
        first.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
        first.defaultFloatingPosition = Config::FloatingPosition {10, 20, Config::FloatingRelativeTo::BottomRight};
        first.focusRing.enabled = true;
        first.focusRing.width = 6.0;
        first.focusRing.active = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::red), std::nullopt};
        first.focusRing.inactive = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::green), std::nullopt};
        first.focusRing.urgent = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::blue), std::nullopt};
        first.focusRing.activeStyle = QStringLiteral("glow");
        first.focusRing.inactiveStyle = QStringLiteral("pulse");
        first.focusRing.urgentStyle = QStringLiteral("flash");
        first.border = first.focusRing;

        Config::WindowRule second;
        second.openFloating = false;
        second.focusRing.activeStyle = QStringLiteral("solid");

        const Layout::EffectiveWindowRules resolved = Layout::resolveWindowRules({first, second}, busyContext(), false);
        QCOMPARE(resolved.openFloating, std::optional(false));
        QCOMPARE(resolved.openOnWorkspace, std::optional(QStringLiteral("web")));
        QCOMPARE(resolved.openOnWorkspaceIndex, std::optional(2));
        QCOMPARE(resolved.openAtColumn, std::optional(3));
        QCOMPARE(resolved.columnPosition, std::optional(Config::ColumnPosition::End));
        QCOMPARE(resolved.maxRowsPerColumn, std::optional(3));
        QCOMPARE(resolved.opacity, std::optional(0.5));
        QCOMPARE(resolved.geometryCornerRadius, std::optional(Config::CornerRadius {1, 2, 3, 4}));
        QCOMPARE(resolved.defaultColumnDisplay, std::optional(Config::ColumnDisplay::Tabbed));
        QCOMPARE(resolved.focusRing.activeStyle, QStringLiteral("solid"));
        QCOMPARE(resolved.focusRing.inactiveStyle, QStringLiteral("pulse"));
        QCOMPARE(resolved.focusRing.urgentStyle, QStringLiteral("flash"));
        QCOMPARE(resolved.border.activeStyle, QStringLiteral("glow"));
        QVERIFY(resolved.focusRing.enabled.value_or(false));

        Config::Border base;
        const Config::Border merged = Layout::mergeBorder(base, resolved.focusRing);
        QVERIFY(merged.enabled);
        QCOMPARE(merged.width, 6.0);
        QCOMPARE(resolved.limitMinSize(QSize(10, 10)), QSize(100, 50));
        QCOMPARE(resolved.limitMaxSize(QSize(0, 0)), QSize(900, 800));
        QCOMPARE(resolved.limitMaxSize(QSize(500, 1000)), QSize(500, 800));
    }

    void maxLimitsKeepSmallerWindowSizes()
    {
        Layout::EffectiveWindowRules rules;
        rules.maxWidth = 0;
        rules.maxHeight = 700;
        QCOMPARE(rules.limitMaxSize(QSize(300, 900)), QSize(300, 700));
        QCOMPARE(rules.limitMaxSize(QSize(300, 0)), QSize(300, 700));
        QCOMPARE(rules.limitMinSize(QSize(20, 20)), QSize(20, 20));
    }
};

QTEST_GUILESS_MAIN(TestLayoutRuleMatching)
#include "test_layout_rulematching.moc"
