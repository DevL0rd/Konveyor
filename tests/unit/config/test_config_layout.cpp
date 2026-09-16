#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigLayout : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesEveryLayoutSetting();
    void parsesPresetsAndDefaultWidth();
    void emptyBorderSectionEnablesBorder();
    void borderOffKeepsBorderDisabled();
    void shadowOffWinsOverOn();
    void parsesTabIndicatorAndInsertHint();
    void parsesStrutsPartially();
    void rejectsOutOfRangeValues_data();
    void rejectsOutOfRangeValues();
    void rejectsUnknownNodes_data();
    void rejectsUnknownNodes();
    void reportsAccurateLocations();
    void rejectsDuplicateSections();
};

void TestConfigLayout::parsesEveryLayoutSetting()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            gaps 8
            center-focused-column "on-overflow"
            new-column-position "left"
            always-center-single-column
            empty-workspace-above-first true
            default-column-display "tabbed"
            background-color "#123456"
            struts { left 1; right 2; top 3; bottom 4; }
        }
    )"));
    const Layout &layout = config.layout;
    QCOMPARE(layout.gaps, 8.0);
    QCOMPARE(layout.centerFocusedColumn, CenterFocusedColumn::OnOverflow);
    QCOMPARE(layout.newColumnPosition, NewColumnPosition::Left);
    QCOMPARE(layout.alwaysCenterSingleColumn, true);
    QCOMPARE(layout.emptyWorkspaceAboveFirst, true);
    QCOMPARE(layout.defaultColumnDisplay, ColumnDisplay::Tabbed);
    QCOMPARE(layout.backgroundColor, QColor(0x12, 0x34, 0x56));
    QCOMPARE(layout.struts, (Struts {1, 2, 3, 4}));
}

void TestConfigLayout::parsesPresetsAndDefaultWidth()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            preset-column-widths { proportion 0.25; fixed 960; }
            preset-window-heights { fixed 200; proportion 0.75; }
            default-column-width { fixed 500; }
        }
    )"));
    QCOMPARE(config.layout.presetColumnWidths.size(), 2);
    QCOMPARE(proportionOf(config.layout.presetColumnWidths.at(0)), 0.25);
    QCOMPARE(fixedOf(config.layout.presetColumnWidths.at(1)), 960.0);
    QCOMPARE(fixedOf(config.layout.presetWindowHeights.at(0)), 200.0);
    QVERIFY(config.layout.defaultColumnWidth.has_value());
    QCOMPARE(fixedOf(*config.layout.defaultColumnWidth), 500.0);

    const Config empty = parsed(QStringLiteral("layout { default-column-width {}; }"));
    QVERIFY(!empty.layout.defaultColumnWidth.has_value());
}

void TestConfigLayout::emptyBorderSectionEnablesBorder()
{
    QCOMPARE(parsed(QStringLiteral("layout { border {}; }")).layout.border.enabled, true);
    QCOMPARE(parsed(QStringLiteral("layout {}")).layout.border.enabled, false);
}

void TestConfigLayout::borderOffKeepsBorderDisabled()
{
    const Config config = parsed(QStringLiteral("layout { border { off; width 8; }; }"));
    QCOMPARE(config.layout.border.enabled, false);
    QCOMPARE(config.layout.border.width, 8.0);

    const Config ring = parsed(QStringLiteral("layout { focus-ring { off; }; }"));
    QCOMPARE(ring.layout.focusRing.enabled, false);
}

void TestConfigLayout::shadowOffWinsOverOn()
{
    QCOMPARE(parsed(QStringLiteral("layout { shadow { on; }; }")).layout.shadow.enabled, true);
    QCOMPARE(parsed(QStringLiteral("layout { shadow { on; off; }; }")).layout.shadow.enabled, false);
    const Config config = parsed(QStringLiteral(R"(
        layout {
            shadow {
                on
                offset x=10 y=-20
                softness 12
                spread 3
                draw-behind-window true
                color "#010203"
                inactive-color "#040506"
            }
        }
    )"));
    QCOMPARE(config.layout.shadow.offset, QPointF(10, -20));
    QCOMPARE(config.layout.shadow.softness, 12.0);
    QCOMPARE(config.layout.shadow.spread, 3.0);
    QCOMPARE(config.layout.shadow.drawBehindWindow, true);
    QCOMPARE(config.layout.shadow.color, QColor(1, 2, 3));
    QCOMPARE(config.layout.shadow.inactiveColor, std::optional {QColor(4, 5, 6)});
}

void TestConfigLayout::parsesTabIndicatorAndInsertHint()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            tab-indicator {
                hide-when-single-tab
                place-within-column
                gap 6
                width 10
                gaps-between-tabs 2
                corner-radius 3
                position "top"
                length total-proportion=0.25
                active-color "#ff0000"
            }
            insert-hint {
                color "#00ff00"
            }
        }
    )"));
    const TabIndicator &indicator = config.layout.tabIndicator;
    QCOMPARE(indicator.hideWhenSingleTab, true);
    QCOMPARE(indicator.placeWithinColumn, true);
    QCOMPARE(indicator.gap, 6.0);
    QCOMPARE(indicator.width, 10.0);
    QCOMPARE(indicator.gapsBetweenTabs, 2.0);
    QCOMPARE(indicator.cornerRadius, 3.0);
    QCOMPARE(indicator.position, TabIndicatorPosition::Top);
    QCOMPARE(indicator.lengthTotalProportion, 0.25);
    QVERIFY(indicator.active.has_value());
    QCOMPARE(indicator.active->color, QColor(255, 0, 0));
    QCOMPARE(config.layout.insertHint.paint.color, QColor(0, 255, 0));

    QCOMPARE(parsed(QStringLiteral("layout { insert-hint { off; }; }")).layout.insertHint.enabled, false);
    QCOMPARE(parsed(QStringLiteral("layout { tab-indicator { off; }; }")).layout.tabIndicator.enabled, false);
}

void TestConfigLayout::parsesStrutsPartially()
{
    const Config config = parsed(QStringLiteral("layout { struts { left 5; }; }"));
    QCOMPARE(config.layout.struts, (Struts {5, 0, 0, 0}));
}

void TestConfigLayout::rejectsOutOfRangeValues_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("fragment");

    QTest::newRow("negative gaps") << QStringLiteral("layout { gaps -1; }") << QStringLiteral("between 0 and 65535");
    QTest::newRow("huge width") << QStringLiteral("layout { border { width 70000; }; }") << QStringLiteral("between 0 and 65535");
    QTest::newRow("softness") << QStringLiteral("layout { shadow { softness 2000; }; }") << QStringLiteral("between 0 and 1024");
    QTest::newRow("zoom") << QStringLiteral("overview { zoom 4; }") << QStringLiteral("between 0 and 1");
    QTest::newRow("string gaps") << QStringLiteral("layout { gaps \"x\"; }") << QStringLiteral("only numbers");
}

void TestConfigLayout::rejectsOutOfRangeValues()
{
    QFETCH(QString, text);
    QFETCH(QString, fragment);
    QVERIFY2(mustFail(text).message.contains(fragment), qPrintable(mustFail(text).message));
}

void TestConfigLayout::rejectsUnknownNodes_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("message");

    QTest::newRow("top level") << QStringLiteral("nonsense {}") << QStringLiteral("unexpected node `nonsense`");
    QTest::newRow("layout child") << QStringLiteral("layout { bogus 1; }") << QStringLiteral("unexpected node `bogus`");
    QTest::newRow("border child") << QStringLiteral("layout { border { bogus; }; }") << QStringLiteral("unexpected node `bogus`");
    QTest::newRow("preset") << QStringLiteral("layout { preset-column-widths { huge 1; }; }") << QStringLiteral("unexpected node `huge`");
    QTest::newRow("property") << QStringLiteral("layout { gaps bogus=1; }") << QStringLiteral("unexpected property `bogus`");
}

void TestConfigLayout::rejectsUnknownNodes()
{
    QFETCH(QString, text);
    QFETCH(QString, message);
    QCOMPARE(mustFail(text).message, message);
}

void TestConfigLayout::reportsAccurateLocations()
{
    const LoadError error = mustFail(QStringLiteral("layout {\n    border {\n        bogus\n    }\n}\n"));
    QCOMPARE(error.location.line, 3);
    QCOMPARE(error.location.column, 9);
    QCOMPARE(error.sourceLine, QStringLiteral("        bogus"));
}

void TestConfigLayout::rejectsDuplicateSections()
{
    QCOMPARE(mustFail(QStringLiteral("layout {}\nlayout {}\n")).message, QStringLiteral("duplicate node `layout`, single node expected"));
    QCOMPARE(mustFail(QStringLiteral("layout { gaps 1; gaps 2; }")).message, QStringLiteral("duplicate node `gaps`, single node expected"));
    QVERIFY(mustFail(QStringLiteral("window-rule {}\nwindow-rule {}\n")).message.contains(QStringLiteral("without an error")));
}

QTEST_MAIN(TestConfigLayout)
#include "test_config_layout.moc"
