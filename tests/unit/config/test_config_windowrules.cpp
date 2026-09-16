#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigWindowRules : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesWindowRuleMatchers();
    void parsesWindowRuleProperties();
    void parsesGeometryCornerRadius();
    void parsesFloatingPosition();
};

void TestConfigWindowRules::parsesWindowRuleMatchers()
{
    const Config config = parsed(QStringLiteral(R"(
        window-rule {
            match app-id="^firefox$" title="Picture-in-Picture"
            match is-active=true is-focused=false is-active-in-column=true
            exclude is-floating=true is-urgent=false at-startup=true is-window-cast-target=false
            open-floating true
            manage false
        }
    )"));
    const WindowRule &rule = config.windowRules.first();
    QCOMPARE(rule.matches.size(), 2);
    QCOMPARE(rule.excludes.size(), 1);
    QVERIFY(rule.matches.first().appId.has_value());
    QCOMPARE(rule.matches.first().appId->pattern(), QStringLiteral("^firefox$"));
    QCOMPARE(rule.matches.first().title->pattern(), QStringLiteral("Picture-in-Picture"));
    QCOMPARE(rule.matches.at(1).isActive, std::optional {true});
    QCOMPARE(rule.matches.at(1).isFocused, std::optional {false});
    QCOMPARE(rule.matches.at(1).isActiveInColumn, std::optional {true});
    QCOMPARE(rule.excludes.first().isFloating, std::optional {true});
    QCOMPARE(rule.excludes.first().atStartup, std::optional {true});
    QCOMPARE(rule.openFloating, std::optional {true});
    QCOMPARE(rule.manage, std::optional {false});
    QVERIFY(mustFail(QStringLiteral("window-rule {\n match app-id=\"(unclosed\"\n}\n")).message.contains(QStringLiteral("invalid regex")));
}

void TestConfigWindowRules::parsesWindowRuleProperties()
{
    const Config config = parsed(QStringLiteral(R"(
        window-rule {
            default-column-width { proportion 0.75; }
            default-window-height {}
            open-on-output "eDP-1"
            open-on-workspace "chat"
            open-maximized true
            open-maximized-to-edges false
            open-fullscreen true
            open-focused true
            on-xdg-activate "set-urgent"
            min-width 100
            max-width 900
            min-height 50
            max-height 800
            opacity 0.9
            scroll-factor 1.5
            clip-to-geometry true
            draw-border-with-background false
            baba-is-float true
            variable-refresh-rate true
            tiled-state false
            block-out-from "screen-capture"
            default-column-display "tabbed"
            focus-ring { off; }
            border { on; width 8.5; }
            shadow { on; softness 12; }
            tab-indicator { active-color "#f00"; }
        }
    )"));
    const WindowRule &rule = config.windowRules.first();
    QCOMPARE(proportionOf(**rule.defaultColumnWidth), 0.75);
    QVERIFY(rule.defaultWindowHeight.has_value());
    QVERIFY(!rule.defaultWindowHeight->has_value());
    QCOMPARE(rule.openOnOutput, std::optional {QStringLiteral("eDP-1")});
    QCOMPARE(rule.openOnWorkspace, std::optional {QStringLiteral("chat")});
    QCOMPARE(rule.openMaximized, std::optional {true});
    QCOMPARE(rule.openMaximizedToEdges, std::optional {false});
    QCOMPARE(rule.openFullscreen, std::optional {true});
    QCOMPARE(rule.onXdgActivate, std::optional {XdgActivate::SetUrgent});
    QCOMPARE(rule.minWidth, std::optional {100});
    QCOMPARE(rule.maxHeight, std::optional {800});
    QCOMPARE(rule.opacity, std::optional {0.9});
    QCOMPARE(rule.scrollFactor, std::optional {1.5});
    QCOMPARE(rule.clipToGeometry, std::optional {true});
    QCOMPARE(rule.drawBorderWithBackground, std::optional {false});
    QCOMPARE(rule.babaIsFloat, std::optional {true});
    QCOMPARE(rule.variableRefreshRate, std::optional {true});
    QCOMPARE(rule.tiledState, std::optional {false});
    QCOMPARE(rule.blockOutFrom, std::optional {BlockOutFrom::ScreenCapture});
    QCOMPARE(rule.blockOutFromScreencast, std::optional {true});
    QCOMPARE(rule.defaultColumnDisplay, std::optional {ColumnDisplay::Tabbed});
    QCOMPARE(rule.focusRing.enabled, std::optional {false});
    QCOMPARE(rule.border.enabled, std::optional {true});
    QCOMPARE(rule.border.width, std::optional {8.5});
    QCOMPARE(rule.shadow, std::optional {true});
    QCOMPARE(rule.shadowRule.softness, std::optional {12.0});
    QVERIFY(rule.tabIndicator.active.has_value());
    QCOMPARE(rule.tabIndicator.active->color, QColor(255, 0, 0));
}

void TestConfigWindowRules::parsesGeometryCornerRadius()
{
    const Config single = parsed(QStringLiteral("window-rule {\n geometry-corner-radius 12\n}\n"));
    QCOMPARE(*single.windowRules.first().geometryCornerRadius, (CornerRadius {12, 12, 12, 12}));

    const Config four = parsed(QStringLiteral("window-rule {\n geometry-corner-radius 1 2 3 4\n}\n"));
    QCOMPARE(*four.windowRules.first().geometryCornerRadius, (CornerRadius {1, 2, 3, 4}));

    QVERIFY(mustFail(QStringLiteral("window-rule {\n geometry-corner-radius 1 2\n}\n"))
            .message.contains(QStringLiteral("either 1 or 4 arguments")));
    QVERIFY(
        mustFail(QStringLiteral("window-rule {\n geometry-corner-radius -1\n}\n")).message.contains(QStringLiteral("between 0 and 65535")));
}

void TestConfigWindowRules::parsesFloatingPosition()
{
    const Config config = parsed(QStringLiteral("window-rule {\n default-floating-position x=100 y=-200 relative-to=\"bottom-left\"\n}\n"));
    const FloatingPosition position = *config.windowRules.first().defaultFloatingPosition;
    QCOMPARE(position.x, 100.0);
    QCOMPARE(position.y, -200.0);
    QCOMPARE(position.relativeTo, FloatingRelativeTo::BottomLeft);

    const Config plain = parsed(QStringLiteral("window-rule {\n default-floating-position x=1 y=2\n}\n"));
    QCOMPARE(plain.windowRules.first().defaultFloatingPosition->relativeTo, FloatingRelativeTo::TopLeft);
    QVERIFY(
        mustFail(QStringLiteral("window-rule {\n default-floating-position x=1\n}\n")).message.contains(QStringLiteral("`y` is required")));
}

QTEST_MAIN(TestConfigWindowRules)
#include "test_config_windowrules.moc"
