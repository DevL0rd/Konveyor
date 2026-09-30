#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config rememberingConfig()
{
    Config::Config config = instantConfig();
    config.layout.rememberWindowSizes = true;
    config.layout.rememberWindowPositions = true;
    return config;
}

Layout::WindowId reopen(Fixture &fixture, Layout::WindowId id, const QString &appId, const Layout::RestorePlacement &placement)
{
    const Layout::WindowId restored = id + 100;
    fixture.engine().addWindow(restored, makeWindow(appId, appId), QString(), Layout::ActivationPolicy::Focus, placement);
    fixture.settle();
    return restored;
}

void verifyReopensTwiceAt(Fixture &fixture, Layout::WindowId window, double width)
{
    const QString appId = QStringLiteral("app");
    fixture.remove(window);
    const Layout::WindowId second = fixture.add(appId);
    QCOMPARE(fixture.frame(second).width(), width);
    fixture.remove(second);
    QCOMPARE(fixture.frame(fixture.add(appId)).width(), width);
}

Layout::WindowProperties floatingWindow(const QString &appId)
{
    Layout::WindowProperties properties = makeWindow(appId, appId, QSizeF(400, 300));
    properties.isDialog = true;
    return properties;
}

}

class TestLayoutMemoryRestore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoringIntoAWorkspaceThatIsGoneOpensOnTheActiveOne()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto moved = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("move-window-to-workspace-down"));
        const Layout::RestorePlacement placement = *fixture.engine().placementOf(moved);
        QCOMPARE(fixture.state(moved).workspaceIndex, 2);
        fixture.remove(moved);
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.advance(1);
        const auto restored = reopen(fixture, moved, QStringLiteral("b"), placement);
        QCOMPARE(fixture.state(restored).workspaceIndex, 1);
        QCOMPARE(fixture.state(restored).columnIndex, 1);
        QCOMPARE(fixture.focused(), std::optional(restored));
        VERIFY_INVARIANTS(fixture);
    }

    void restoringPastTheLastColumnAppendsIt()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        const Layout::RestorePlacement placement = *fixture.engine().placementOf(c);
        fixture.remove(c);
        fixture.remove(b);
        const auto restored = reopen(fixture, c, QStringLiteral("c"), placement);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(restored).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void restoringPastTheLastTileAppendsItToTheColumn()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto top = fixture.add(QStringLiteral("top"));
        const auto middle = fixture.add(QStringLiteral("middle"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const auto bottom = fixture.add(QStringLiteral("bottom"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const Layout::RestorePlacement placement = *fixture.engine().placementOf(bottom);
        QCOMPARE(placement.tileIndex, std::optional<std::size_t>(2));
        fixture.remove(bottom);
        fixture.remove(middle);
        const auto restored = reopen(fixture, bottom, QStringLiteral("bottom"), placement);
        QCOMPARE(fixture.state(restored).columnIndex, fixture.state(top).columnIndex);
        QCOMPARE(fixture.state(restored).tileIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void restoringIntoAColumnThatIsGoneOpensANewColumn()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("top"));
        const auto bottom = fixture.add(QStringLiteral("bottom"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const Layout::RestorePlacement placement = *fixture.engine().placementOf(bottom);
        QCOMPARE(placement.columnIndex, std::size_t(1));
        fixture.remove(bottom);
        fixture.remove(bottom - 1);
        const auto restored = reopen(fixture, bottom, QStringLiteral("bottom"), placement);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(restored).columnIndex, 1);
        QCOMPARE(fixture.state(restored).tileIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedRememberedWidthLeavesTheBorderOut()
    {
        Config::Config config = rememberingConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("app"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("700")});
        QCOMPARE(fixture.frame(first).width(), 700.0);
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("app")).columnWidth, Layout::ColumnWidth::fixed(708));
        verifyReopensTwiceAt(fixture, first, 700.0);
    }

    void fixedRememberedWidthUsesTheBorderOfTheOutputLayout()
    {
        Config::Config config = rememberingConfig();
        Config::OutputConfig output;
        output.name = QStringLiteral("DP-1");
        output.layout = config.layout;
        output.layout->border.enabled = true;
        output.layout->border.width = 4;
        config.outputs.append(output);
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("app"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("700")});
        verifyReopensTwiceAt(fixture, first, 700.0);
    }

    void rememberingFollowsTheOutputLayout()
    {
        Config::Config config = instantConfig();
        Config::OutputConfig output;
        output.name = QStringLiteral("DP-2");
        output.layout = rememberingConfig().layout;
        config.outputs.append(output);
        Fixture fixture(config);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.add(QStringLiteral("left"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        QVERIFY(!fixture.engine().windowMemory().value(QStringLiteral("left")).columnWidth);
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto right = fixture.add(QStringLiteral("right"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("right")).columnWidth, Layout::ColumnWidth::proportion(0.25));
        fixture.remove(right);
        QCOMPARE(fixture.frame(fixture.add(QStringLiteral("right"))).width(), 460.0);
    }

    void oneAppAtTwoWidthsSettlesOnOneMemory()
    {
        int changes = 0;
        Layout::Hooks hooks;
        hooks.windowMemoryChanged = [&changes] { ++changes; };
        Fixture fixture(rememberingConfig(), QRectF(0, 0, 1920, 1080), hooks);
        fixture.add(QStringLiteral("app"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        fixture.add(QStringLiteral("app"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("75%")});
        const std::optional<Layout::ColumnWidth> remembered = fixture.engine().windowMemory().value(QStringLiteral("app")).columnWidth;
        QVERIFY(remembered == Layout::ColumnWidth::proportion(0.25) || remembered == Layout::ColumnWidth::proportion(0.75));
        changes = 0;
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.perform(QStringLiteral("focus-column-right"));
        fixture.engine().setLayoutFocused(false);
        QCOMPARE(changes, 0);
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("app")).columnWidth, remembered);
    }

    void rememberedPositionReopensTheFloatingWindow()
    {
        Fixture fixture(rememberingConfig());
        const auto first = fixture.addWith(floatingWindow(QStringLiteral("picker")));
        fixture.engine().setFloatingFrame(first, QRectF(700, 500, 400, 300));
        fixture.settle();
        fixture.remove(first);
        const auto reopened = fixture.addWith(floatingWindow(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(reopened).topLeft(), QPointF(700, 500));
    }

    void defaultFloatingPositionBeatsARememberedPosition()
    {
        Config::Config config = rememberingConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("picker"));
        rule.defaultFloatingPosition = Config::FloatingPosition {10, 20, Config::FloatingRelativeTo::BottomRight};
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto first = fixture.addWith(floatingWindow(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(first).topLeft(), QPointF(1920 - 10 - 400, 1080 - 20 - 300));
        fixture.engine().setFloatingFrame(first, QRectF(700, 500, 400, 300));
        fixture.settle();
        QVERIFY(fixture.engine().windowMemory().value(QStringLiteral("picker")).floatingPosition);
        fixture.remove(first);
        const auto reopened = fixture.addWith(floatingWindow(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(reopened).topLeft(), QPointF(1510, 760));
    }

    void rememberedFloatingSizeYieldsToASizeRule()
    {
        Config::Config config = rememberingConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("picker"));
        rule.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Fixed {500});
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto first = fixture.addWith(floatingWindow(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(first).width(), 500.0);
        fixture.engine().setFloatingFrame(first, QRectF(100, 100, 800, 600));
        fixture.settle();
        fixture.remove(first);
        const auto reopened = fixture.addWith(floatingWindow(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(500, 600));
    }

    void memorySwitchedOffKeepsNativeSizesOnly()
    {
        Fixture fixture;
        Layout::WindowProperties fixed = makeWindow(QStringLiteral("fixed"), QStringLiteral("fixed"), QSizeF(400, 300));
        fixed.isResizable = false;
        const auto id = fixture.addWith(fixed);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        const Layout::RememberedWindow remembered = fixture.engine().windowMemory().value(QStringLiteral("fixed"));
        QCOMPARE(remembered.nativeSize, std::optional(QSize(400, 300)));
        QVERIFY(!remembered.columnWidth);
        QVERIFY(!remembered.floatingPosition);
        fixture.remove(id);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMemoryRestore)
#include "test_layout_memoryrestore.moc"
