#include "helpers.h"

using namespace LayoutTest;

class TestLayoutFullscreen : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maximizeRequestTogglesOnlyTheColumnWidthAndKeepsItOnScreen()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        const auto third = fixture.add();
        fixture.engine().activateWindow(second);
        const QRectF before = fixture.frame(second);

        fixture.engine().toggleWindowFillWidth(second);
        fixture.settle();
        QCOMPARE(fixture.frame(second), QRectF(16, 16, 1888, 1048));
        QCOMPARE(fixture.state(second).sizingMode, Layout::WindowMode::Normal);

        fixture.engine().toggleWindowFillWidth(second);
        fixture.settle();
        QCOMPARE(fixture.frame(second).size(), before.size());
        QVERIFY(fixture.frame(second).left() >= 0.0 && fixture.frame(second).right() <= 1920.0);

        fixture.engine().activateWindow(third);
        fixture.settle();
        fixture.engine().toggleWindowFillWidth(first);
        fixture.settle();
        QCOMPARE(fixture.focused(), std::optional(first));
        QCOMPARE(fixture.frame(first), QRectF(16, 16, 1888, 1048));
        QCOMPARE(fixture.state(third).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void windowsThatOpenMaximizedTakeTheFullColumnWidthNotTheEdges()
    {
        Fixture fixture;
        Layout::WindowProperties maximized = LayoutTest::makeWindow(QStringLiteral("app"));
        maximized.wantsMaximized = true;
        const auto id = fixture.addWith(maximized);
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 1888, 1048));
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionGoesMaxSizeThenEdgesThenNormal()
    {
        Fixture fixture;
        fixture.add();
        const auto id = fixture.add();
        fixture.engine().activateWindow(id);
        const QRectF normal = fixture.frame(id);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 1888, 1048));

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id).size(), normal.size());
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionTemporarilyResizesFixedWindowThenRestoresNativeSize()
    {
        Fixture fixture;
        Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 1888, 1048));
        QVERIFY(fixture.state(id).isExpansionForceResizable);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.state(id).isExpansionForceResizable);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id).size(), QSizeF(300, 200));
        QVERIFY(!fixture.state(id).isForceResizable);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionReturnsForceResizableWindowToItsPreset()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("game"));
        rule.forceResizable = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);
        fixture.perform(QStringLiteral("switch-preset-column-width"), {}, {{QStringLiteral("from-native"), QStringLiteral("true")}});
        const QSizeF preset = fixture.frame(id).size();

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        fixture.perform(QStringLiteral("cycle-window-expansion"));
        fixture.perform(QStringLiteral("cycle-window-expansion"));

        QCOMPARE(fixture.frame(id).size(), preset);
        QVERIFY(fixture.state(id).isForceResizableByRule);
        QVERIFY(!fixture.state(id).isExpansionForceResizable);
        VERIFY_INVARIANTS(fixture);
    }

    void expandedWindowsCoverTheWholeOutputWithoutGaps()
    {
        Fixture fixture;
        fixture.add();
        const auto id = fixture.add();
        fixture.engine().activateWindow(id);
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenWindowCoversOutput()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("fullscreen-window")).ok);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenColumnReservesTheWholeOutputInTheRow()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        fixture.engine().setWindowFullscreen(first, true);
        fixture.settle();
        fixture.engine().activateWindow(second);
        fixture.settle();
        QVERIFY(
            fixture.frame(second).left() >= fixture.frame(first).right() || fixture.frame(second).right() <= fixture.frame(first).left());
        fixture.engine().activateWindow(first);
        fixture.settle();
        QCOMPARE(fixture.frame(first), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenWindowsHaveNoFocusRingBorderOrCornerClipping()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.focusRing.enabled = true;
        Config::WindowRule rule;
        rule.geometryCornerRadius = Config::CornerRadius {8, 8, 8, 8};
        rule.clipToGeometry = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add();
        const auto decorated = [&fixture, id]() {
            const Layout::WindowState state = fixture.state(id);
            return state.focusRing.enabled && state.border.enabled && state.clipToGeometry && state.cornerRadius.topLeft == 8.0;
        };
        const auto bare = [&fixture, id]() {
            const Layout::WindowState state = fixture.state(id);
            return !state.focusRing.enabled && !state.border.enabled && !state.clipToGeometry && state.cornerRadius.topLeft == 0.0;
        };
        QVERIFY(decorated());

        fixture.engine().setWindowFullscreen(id, true);
        QVERIFY(bare());
        fixture.settle();
        QVERIFY(bare());

        fixture.engine().setWindowFullscreen(id, false);
        fixture.settle();
        QVERIFY(decorated());
    }

    void columnsOverlayAFullscreenWindowLikeAStack()
    {
        Fixture fixture;
        const auto fullscreen = fixture.add();
        const auto first = fixture.add();
        const auto second = fixture.add();
        fixture.engine().activateWindow(fullscreen);
        fixture.engine().setWindowFullscreen(fullscreen, true);
        fixture.settle();
        const auto onScreen = [&fixture](Layout::WindowId id) { return fixture.frame(id).intersects(QRectF(0, 0, 1920, 1080)); };

        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.frame(first).right(), 1904.0);
        QVERIFY(!onScreen(second));

        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.frame(second).right(), 1904.0);
        QVERIFY(onScreen(first));

        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(first).right(), 1904.0);
        QVERIFY(!onScreen(second));

        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.focused(), std::optional(fullscreen));
        QCOMPARE(fixture.frame(fullscreen), QRectF(0, 0, 1920, 1080));
        QVERIFY(!onScreen(first));
        QVERIFY(!onScreen(second));
        VERIFY_INVARIANTS(fixture);
    }

    void columnsOverlayFromTheLeftTheSameWay()
    {
        Fixture fixture;
        const auto second = fixture.add();
        const auto first = fixture.add();
        const auto fullscreen = fixture.add();
        fixture.engine().setWindowFullscreen(fullscreen, true);
        fixture.settle();
        const auto onScreen = [&fixture](Layout::WindowId id) { return fixture.frame(id).intersects(QRectF(0, 0, 1920, 1080)); };

        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(first).left(), 16.0);
        QVERIFY(!onScreen(second));

        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(second).left(), 16.0);
        QVERIFY(onScreen(first));

        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.frame(first).left(), 16.0);
        QVERIFY(!onScreen(second));
        VERIFY_INVARIANTS(fixture);
    }

    void unfullscreenRestoresLayout()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("fullscreen-window"));
        QVERIFY(fixture.perform(QStringLiteral("fullscreen-window")).ok);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void engineSetFullscreen()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.engine().setWindowFullscreen(id, true);
        fixture.settle();
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        fixture.engine().setWindowFullscreen(id, false);
        fixture.settle();
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void windowOpeningFullscreen()
    {
        Fixture fixture;
        Layout::WindowProperties properties = makeWindow(QStringLiteral("player"), QStringLiteral("player"));
        properties.wantsFullscreen = true;
        const auto id = fixture.addWith(properties);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        VERIFY_INVARIANTS(fixture);
    }

    void openFullscreenRuleOverridesWindow()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^player$"));
        rule.matches.append(match);
        rule.openFullscreen = false;
        config.windowRules.append(rule);
        Fixture fixture(config);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("player"), QStringLiteral("player"));
        properties.wantsFullscreen = true;
        const auto id = fixture.addWith(properties);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void maximizeWindowToEdges()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("maximize-window-to-edges")).ok);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Maximized);
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.perform(QStringLiteral("maximize-window-to-edges")).ok);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void maximizeColumnTogglesFullWidth()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("maximize-column")).ok);
        QCOMPARE(fixture.frame(id).width(), 1920.0 - 16.0 * 2);
        QVERIFY(fixture.perform(QStringLiteral("maximize-column")).ok);
        QCOMPARE(fixture.frame(id).width(), 936.0);
        VERIFY_INVARIANTS(fixture);
    }

    void openMaximizedRuleMakesColumnFullWidth()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.openMaximized = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).width(), 1920.0 - 16.0 * 2);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenWindowStacksOnTop()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        fixture.perform(QStringLiteral("fullscreen-window"));
        QVERIFY(fixture.state(second).stackingIndex > fixture.state(first).stackingIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowUnfullscreensBackToFloating()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(id).isFloating);
        fixture.perform(QStringLiteral("fullscreen-window"));
        QVERIFY(!fixture.state(id).isFloating);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        fixture.perform(QStringLiteral("fullscreen-window"));
        QVERIFY(fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void toggleWindowedFullscreen()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("toggle-windowed-fullscreen")).ok);
        fixture.settle();
        QVERIFY(fixture.state(id).isWindowedFullscreen);
        QVERIFY(fixture.perform(QStringLiteral("toggle-windowed-fullscreen")).ok);
        fixture.settle();
        QVERIFY(!fixture.state(id).isWindowedFullscreen);
        VERIFY_INVARIANTS(fixture);
    }

    void tabbedColumnShowsOneWindow()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(fixture.perform(QStringLiteral("toggle-column-tabbed-display")).ok);
        QCOMPARE(fixture.state(second).visible, true);
        QCOMPARE(fixture.state(first).visible, false);
        QCOMPARE(fixture.frame(first).height(), fixture.frame(second).height());
        VERIFY_INVARIANTS(fixture);
    }

    void tabbedColumnHasTabBar()
    {
        Fixture fixture;
        fixture.add();
        const auto second = fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        const Layout::TabBarState indicator = fixture.state(second).tabBar;
        QVERIFY(indicator.visible);
        QCOMPARE(indicator.tabRects.size(), 2);
        QCOMPARE(indicator.tabPaints.size(), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void setColumnDisplayNormal()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("tabbed")});
        QCOMPARE(fixture.state(first).visible, false);
        QVERIFY(fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("normal")}).ok);
        QCOMPARE(fixture.state(first).visible, true);
        VERIFY_INVARIANTS(fixture);
    }

    void invalidColumnDisplayFails()
    {
        Fixture fixture;
        fixture.add();
        const auto result = fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("grid")});
        QVERIFY(!result.ok);
    }

    void defaultColumnDisplayFromConfig()
    {
        Config::Config config = instantConfig();
        config.layout.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
        Fixture fixture(config);
        const auto first = fixture.add();
        fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QCOMPARE(fixture.state(first).visible, false);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFullscreen)
#include "test_layout_fullscreen.moc"
