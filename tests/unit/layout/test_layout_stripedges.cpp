#include "striphelpers.h"

using namespace LayoutTest;

class TestLayoutStripEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scrollOffsetsFollowTheActiveMotion()
    {
        Layout::StripScroll idle;
        idle.offset(5.0);
        QCOMPARE(idle.current(), 5.0);
        QVERIFY(idle.isIdle());

        Clock clock = Clock::frozenAt(Duration::zero());
        const Config::AnimationParams params = linearAnimationConfig().animations.horizontalViewMovement;
        Layout::StripScroll animated;
        animated.setAnimation(Konveyor::Anim::Animation(clock, 0.0, 100.0, 0.0, params));
        animated.offset(10.0);
        QCOMPARE(animated.target(), 110.0);
        animated.halt();
        QVERIFY(animated.isIdle());

        Layout::StripScroll swiping;
        Layout::StripSwipe gesture;
        gesture.swipePosition = 20.0;
        gesture.restingPosition = 20.0;
        swiping.setSwipe(gesture);
        QVERIFY(swiping.isSwiping());
        swiping.offset(4.0);
        QCOMPARE(swiping.target(), 24.0);
        QCOMPARE(swiping.resting(), 24.0);
        swiping.cancelSwipe();
        QVERIFY(!swiping.isSwiping());
        QCOMPARE(swiping.current(), 24.0);
        swiping.cancelSwipe();
        QCOMPARE(swiping.current(), 24.0);
    }

    void tabBarWithoutTabsHasNoRects()
    {
        const Layout::TabBar bar {Config::TabIndicator()};
        QVERIFY(bar.tabRects(QRectF(0, 0, 400, 300), 0, 1.0).isEmpty());
        QCOMPARE(bar.tabRects(QRectF(0, 0, 400, 300), 3, 1.0).size(), 3);
    }

    void columnSettersIgnoreRepeatedState()
    {
        StripKit kit;
        const Layout::WindowId id = kit.addColumn();
        Layout::Column &column = kit.strip.columns().front();
        column.setFullscreen(false);
        QVERIFY(!column.fullscreenPending);
        column.setMaximized(false);
        QVERIFY(!column.maximizePending);
        column.setColumnDisplay(Config::ColumnDisplay::Normal);
        QVERIFY(!column.isTabbed());
        column.updateWindow(id + 100);
        column.activateWindow(id + 100);
        QVERIFY(!column.animateOpening(id + 100));
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void columnsReportSlidesAsAnimations()
    {
        StripKit kit(linearAnimationConfig());
        kit.addColumn();
        Layout::Column &column = kit.strip.columns().front();
        const Config::AnimationParams slow = linearAnimationConfig(1000).animations.windowMovement;
        const Config::AnimationParams quick = linearAnimationConfig(100).animations.windowMovement;
        kit.clock.setRawNow(kit.clock.rawNow() + std::chrono::seconds(5));
        column.tickAnimations();
        QVERIFY(!column.isAnimating());
        column.slideFromWith(QPointF(0, 40), slow);
        QVERIFY(column.isAnimating());
        column.slideXFromWith(40, quick);
        kit.clock.setRawNow(kit.clock.rawNow() + std::chrono::milliseconds(500));
        column.tickAnimations();
        QVERIFY(column.isAnimating());
        column.shiftSlideX(5);
        kit.clock.setRawNow(kit.clock.rawNow() + std::chrono::seconds(5));
        column.tickAnimations();
        QVERIFY(!column.isAnimating());
        column.slideXFromWith(40, slow);
        QVERIFY(column.isAnimating());
    }

    void columnsAdoptParentAreaChanges()
    {
        StripKit kit;
        kit.addColumn();
        Layout::Column &column = kit.strip.columns().front();
        Layout::AreaInfo area = column.area();
        area.parentArea = QRectF(0, 0, 1920, 1000);
        column.updateConfig(area, kit.options);
        QCOMPARE(column.area().parentArea, QRectF(0, 0, 1920, 1000));
    }

    void columnPositionsClampPastTheEnd()
    {
        StripKit kit;
        kit.addColumn();
        kit.addColumn();
        QCOMPARE(kit.strip.columnOffset(99), kit.strip.columnOffset(2));
        QVERIFY(!kit.strip.columnRenderPosition(0));
        QVERIFY(kit.strip.columnRenderPosition(kit.strip.columns().front().id()));
        QCOMPARE(kit.strip.renderedTilesMut(true).size(), std::size_t(2));
        QCOMPARE(kit.strip.renderedTilesMut(false).size(), std::size_t(2));
    }

    void pinnedColumnsKeepTheirEndsOfTheRow()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        const Layout::WindowId second = kit.addColumn();
        kit.strip.focusColumnFirst();
        kit.addPinned(Config::ColumnPosition::Start, false);
        QCOMPARE(kit.strip.columns().front().tiles.front().id(), second + 1);
        QCOMPARE(kit.strip.activeTile()->id(), first);
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void pinsMovingPastTheActiveColumnKeepItActive()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        kit.addColumn();
        kit.addPinned(Config::ColumnPosition::Start, false);
        std::swap(kit.strip.columns().at(0), kit.strip.columns().at(2));
        kit.strip.focusColumn(2);
        QCOMPARE(kit.strip.activeTile()->id(), first);
        kit.strip.placeColumnsWithinPins();
        QCOMPARE(kit.strip.activeTile()->id(), first);
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void expellingFromAColumnRightOfTheActiveOne()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        kit.addColumn();
        kit.addColumn();
        const Layout::WindowId stacked = kit.addToColumn(2);
        kit.strip.focusColumn(0);
        kit.strip.consumeOrExpelWindowLeft(stacked);
        QCOMPARE(kit.strip.columns().size(), std::size_t(4));
        QCOMPARE(kit.strip.activeTile()->id(), first);
    }

    void allowedIndexSkipsMisorderedPins()
    {
        StripKit kit;
        kit.addColumn();
        kit.addPinned(Config::ColumnPosition::Start, true);
        std::swap(kit.strip.columns().at(0), kit.strip.columns().at(1));
        QCOMPARE(kit.strip.allowedColumnIndex(std::nullopt, 0, std::nullopt), std::size_t(0));
        QCOMPARE(kit.strip.allowedColumnIndex(Config::ColumnPosition::Start, 5, std::nullopt), std::size_t(0));
    }

    void unknownWindowsLeaveColumnDisplayAlone()
    {
        StripKit empty;
        empty.strip.toggleColumnTabbedDisplay(std::nullopt);
        empty.strip.setColumnDisplay(Config::ColumnDisplay::Tabbed, std::nullopt);
        empty.strip.centerWindow(std::nullopt);
        empty.strip.moveColumnToIndex(1, std::nullopt);

        StripKit kit;
        kit.addColumn();
        kit.strip.toggleColumnTabbedDisplay(std::nullopt);
        kit.strip.toggleColumnTabbedDisplay(std::nullopt);
        kit.strip.toggleColumnTabbedDisplay(99);
        kit.strip.setColumnDisplay(Config::ColumnDisplay::Tabbed, 99);
        QVERIFY(!kit.strip.columns().front().isTabbed());
        kit.strip.setColumnDisplay(Config::ColumnDisplay::Normal, std::nullopt);
        kit.strip.moveColumnToIndex(2, 99);
        kit.strip.centerWindow(std::nullopt);
        kit.strip.updateWindow(99);
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void movingAnInactiveColumnByWindow()
    {
        StripKit kit;
        const Layout::WindowId first = kit.addColumn();
        kit.addColumn();
        kit.addColumn();
        kit.strip.moveColumnToIndex(3, first);
        QCOMPARE(kit.strip.columns().back().tiles.front().id(), first);
        QCOMPARE(kit.strip.checkConsistency(), QString());
    }

    void tabbedColumnsHitTestTheirTabs()
    {
        StripKit kit;
        kit.addColumn();
        const Layout::WindowId second = kit.addToColumn(0);
        kit.strip.setColumnDisplay(Config::ColumnDisplay::Tabbed, second);
        QVERIFY(kit.strip.setFullscreen(second, true));
        kit.strip.tileFor(second)->window().commit(QSizeF(1920, 1080));
        kit.strip.updateWindow(second);
        QPointF inside;
        for (const Layout::TileRef &ref : kit.strip.renderedTilesMut(false)) {
            if (ref.tile->id() == second) {
                inside = ref.pos + QPointF(50, 50);
            }
        }
        const auto hit = kit.strip.windowUnder(inside);
        QVERIFY(hit);
        QCOMPARE(hit->id, second);
    }

    void layoutChangesStackLooseColumns()
    {
        StripKit kit;
        kit.addColumn();
        kit.addColumn();
        kit.addColumn();
        Layout::Tile ruled = kit.makeTile();
        Layout::EffectiveWindowRules rules;
        rules.newWindowPlacement = Config::NewWindowPlacement::Stack;
        ruled.window().setRules(rules);
        kit.strip.addTile(std::nullopt, std::move(ruled), false, Layout::ColumnWidth::proportion(0.5), false, std::nullopt);
        Layout::Tile separate = kit.makeTile();
        Layout::EffectiveWindowRules other;
        other.newWindowPlacement = Config::NewWindowPlacement::Column;
        separate.window().setRules(other);
        kit.strip.addTile(std::nullopt, std::move(separate), false, Layout::ColumnWidth::proportion(0.5), false, std::nullopt);
        Config::Layout from;
        from.newWindowPlacement = Config::NewWindowPlacement::Column;
        Config::Layout to = from;
        to.newWindowPlacement = Config::NewWindowPlacement::Stack;
        to.maxRowsPerColumn = 2;
        kit.strip.reflowForLayout(from, to);
        QVERIFY(kit.strip.columns().size() < 5);
        QCOMPARE(kit.strip.checkConsistency(), QString());
        kit.strip.reflowForLayout(to, from);
        QCOMPARE(kit.strip.columns().size(), std::size_t(5));
    }

    void closestPresetNeedsPresets()
    {
        Layout::WindowProperties fixed = makeWindow(QStringLiteral("fixed"), {}, QSizeF(960, 540));
        fixed.isResizable = false;
        StripKit bare(optionsWithoutPresets(), QSizeF(1920, 1080), QRectF(0, 0, 1920, 1080), 1.0);
        bare.addColumn(fixed);
        QVERIFY(!bare.strip.columns().front().closestWidthPresetIndex(0).has_value());
        StripKit plain;
        plain.addColumn();
        QVERIFY(!plain.strip.columns().front().closestWidthPresetIndex(0).has_value());
    }

    void proportionChangesOfFixedSizesSurviveNarrowAreas()
    {
        StripKit kit(instantConfig(), QSizeF(16, 16), QRectF(0, 0, 16, 16));
        kit.addColumn();
        Layout::Column &column = kit.strip.columns().front();
        column.widthSetting = Layout::ColumnWidth::fixed(10);
        column.setColumnWidth(Layout::SizeChange {Layout::ChangeKind::AdjustProportion, 10}, std::nullopt, false);
        QVERIFY(column.widthSetting.isProportion);
        column.setWindowHeight(Layout::SizeChange {Layout::ChangeKind::AdjustProportion, 10}, std::nullopt, false);
    }

    void presetHeightsRestartAfterMaximizing()
    {
        StripKit kit;
        kit.addColumn();
        kit.addToColumn(0);
        Layout::Column &column = kit.strip.columns().front();
        column.toggleWindowHeight(std::nullopt, true);
        column.toggleWindowHeight(std::nullopt, true);
        column.maximizePending = true;
        column.toggleWindowHeight(std::nullopt, true);
        QVERIFY(!column.maximizePending);
    }

    void zeroWeightHeightsTakeNoSpace()
    {
        StripKit kit;
        kit.addColumn();
        kit.addToColumn(0);
        Layout::Column &column = kit.strip.columns().front();
        column.data[0].height = Layout::WindowHeight::autoWeight(0.0);
        column.layoutTiles(false);
        QVERIFY(column.tiles.front().targetOuterSize().height() > 0.0);
    }
};

QTEST_GUILESS_MAIN(TestLayoutStripEdges)
#include "test_layout_stripedges.moc"
