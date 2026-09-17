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
    void rejectsRemovedRuleOptions();
};

void TestConfigWindowRules::parsesWindowRuleMatchers()
{
    const Config config = parsed(QStringLiteral(R"(
        window-rule {
            match app-id="^firefox$" title="Picture-in-Picture"
            match is-active=true is-focused=false is-active-in-column=true
            exclude is-floating=true is-urgent=false at-startup=true
            open-floating true
            manage false
            group-app-windows "stack"
            new-window-placement "column"
            max-rows-per-column 2
            float-child-windows true
        }
    )"));
    const WindowRule &rule = config.windowRules.first();
    QCOMPARE(rule.matches.size(), 2);
    QCOMPARE(rule.excludes.size(), 1);
    QVERIFY(rule.matches.first().appId.has_value());
    QCOMPARE(rule.matches.first().appId->pattern(), QStringLiteral("^firefox$"));
    QCOMPARE(rule.groupAppWindows, std::optional(GroupAppWindows::Stack));
    QCOMPARE(rule.newWindowPlacement, std::optional(NewWindowPlacement::Column));
    QCOMPARE(rule.maxRowsPerColumn, std::optional(2));
    QCOMPARE(rule.floatChildWindows, std::optional(true));
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
            min-width 100
            max-width 900
            min-height 50
            max-height 800
            opacity 0.9
            clip-to-geometry true
            default-column-display "tabbed"
            focus-ring { off; }
            border { on; width 8.5; }
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
    QCOMPARE(rule.minWidth, std::optional {100});
    QCOMPARE(rule.maxHeight, std::optional {800});
    QCOMPARE(rule.opacity, std::optional {0.9});
    QCOMPARE(rule.clipToGeometry, std::optional {true});
    QCOMPARE(rule.defaultColumnDisplay, std::optional {ColumnDisplay::Tabbed});
    QCOMPARE(rule.focusRing.enabled, std::optional {false});
    QCOMPARE(rule.border.enabled, std::optional {true});
    QCOMPARE(rule.border.width, std::optional {8.5});
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

void TestConfigWindowRules::rejectsRemovedRuleOptions()
{
    const QStringList nodes {QStringLiteral("on-xdg-activate \"focus\""), QStringLiteral("scroll-factor 1.5"),
        QStringLiteral("draw-border-with-background false"), QStringLiteral("baba-is-float true"),
        QStringLiteral("variable-refresh-rate true"), QStringLiteral("tiled-state false"), QStringLiteral("block-out-from \"screencast\""),
        QStringLiteral("shadow { on; }"), QStringLiteral("tab-indicator { active-color \"#f00\"; }"),
        QStringLiteral("popups { opacity 0.5; }"), QStringLiteral("background-effect { blur true; }")};
    for (const QString &node : nodes) {
        const QString text = QStringLiteral("window-rule {\n %1\n}\n").arg(node);
        QVERIFY2(mustFail(text).message.contains(QStringLiteral("unexpected node")), qPrintable(node));
    }
    QVERIFY(mustFail(QStringLiteral("window-rule {\n match is-window-cast-target=true\n}\n"))
            .message.contains(QStringLiteral("unexpected property")));
}

QTEST_MAIN(TestConfigWindowRules)
#include "test_config_windowrules.moc"
