#include "consistency.h"
#include "striphelpers.h"

#include <cmath>
#include <limits>

using namespace LayoutTest;

namespace
{

void fillAndFocusLast(StripKit &kit)
{
    for (int i = 0; i < 4; ++i) {
        kit.addColumn(makeWindow(), 0.5);
    }
    kit.strip.focusColumnLast();
    QCOMPARE(Layout::ConsistencyCheck::viewError(kit.strip), QString());
}

}

class TestLayoutStripConsistency : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void healthyStripIsConsistent()
    {
        StripKit kit;
        kit.addColumn();
        kit.addToColumn(0);
        kit.addColumn();
        QCOMPARE(Layout::ConsistencyCheck::of(kit.strip), QString());
        QCOMPARE(Layout::ConsistencyCheck::viewError(kit.strip), QString());
    }

    void emptyStripIsConsistent()
    {
        StripKit kit;
        QCOMPARE(Layout::ConsistencyCheck::of(kit.strip), QString());
        QCOMPARE(Layout::ConsistencyCheck::viewError(kit.strip), QString());
    }

    void rejectsViewWithoutWidth()
    {
        StripKit kit(instantConfig(), QSizeF(0, 1080));
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("view size must be positive")));
    }

    void rejectsViewWithoutHeight()
    {
        StripKit kit(instantConfig(), QSizeF(1920, 0));
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("view size must be positive")));
    }

    void rejectsNonPositiveScale()
    {
        StripKit kit(instantConfig(), QSizeF(1920, 1080), QRectF(0, 0, 1920, 1080), 0.0);
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("scale must be positive and finite")));
    }

    void rejectsInfiniteScale()
    {
        StripKit kit(instantConfig(), QSizeF(1920, 1080), QRectF(0, 0, 1920, 1080), std::numeric_limits<double>::infinity());
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("scale must be positive and finite")));
    }

    void rejectsWorkingAreaThatIgnoresStruts()
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        StripKit kit(instantConfig(), QSizeF(1920, 1080), QRectF(nan, 0, 1920, 1080));
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("working area must match the struts")));
    }

    void rejectsInteractiveResizeWithoutItsWindow()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        QVERIFY(kit.strip.beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Right)));
        QCOMPARE(Layout::ConsistencyCheck::of(kit.strip), QString());
        kit.strip.columns().clear();
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("interactive resize window must be present")));
    }

    void rejectsActiveColumnOutOfRange()
    {
        StripKit kit;
        kit.addColumn();
        kit.addColumn();
        kit.addColumn();
        kit.strip.columns().pop_back();
        kit.strip.columns().pop_back();
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("active column index out of range")));
    }

    void rejectsEmptyColumns()
    {
        StripKit kit;
        kit.addColumn();
        kit.strip.columns().front().tiles.clear();
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("columns must not be empty")));
    }

    void rejectsActiveTileOutOfRange()
    {
        StripKit kit;
        kit.addColumn();
        kit.strip.columns().front().activeTileIndex = 3;
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("active tile index out of range")));
    }

    void rejectsSizingDataThatDoesNotMatchTiles()
    {
        StripKit kit;
        kit.addColumn();
        kit.strip.columns().front().data.clear();
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("tile sizing data must match the tiles")));
    }

    void rejectsWindowSmallerThanItsMinimum()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        Layout::WindowProperties properties = makeWindow();
        properties.minSize = QSizeF(5000, 100);
        kit.strip.tileFor(id)->window().setProperties(properties);
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("smaller than its minimum size")));
    }

    void rejectsWindowShorterThanItsMinimum()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        Layout::WindowProperties properties = makeWindow();
        properties.minSize = QSizeF(100, 5000);
        kit.strip.tileFor(id)->window().setProperties(properties);
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("smaller than its minimum size")));
    }

    void rejectsWindowWiderThanItsMaximum()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        Layout::WindowProperties properties = makeWindow();
        properties.maxSize = QSizeF(200, 0);
        kit.strip.tileFor(id)->window().setProperties(properties);
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("larger than its maximum size")));
    }

    void rejectsWindowTallerThanItsMaximum()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        Layout::WindowProperties properties = makeWindow();
        properties.maxSize = QSizeF(0, 200);
        kit.strip.tileFor(id)->window().setProperties(properties);
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("larger than its maximum size")));
    }

    void ignoresMaximumsBelowTheColumnMinimum()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        const Layout::WindowId second = kit.addToColumn(0);
        Layout::WindowProperties wide = makeWindow();
        wide.minSize = QSizeF(900, 100);
        kit.strip.tileFor(first)->window().setProperties(wide);
        Layout::WindowProperties narrow = makeWindow();
        narrow.maxSize = QSizeF(200, 0);
        kit.strip.tileFor(second)->window().setProperties(narrow);
        QVERIFY(!Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("larger than its maximum size")));
    }

    void rejectsStackedHeightsThatMissTheColumn()
    {
        StripKit kit;
        kit.addColumn();
        const Layout::WindowId second = kit.addToColumn(0);
        Layout::Tile *tile = kit.strip.tileFor(second);
        tile->requestOuterSize(QSizeF(tile->targetOuterSize().width(), 100), false);
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("stacked heights plus gaps")));
    }

    void skipsStackedHeightsForTabbedColumns()
    {
        StripKit kit;
        kit.addColumn();
        const Layout::WindowId second = kit.addToColumn(0);
        kit.strip.setColumnDisplay(Config::ColumnDisplay::Tabbed, second);
        Layout::Tile *tile = kit.strip.tileFor(second);
        tile->requestOuterSize(QSizeF(tile->targetOuterSize().width(), 100), false);
        QVERIFY(!Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("stacked heights plus gaps")));
    }

    void rejectsColumnPlacedOutsideItsPinnedEnd()
    {
        StripKit kit;
        kit.addColumn();
        Layout::WindowProperties properties = makeWindow();
        Layout::Tile pinned = kit.makeTile(properties);
        Layout::EffectiveWindowRules rules;
        rules.columnPosition = Config::ColumnPosition::End;
        pinned.window().setRules(rules);
        kit.strip.addTile(std::nullopt, std::move(pinned), false, Layout::ColumnWidth::proportion(0.5), false, std::nullopt);
        QCOMPARE(Layout::ConsistencyCheck::of(kit.strip), QString());
        std::swap(kit.strip.columns().at(0), kit.strip.columns().at(1));
        QVERIFY(Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("outside its pinned end of the row")));
    }

    void rejectsViewPastTheRowWhenTheRowFits()
    {
        StripKit kit(wideColumns());
        fillAndFocusLast(kit);
        for (Layout::Column &column : kit.strip.columns()) {
            column.widthSetting = Layout::ColumnWidth::proportion(0.1);
            column.layoutTiles(false);
        }
        QVERIFY(Layout::ConsistencyCheck::viewError(kit.strip).contains(QStringLiteral("past the row")));
    }

    void rejectsViewPastTheRowWhenTheRowOverflows()
    {
        StripKit kit(wideColumns());
        fillAndFocusLast(kit);
        kit.strip.columns().pop_back();
        QVERIFY(Layout::ConsistencyCheck::viewError(kit.strip).contains(QStringLiteral("past the row")));
    }

    void skipsLimitChecksForWindowsThatWantAnotherMode()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        Layout::WindowProperties properties = makeWindow();
        properties.minSize = QSizeF(5000, 100);
        kit.strip.tileFor(id)->window().setProperties(properties);
        kit.strip.tileFor(id)->window().requestSize(QSize(100, 100), Layout::WindowMode::Maximized, false);
        QCOMPARE(Layout::ConsistencyCheck::of(kit.strip), QString());
        QCOMPARE(Layout::ConsistencyCheck::viewError(kit.strip), QString());
    }

    void rejectsScrollRestoreWithoutExpandedColumn()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        QVERIFY(kit.strip.setFullscreen(id, true));
        Layout::Tile *tile = kit.strip.tileFor(id);
        tile->window().commit(QSizeF(1920, 1080));
        kit.strip.updateWindow(id);
        QCOMPARE(Layout::ConsistencyCheck::of(kit.strip), QString());
        tile->requestOuterSize(QSizeF(500, 500), false);
        tile->window().commit(QSizeF(500, 500));
        tile->updateWindow();
        QVERIFY(
            Layout::ConsistencyCheck::of(kit.strip).contains(QStringLiteral("must be fullscreen or maximized to restore a view offset")));
    }
};

QTEST_GUILESS_MAIN(TestLayoutStripConsistency)
#include "test_layout_stripconsistency.moc"
