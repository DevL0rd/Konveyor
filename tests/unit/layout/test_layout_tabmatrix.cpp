#include "tabhelpers.h"

using namespace LayoutTest;

namespace
{

struct Scene
{
    Layout::WindowId before = 0;
    QList<Layout::WindowId> tabs;
    Layout::WindowId after = 0;
};

Scene buildScene(Fixture &fixture)
{
    Scene scene;
    scene.before = addPlainColumn(fixture, QStringLiteral("before"));
    scene.tabs = addTabs(fixture, 3);
    scene.after = addPlainColumn(fixture, QStringLiteral("after"));
    fixture.perform(QStringLiteral("focus-column-left"));
    fixture.advance(1);
    return scene;
}

QRectF outer(Fixture &fixture, Layout::WindowId id, const Config::Config &config)
{
    const double border = config.layout.border.enabled ? config.layout.border.width : 0.0;
    return fixture.frame(id).adjusted(-border, -border, border, border);
}

void verifyTabRect(Fixture &fixture, const Scene &scene, const Config::Config &config, qsizetype index)
{
    const Config::TabIndicator &tab = config.layout.tabIndicator;
    const Layout::WindowId shown = fixture.focused().value();
    const Layout::TabBarState bar = fixture.state(shown).tabBar;
    const QRectF rect = bar.tabRects[index];
    const QRectF own = outer(fixture, shown, config);
    const QString where = QDebug::toString(rect);
    QVERIFY2(QRectF(0, 0, 1920, 1080).contains(rect), qPrintable(where + QStringLiteral(" leaves the output")));
    QCOMPARE(distance(rect, own), std::max(tab.gap, 0.0));
    for (const Layout::WindowId other : {scene.before, scene.after}) {
        const QRectF otherFrame = outer(fixture, other, config);
        QVERIFY2(!rect.intersects(otherFrame), qPrintable(where + QStringLiteral(" covers ") + QDebug::toString(otherFrame)));
        if (beside(rect, otherFrame, tab.position)) {
            const double least = tab.placeWithinColumn ? config.layout.gaps : std::max(tab.gap, 0.0);
            QVERIFY2(distance(rect, otherFrame) >= least,
                qPrintable(where + QStringLiteral(" is too close to ") + QDebug::toString(otherFrame)));
        }
    }
    const double half = std::min(rect.width(), rect.height()) / 2.0;
    QCOMPARE(bar.tabRadii[index].topLeft, std::min(tab.cornerRadius, half));
    QCOMPARE(bar.tabRadii[index].bottomRight, std::min(tab.cornerRadius, half));
    QCOMPARE(fixture.engine().hitAt(rect.center()), std::optional(Layout::WindowHit {scene.tabs[index], true}));
}

void verifyScene(Fixture &fixture, const Scene &scene, const Config::Config &config, int shownIndex)
{
    QCOMPARE(fixture.focused(), std::optional(scene.tabs[shownIndex]));
    verifyShownTab(fixture, scene.tabs, shownIndex);
    const Layout::TabBarState bar = fixture.state(scene.tabs[shownIndex]).tabBar;
    QVERIFY(bar.visible);
    QCOMPARE(bar.tabRects.size(), scene.tabs.size());
    QCOMPARE(bar.tabRadii.size(), scene.tabs.size());
    QCOMPARE(bar.tabPaints.size(), scene.tabs.size());
    for (qsizetype i = 0; i < bar.tabRects.size(); ++i) {
        verifyTabRect(fixture, scene, config, i);
        if (QTest::currentTestFailed()) {
            return;
        }
        if (i > 0) {
            QVERIFY(!bar.tabRects[i].intersects(bar.tabRects[i - 1]));
        }
    }
    for (const Layout::WindowId tab : scene.tabs) {
        QCOMPARE(fixture.frame(tab), fixture.frame(scene.tabs[shownIndex]));
    }
    VERIFY_INVARIANTS(fixture);
}

}

class TestLayoutTabMatrix : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everySettingKeepsTheTabsInPlace_data()
    {
        QTest::addColumn<Config::TabIndicatorPosition>("position");
        QTest::addColumn<bool>("inside");
        QTest::addColumn<double>("width");
        QTest::addColumn<double>("gap");
        QTest::addColumn<double>("gaps");
        QTest::addColumn<bool>("border");
        QTest::addColumn<double>("radius");
        const QList<std::pair<const char *, Config::TabIndicatorPosition>> positions {{"left", Config::TabIndicatorPosition::Left},
            {"right", Config::TabIndicatorPosition::Right}, {"top", Config::TabIndicatorPosition::Top},
            {"bottom", Config::TabIndicatorPosition::Bottom}};
        const QList<double> radii {0.0, 3.0, 100.0};
        int row = 0;
        for (const auto &[name, position] : positions) {
            for (const bool inside : {false, true}) {
                for (const double width : {1.0, 4.0, 32.0}) {
                    for (const double gap : {-3.0, 0.0, 5.0, 20.0}) {
                        for (const double gaps : {0.0, 16.0, 50.0}) {
                            for (const bool border : {false, true}) {
                                const double radius = radii[row++ % radii.size()];
                                QTest::addRow("%s %s width %g gap %g gaps %g border %d radius %g", name, inside ? "inside" : "outside",
                                    width, gap, gaps, border, radius)
                                    << position << inside << width << gap << gaps << border << radius;
                            }
                        }
                    }
                }
            }
        }
    }

    void everySettingKeepsTheTabsInPlace()
    {
        QFETCH(Config::TabIndicatorPosition, position);
        QFETCH(bool, inside);
        QFETCH(double, width);
        QFETCH(double, gap);
        QFETCH(double, gaps);
        QFETCH(bool, border);
        QFETCH(double, radius);
        Config::Config config = tabbedConfig(position, width, gap);
        config.layout.tabIndicator.placeWithinColumn = inside;
        config.layout.tabIndicator.cornerRadius = radius;
        config.layout.gaps = gaps;
        config.layout.border.enabled = border;
        Fixture fixture(config);
        const Scene scene = buildScene(fixture);
        verifyScene(fixture, scene, config, 2);
        fixture.perform(QStringLiteral("focus-window-up"));
        verifyScene(fixture, scene, config, 1);
        fixture.perform(QStringLiteral("focus-column-right"));
        QVERIFY(fixture.state(scene.tabs[1]).visible);
        QCOMPARE(topmostTab(fixture, scene.tabs), 1);
        fixture.perform(QStringLiteral("focus-column-left"));
        verifyScene(fixture, scene, config, 1);
        fixture.perform(QStringLiteral("focus-window-top"));
        verifyScene(fixture, scene, config, 0);
    }
};

QTEST_GUILESS_MAIN(TestLayoutTabMatrix)
#include "test_layout_tabmatrix.moc"
