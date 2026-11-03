#include "striphelpers.h"

using namespace LayoutTest;

namespace
{

Layout::SizeChange fixedChange(double value)
{
    return Layout::SizeChange {Layout::ChangeKind::SetFixed, value};
}

}

class TestLayoutStripSizing : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sizingWithoutAWindowTargetsTheActiveColumn()
    {
        StripKit kit;
        kit.addColumn();
        kit.addColumn();
        kit.strip.setWindowWidth(std::nullopt, fixedChange(700));
        QCOMPARE(kit.strip.columns().at(1).widthSetting, Layout::ColumnWidth::fixed(700));
        kit.strip.setWindowHeight(std::nullopt, fixedChange(500));
        QVERIFY(!kit.strip.columns().at(1).data.front().height.isAuto());
        kit.strip.resetWindowHeight(std::nullopt);
        QVERIFY(kit.strip.columns().at(1).data.front().height.isAuto());
        kit.strip.toggleWindowWidth(std::nullopt, true, false);
        kit.strip.toggleWindowHeight(std::nullopt, true);
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void sizingAnUnknownWindowDoesNothing()
    {
        StripKit kit;
        kit.addColumn();
        const Layout::ColumnWidth before = kit.strip.columns().front().widthSetting;
        kit.strip.setWindowWidth(99, fixedChange(700));
        kit.strip.setWindowHeight(99, fixedChange(500));
        kit.strip.resetWindowHeight(99);
        kit.strip.toggleWindowWidth(99, true, false);
        kit.strip.toggleWindowHeight(99, true);
        kit.strip.toggleFillWidthFor(99);
        QCOMPARE(kit.strip.columns().front().widthSetting, before);
    }

    void sizingOnAnEmptyStripDoesNothing()
    {
        StripKit kit;
        kit.strip.setWindowWidth(std::nullopt, fixedChange(700));
        kit.strip.setWindowHeight(std::nullopt, fixedChange(700));
        kit.strip.toggleFillWidth();
        QVERIFY(kit.strip.isEmpty());
    }

    void returningFromFullscreenNeedsItsOriginColumn_data()
    {
        QTest::addColumn<int>("scenario");
        QTest::newRow("origin removed") << 0;
        QTest::newRow("fullscreen column grew") << 1;
        QTest::newRow("origin tabbed") << 2;
        QTest::newRow("origin fullscreen") << 3;
        QTest::newRow("plain return") << 4;
    }

    void returningFromFullscreenNeedsItsOriginColumn()
    {
        QFETCH(int, scenario);
        StripKit kit;
        const Layout::WindowId base = kit.addColumn();
        const Layout::WindowId stacked = kit.addToColumn(0);
        QVERIFY(kit.strip.setFullscreen(stacked, true));
        QCOMPARE(kit.strip.columns().size(), std::size_t(2));
        switch (scenario) {
        case 0:
            kit.strip.removeTile(base);
            break;
        case 1:
            kit.addToColumn(kit.strip.columns().size() - 1);
            kit.strip.columns().back().fullscreenPending = true;
            break;
        case 2:
            kit.strip.setColumnDisplay(Config::ColumnDisplay::Tabbed, base);
            break;
        case 3:
            QVERIFY(kit.strip.setFullscreen(base, true));
            break;
        default:
            break;
        }
        QVERIFY(kit.strip.setFullscreen(stacked, false));
        if (scenario == 4) {
            QCOMPARE(kit.strip.columns().size(), std::size_t(1));
        }
        QVERIFY(kit.strip.hasWindow(stacked));
    }

    void resizeSessionsRejectBadRequests()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        const Layout::WindowId second = kit.addColumn();
        const quint8 right = static_cast<quint8>(Layout::ResizeEdge::Right);
        QVERIFY(!kit.strip.beginResize(99, right));
        QVERIFY(!kit.strip.updateResize(first, QPointF(5, 5)));
        QVERIFY(kit.strip.setFullscreen(second, true));
        QVERIFY(!kit.strip.beginResize(second, right));
        QVERIFY(kit.strip.setFullscreen(second, false));
        QVERIFY(kit.strip.beginResize(first, right));
        QVERIFY(!kit.strip.beginResize(first, right));
        QVERIFY(!kit.strip.updateResize(second, QPointF(5, 5)));
        kit.strip.endResize(second);
        QVERIFY(kit.strip.updateResize(first, QPointF(5, 5)));
        kit.strip.endResize(std::nullopt);
        QVERIFY(!kit.strip.updateResize(first, QPointF(5, 5)));
        kit.strip.endResize(std::nullopt);
    }

    void resizingFromTheTopOfTheFirstTileOnlyChangesWidth()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        const Layout::WindowId second = kit.addToColumn(0);
        const quint8 topLeft = static_cast<quint8>(Layout::ResizeEdge::Top) | static_cast<quint8>(Layout::ResizeEdge::Left);
        QVERIFY(kit.strip.beginResize(first, topLeft));
        QVERIFY(kit.strip.updateResize(first, QPointF(-20, -20)));
        kit.strip.endResize(first);
        QVERIFY(kit.strip.beginResize(second, topLeft));
        QVERIFY(kit.strip.updateResize(second, QPointF(-20, -20)));
        kit.strip.endResize(second);
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void endingAResizeOnAnotherColumnKeepsTheView()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        kit.addColumn();
        QVERIFY(kit.strip.beginResize(first, static_cast<quint8>(Layout::ResizeEdge::Right)));
        kit.strip.endResize(first);
        QVERIFY(kit.strip.beginResize(first, static_cast<quint8>(Layout::ResizeEdge::Right)));
    }

    void dropSlotsDescribeTheSpaceAroundColumns()
    {
        StripKit kit;
        QVERIFY(kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::NewColumn, 0, 0}).has_value());
        QVERIFY(!kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::Floating, 0, 0}).has_value());
        QVERIFY(!kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::NewColumn, 5, 0}).has_value());
        kit.addColumn();
        kit.addToColumn(0);
        kit.addToColumn(0);
        kit.addColumn();
        for (std::size_t tile = 0; tile <= 3; ++tile) {
            QVERIFY(kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::InColumn, 0, tile}).has_value());
        }
        QVERIFY(!kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::InColumn, 0, 9}).has_value());
        for (std::size_t column = 0; column <= 2; ++column) {
            QVERIFY(kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::NewColumn, column, 0}).has_value());
        }
    }

    void dropSlotsOfTabbedColumnsSitAroundTheActiveTab()
    {
        StripKit kit;
        kit.addColumn();
        const Layout::WindowId second = kit.addToColumn(0);
        kit.addToColumn(0);
        kit.strip.setColumnDisplay(Config::ColumnDisplay::Tabbed, second);
        for (std::size_t tile = 0; tile <= 3; ++tile) {
            QVERIFY(kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::InColumn, 0, tile}).has_value());
        }
        QVERIFY(kit.strip.setFullscreen(second, true));
        kit.strip.tileFor(second)->window().commit(QSizeF(1920, 1080));
        kit.strip.updateWindow(second);
        QVERIFY(kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::InColumn, 0, 1}).has_value());
    }

    void centeredEmptyStripsPlaceTheDropSlotInTheMiddle()
    {
        Config::Config config = instantConfig();
        config.layout.centerFocusedColumn = Config::CenterFocusedColumn::Always;
        StripKit kit(config);
        const auto centered = kit.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::NewColumn, 0, 0});
        StripKit plain;
        const auto fitted = plain.strip.dropSlotRect(Layout::DropSlot {Layout::DropSlot::Kind::NewColumn, 0, 0});
        QVERIFY(centered && fitted);
        QVERIFY(centered->x() != fitted->x());
    }

    void pinnedDropsOnlyJoinColumnsWithTheSamePin()
    {
        StripKit kit;
        kit.addColumn();
        kit.addPinned(Config::ColumnPosition::Start, false);
        const QPointF inside(300 - kit.strip.scrollPosition(), 20);
        const Layout::DropSlot same = kit.strip.dropSlotAt(inside, Config::ColumnPosition::Start);
        QCOMPARE(same.kind, Layout::DropSlot::Kind::InColumn);
        const Layout::DropSlot other = kit.strip.dropSlotAt(inside, Config::ColumnPosition::End);
        QCOMPARE(other.kind, Layout::DropSlot::Kind::NewColumn);
        const Layout::DropSlot none = kit.strip.dropSlotAt(inside, std::nullopt);
        QCOMPARE(none.kind, Layout::DropSlot::Kind::InColumn);
    }
};

QTEST_GUILESS_MAIN(TestLayoutStripSizing)
#include "test_layout_stripsizing.moc"
