#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

WindowRule ruleOf(const QString &body)
{
    return parsed(QStringLiteral("window-rule {\n    %1\n}\n").arg(body)).windowRules.value(0);
}

QString inRule(const QString &body)
{
    return QStringLiteral("window-rule {\n    %1\n}\n").arg(body);
}

}

class TestConfigRuleNodes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyRuleSetsNothing();
    void columnPosition();
    void columnPositionRejects_data();
    void columnPositionRejects();
    void sizeLimits();
    void matchProperties();
    void matchKeepsOrder();
    void monitorProfileMatch();
    void outputMatch();
    void placementNodes();
    void floatingAnchors_data();
    void floatingAnchors();
    void floatingPositionRejects_data();
    void floatingPositionRejects();
    void keywordNodes();
    void keywordNodesReject_data();
    void keywordNodesReject();
    void defaultWindowHeight();
    void ruleDecorations();
    void rejectsDuplicateRuleNodes();
};

void TestConfigRuleNodes::emptyRuleSetsNothing()
{
    const Config config = parsed(QStringLiteral("window-rule {}\nwindow-rule {}\n"));
    QCOMPARE(config.windowRules.size(), 2);
    QCOMPARE(config.windowRules.first(), WindowRule {});
}

void TestConfigRuleNodes::columnPosition()
{
    QCOMPARE(ruleOf(QString()).columnPosition, std::nullopt);
    QCOMPARE(ruleOf(QStringLiteral("column-position \"start\"")).columnPosition, std::optional(ColumnPosition::Start));
    QCOMPARE(ruleOf(QStringLiteral("column-position \"end\"")).columnPosition, std::optional(ColumnPosition::End));
}

void TestConfigRuleNodes::columnPositionRejects_data()
{
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");

    QTest::newRow("extra argument") << inRule(QStringLiteral("column-position \"start\" »\"x\"")) << QStringLiteral("unexpected argument");
    QTest::newRow("children") << inRule(QStringLiteral("column-position \"end\" { »bogus; }")) << QStringLiteral("unexpected node `bogus`");
    QTest::newRow("property") << inRule(QStringLiteral("column-position \"end\" »at=1")) << QStringLiteral("unexpected property `at`");
    QTest::newRow("unknown") << inRule(QStringLiteral("column-position »\"middle\"")) << QStringLiteral("expected one of `start`, `end`");
    QTest::newRow("number") << inRule(QStringLiteral("column-position »1")) << QStringLiteral("expected a string");
    QTest::newRow("missing") << inRule(QStringLiteral("»column-position"))
                             << QStringLiteral("additional argument `column-position` is required");
}

void TestConfigRuleNodes::columnPositionRejects()
{
    QFETCH(QString, marked);
    QFETCH(QString, message);
    verifyFailure(marked, message);
}

void TestConfigRuleNodes::sizeLimits()
{
    const WindowRule rule = ruleOf(QStringLiteral("min-width 10; min-height 20; max-width 30; max-height 40"));
    QCOMPARE(rule.minWidth, std::optional(10));
    QCOMPARE(rule.minHeight, std::optional(20));
    QCOMPARE(rule.maxWidth, std::optional(30));
    QCOMPARE(rule.maxHeight, std::optional(40));
    QCOMPARE(ruleOf(QStringLiteral("min-height 5")).maxHeight, std::nullopt);
}

void TestConfigRuleNodes::matchProperties()
{
    const WindowRule rule = ruleOf(QStringLiteral("match is-urgent=true is-active-in-column=false is-focused=true is-active=false "
                                                  "is-floating=true at-startup=false title=\"^Edit\" app-id=\"x\""));
    const Match &match = rule.matches.first();
    QCOMPARE(match.isUrgent, std::optional(true));
    QCOMPARE(match.isActiveInColumn, std::optional(false));
    QCOMPARE(match.isFocused, std::optional(true));
    QCOMPARE(match.isActive, std::optional(false));
    QCOMPARE(match.isFloating, std::optional(true));
    QCOMPARE(match.atStartup, std::optional(false));
    QCOMPARE(match.title->pattern(), QStringLiteral("^Edit"));
    QCOMPARE(match.appId->pattern(), QStringLiteral("x"));
    QCOMPARE(match.monitorProfile, std::nullopt);
    QCOMPARE(ruleOf(QStringLiteral("match")).matches.first(), Match {});
    verifyFailure(inRule(QStringLiteral("match »\"firefox\"")), QStringLiteral("no arguments expected for this node"));
    verifyFailure(inRule(QStringLiteral("exclude { »app-id; }")), QStringLiteral("unexpected node `app-id`"));
    verifyFailure(inRule(QStringLiteral("match title=»\"[\"")), QStringLiteral("invalid regex: missing terminating ] for character class"));
    verifyFailure(inRule(QStringLiteral("match app-id=»1")), QStringLiteral("expected a string"));
}

void TestConfigRuleNodes::matchKeepsOrder()
{
    const WindowRule rule = ruleOf(QStringLiteral("match app-id=\"a\"; exclude title=\"b\"; match app-id=\"c\"; exclude is-urgent=true"));
    QCOMPARE(rule.matches.size(), 2);
    QCOMPARE(rule.matches.at(1).appId->pattern(), QStringLiteral("c"));
    QCOMPARE(rule.excludes.size(), 2);
    QCOMPARE(rule.excludes.at(0).title->pattern(), QStringLiteral("b"));
    QCOMPARE(rule.excludes.at(1).isUrgent, std::optional(true));
}

void TestConfigRuleNodes::monitorProfileMatch()
{
    const WindowRule rule = ruleOf(QStringLiteral("match app-id=\"^chrome$\" monitor-profile=\"^ultra\""));
    QVERIFY(rule.matches.first().monitorProfile.has_value());
    QVERIFY(rule.matches.first().monitorProfile->match(QStringLiteral("ultrawide")).hasMatch());
    QVERIFY(!rule.matches.first().monitorProfile->match(QStringLiteral("portrait")).hasMatch());
    QVERIFY(ruleOf(QStringLiteral("exclude monitor-profile=\"portrait\"")).excludes.first().monitorProfile.has_value());
    verifyFailure(inRule(QStringLiteral("match monitor-profile=»\"(\"")), QStringLiteral("invalid regex: missing closing parenthesis"));
    verifyFailure(inRule(QStringLiteral("match monitor-profile=»true")), QStringLiteral("expected a string"));
}

void TestConfigRuleNodes::outputMatch()
{
    const WindowRule rule = ruleOf(QStringLiteral("match app-id=\"^kitty$\" output=\"^DP-1$\""));
    QVERIFY(rule.matches.first().output->match(QStringLiteral("DP-1")).hasMatch());
    QVERIFY(!rule.matches.first().output->match(QStringLiteral("DP-12")).hasMatch());
    QVERIFY(ruleOf(QStringLiteral("exclude output=\"HDMI\"")).excludes.first().output.has_value());
    verifyFailure(inRule(QStringLiteral("match output=»2")), QStringLiteral("expected a string"));
}

void TestConfigRuleNodes::placementNodes()
{
    const WindowRule placed = ruleOf(QStringLiteral("open-on-workspace 2; open-at-column 3; open-on-all-workspaces true"));
    QCOMPARE(placed.openOnWorkspaceIndex, std::optional(2));
    QCOMPARE(placed.openOnWorkspace, std::nullopt);
    QCOMPARE(placed.openAtColumn, std::optional(3));
    QCOMPARE(placed.openOnAllWorkspaces, std::optional(true));
    QCOMPARE(ruleOf(QStringLiteral("open-on-workspace \"2\"")).openOnWorkspace, std::optional(QStringLiteral("2")));
    QCOMPARE(ruleOf(QStringLiteral("open-on-workspace \"2\"")).openOnWorkspaceIndex, std::nullopt);
    verifyFailure(inRule(QStringLiteral("open-at-column »0")), QStringLiteral("value must be between 1 and 999"));
    verifyFailure(inRule(QStringLiteral("open-on-workspace »0")), QStringLiteral("value must be between 1 and 999"));
    verifyFailure(inRule(QStringLiteral("open-on-all-workspaces »\"yes\"")), QStringLiteral("expected a boolean"));
}

void TestConfigRuleNodes::floatingAnchors_data()
{
    QTest::addColumn<QString>("anchor");
    QTest::addColumn<FloatingRelativeTo>("expected");

    QTest::newRow("top-left") << QStringLiteral("top-left") << FloatingRelativeTo::TopLeft;
    QTest::newRow("top-right") << QStringLiteral("top-right") << FloatingRelativeTo::TopRight;
    QTest::newRow("bottom-left") << QStringLiteral("bottom-left") << FloatingRelativeTo::BottomLeft;
    QTest::newRow("bottom-right") << QStringLiteral("bottom-right") << FloatingRelativeTo::BottomRight;
    QTest::newRow("top") << QStringLiteral("top") << FloatingRelativeTo::Top;
    QTest::newRow("bottom") << QStringLiteral("bottom") << FloatingRelativeTo::Bottom;
    QTest::newRow("left") << QStringLiteral("left") << FloatingRelativeTo::Left;
    QTest::newRow("right") << QStringLiteral("right") << FloatingRelativeTo::Right;
}

void TestConfigRuleNodes::floatingAnchors()
{
    QFETCH(QString, anchor);
    QFETCH(FloatingRelativeTo, expected);
    const WindowRule rule = ruleOf(QStringLiteral("default-floating-position x=5 y=-6 relative-to=\"%1\"").arg(anchor));
    QCOMPARE(*rule.defaultFloatingPosition, (FloatingPosition {5, -6, expected}));
}

void TestConfigRuleNodes::floatingPositionRejects_data()
{
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");

    QTest::newRow("unknown anchor") << inRule(QStringLiteral("default-floating-position x=0 y=0 relative-to=»\"center\""))
                                    << QStringLiteral("expected one of `top-left`, `top-right`, `bottom-left`, `bottom-right`, `top`, "
                                                      "`bottom`, `left`, `right`");
    QTest::newRow("anchor case") << inRule(QStringLiteral("default-floating-position x=0 y=0 relative-to=»\"Top\""))
                                 << QStringLiteral("expected one of `top-left`, `top-right`, `bottom-left`, `bottom-right`, `top`, "
                                                   "`bottom`, `left`, `right`");
    QTest::newRow("anchor number") << inRule(QStringLiteral("default-floating-position x=0 y=0 relative-to=»1"))
                                   << QStringLiteral("expected a string");
    QTest::newRow("missing x") << inRule(QStringLiteral("»default-floating-position y=0")) << QStringLiteral("property `x` is required");
    QTest::newRow("argument") << inRule(QStringLiteral("default-floating-position »1 x=0 y=0"))
                              << QStringLiteral("no arguments expected for this node");
    QTest::newRow("unknown property") << inRule(QStringLiteral("default-floating-position x=0 y=0 »z=1"))
                                      << QStringLiteral("unexpected property `z`");
    QTest::newRow("string x") << inRule(QStringLiteral("default-floating-position x=»\"1\" y=0"))
                              << QStringLiteral("unsupported value, only numbers are recognized");
}

void TestConfigRuleNodes::floatingPositionRejects()
{
    QFETCH(QString, marked);
    QFETCH(QString, message);
    verifyFailure(marked, message);
}

void TestConfigRuleNodes::keywordNodes()
{
    QCOMPARE(ruleOf(QStringLiteral("group-app-windows \"off\"")).groupAppWindows, std::optional(GroupAppWindows::Off));
    QCOMPARE(ruleOf(QStringLiteral("group-app-windows \"beside\"")).groupAppWindows, std::optional(GroupAppWindows::Beside));
    QCOMPARE(ruleOf(QStringLiteral("new-window-placement \"stack\"")).newWindowPlacement, std::optional(NewWindowPlacement::Stack));
    QCOMPARE(ruleOf(QStringLiteral("default-column-display \"normal\"")).defaultColumnDisplay, std::optional(ColumnDisplay::Normal));
    QCOMPARE(ruleOf(QStringLiteral("open-on-output \"HDMI-A-1\"")).openOnOutput, std::optional(QStringLiteral("HDMI-A-1")));
    QCOMPARE(ruleOf(QStringLiteral("open-on-workspace \"chat\"")).openOnWorkspace, std::optional(QStringLiteral("chat")));
}

void TestConfigRuleNodes::keywordNodesReject_data()
{
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");

    QTest::newRow("group") << inRule(QStringLiteral("group-app-windows »\"tabbed\""))
                           << QStringLiteral("expected one of `off`, `beside`, `stack`");
    QTest::newRow("placement") << inRule(QStringLiteral("new-window-placement »\"row\""))
                               << QStringLiteral("expected one of `column`, `stack`");
    QTest::newRow("display") << inRule(QStringLiteral("default-column-display »\"tabs\""))
                             << QStringLiteral("expected one of `normal`, `tabbed`");
    QTest::newRow("output number") << inRule(QStringLiteral("open-on-output »1")) << QStringLiteral("expected a string");
    QTest::newRow("workspace bare") << inRule(QStringLiteral("»open-on-workspace"))
                                    << QStringLiteral("additional argument `open-on-workspace` is required");
    QTest::newRow("output two") << inRule(QStringLiteral("open-on-output \"a\" »\"b\"")) << QStringLiteral("unexpected argument");
}

void TestConfigRuleNodes::keywordNodesReject()
{
    QFETCH(QString, marked);
    QFETCH(QString, message);
    verifyFailure(marked, message);
}

void TestConfigRuleNodes::defaultWindowHeight()
{
    QCOMPARE(ruleOf(QString()).defaultWindowHeight, std::nullopt);
    QCOMPARE(fixedOf(**ruleOf(QStringLiteral("default-window-height { fixed 300; }")).defaultWindowHeight), 300.0);
    QCOMPARE(proportionOf(**ruleOf(QStringLiteral("default-window-height { proportion 0.4; }")).defaultWindowHeight), 0.4);
    verifyFailure(
        inRule(QStringLiteral("default-window-height { fixed 1; »fixed 2; }")), QStringLiteral("expected no more than one child"));
    verifyFailure(inRule(QStringLiteral("default-window-height { »percent 50; }")), QStringLiteral("unexpected node `percent`"));
    verifyFailure(inRule(QStringLiteral("default-column-width »1 {}")), QStringLiteral("no arguments expected for this node"));
}

void TestConfigRuleNodes::ruleDecorations()
{
    const WindowRule rule
        = ruleOf(QStringLiteral("focus-ring { width 2; active-color \"accent\"; urgent-gradient from=\"red\" to=\"blue\"; }; "
                                "border { off; inactive-color \"#123\"; }"));
    QCOMPARE(rule.focusRing.enabled, std::nullopt);
    QCOMPARE(rule.focusRing.width, std::optional(2.0));
    QCOMPARE(rule.focusRing.active->source, ColorSource::SystemAccent);
    QVERIFY(rule.focusRing.urgent->gradient.has_value());
    QCOMPARE(rule.border.enabled, std::optional(false));
    QCOMPARE(rule.border.inactive->color, QColor(0x11, 0x22, 0x33));
    QCOMPARE(ruleOf(QString()).border, BorderRule {});
}

void TestConfigRuleNodes::rejectsDuplicateRuleNodes()
{
    verifyFailure(inRule(QStringLiteral("opacity 0.5; »opacity 0.6")), QStringLiteral("duplicate node `opacity`, single node expected"));
    verifyFailure(inRule(QStringLiteral("column-position \"start\"; »column-position \"end\"")),
        QStringLiteral("duplicate node `column-position`, single node expected"));
    verifyFailure(inRule(QStringLiteral("border {}; »border {}")), QStringLiteral("duplicate node `border`, single node expected"));
}

QTEST_MAIN(TestConfigRuleNodes)
#include "test_config_rulenodes.moc"
