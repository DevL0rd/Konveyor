#include "helpers.h"

#include <tuple>

using namespace LayoutTest;

namespace
{

Config::Config pinConfig(const QString &appId, Config::ColumnPosition position)
{
    Config::Config config = instantConfig();
    Config::WindowRule rule = ruleFor(appId);
    rule.columnPosition = position;
    config.windowRules.append(rule);
    return config;
}

Layout::WindowId addIntoColumn(Fixture &fixture, const QString &appId, const QString &direction)
{
    const Layout::WindowId id = fixture.add(appId);
    fixture.perform(QStringLiteral("consume-or-expel-window-") + direction);
    return id;
}

std::tuple<Layout::WindowId, Layout::WindowId, Layout::WindowId> addEndPinnedStack(Fixture &fixture)
{
    const Layout::WindowId first = fixture.add(QStringLiteral("first"));
    const Layout::WindowId pinned = fixture.add(QStringLiteral("pin"));
    return {first, pinned, addIntoColumn(fixture, QStringLiteral("other"), QStringLiteral("right"))};
}

void verifyOnlyFullscreen(Fixture &fixture, Layout::WindowId window, const QList<Layout::WindowId> &others)
{
    QCOMPARE(fixture.state(window).sizingMode, Layout::WindowMode::Fullscreen);
    QCOMPARE(fixture.frame(window), QRectF(0, 0, 1920, 1080));
    const int column = fixture.state(window).columnIndex;
    for (const Layout::WindowId other : others) {
        QCOMPARE(fixture.state(other).sizingMode, Layout::WindowMode::Normal);
        QVERIFY(fixture.state(other).columnIndex != column);
    }
}

}

class TestLayoutFullscreenStack : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fullscreenExpelsTheBottomWindowToItsOwnColumn()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("fullscreen-window"));
        verifyOnlyFullscreen(fixture, bottom, {top});
        QCOMPARE(fixture.state(bottom).columnIndex, 1);
        QCOMPARE(fixture.frame(top), QRectF(-952, 16, 936, 1048));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(bottom), QRectF(968, 16, 936, 1048));
        QCOMPARE(fixture.focused(), std::optional(bottom));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenExpelsTheTopWindowToo()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("focus-window-up"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        verifyOnlyFullscreen(fixture, top, {bottom});
        QCOMPARE(fixture.state(top).columnIndex, 1);
        QCOMPARE(fixture.state(bottom).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenInAStartPinnedStackSkipsTheOtherPins()
    {
        Fixture fixture(pinConfig(QStringLiteral("pin"), Config::ColumnPosition::Start));
        const auto pinned = fixture.add(QStringLiteral("pin"));
        const auto stacked = addIntoColumn(fixture, QStringLiteral("other"), QStringLiteral("left"));
        const auto secondPin = fixture.add(QStringLiteral("pin"));
        const auto last = fixture.add(QStringLiteral("last"));
        QCOMPARE(fixture.state(stacked).columnIndex, 0);
        QCOMPARE(fixture.state(secondPin).columnIndex, 1);
        fixture.engine().activateWindow(stacked);
        fixture.perform(QStringLiteral("fullscreen-window"));
        verifyOnlyFullscreen(fixture, stacked, {pinned, secondPin, last});
        QCOMPARE(fixture.state(stacked).columnIndex, 2);
        QCOMPARE(fixture.state(pinned).columnIndex, 0);
        QCOMPARE(fixture.state(secondPin).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenOfAPinnedWindowLeavesItsStackBehind()
    {
        Fixture fixture(pinConfig(QStringLiteral("pin"), Config::ColumnPosition::Start));
        const auto pinned = fixture.add(QStringLiteral("pin"));
        const auto stacked = addIntoColumn(fixture, QStringLiteral("other"), QStringLiteral("left"));
        fixture.engine().activateWindow(pinned);
        fixture.perform(QStringLiteral("fullscreen-window"));
        verifyOnlyFullscreen(fixture, pinned, {stacked});
        QCOMPARE(fixture.state(pinned).columnIndex, 0);
        QCOMPARE(fixture.state(stacked).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenInAnEndPinnedStackStaysInsideTheRow()
    {
        Fixture fixture(pinConfig(QStringLiteral("pin"), Config::ColumnPosition::End));
        const auto [first, pinned, stacked] = addEndPinnedStack(fixture);
        QCOMPARE(fixture.state(stacked).columnIndex, 1);
        fixture.perform(QStringLiteral("fullscreen-window"));
        verifyOnlyFullscreen(fixture, stacked, {first, pinned});
        QCOMPARE(fixture.state(stacked).columnIndex, 1);
        QCOMPARE(fixture.state(pinned).columnIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void maximizeToEdgesInAPinnedStackHitsTheRightWindow()
    {
        Fixture fixture(pinConfig(QStringLiteral("pin"), Config::ColumnPosition::End));
        const auto [first, pinned, stacked] = addEndPinnedStack(fixture);
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QCOMPARE(fixture.state(stacked).sizingMode, Layout::WindowMode::Maximized);
        QCOMPARE(fixture.frame(stacked), QRectF(0, 0, 1920, 1080));
        QCOMPARE(fixture.state(pinned).sizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.state(first).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void presetIndexSurvivesFullscreen()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.state(a).widthPresetIndex, std::optional(2));
        fixture.perform(QStringLiteral("fullscreen-window"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(a).width(), 1253.0);
        QCOMPARE(fixture.state(a).widthPresetIndex, std::optional(2));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 618.0);
    }

    void expelledFullscreenWindowKeepsTheStacksWidthPreset()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(top).width(), 1253.0);
        fixture.perform(QStringLiteral("fullscreen-window"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(bottom).width(), 1253.0);
        QCOMPARE(fixture.state(bottom).widthPresetIndex, std::optional(2));
        QCOMPARE(fixture.frame(top).width(), 1253.0);
        VERIFY_INVARIANTS(fixture);
    }

    void presetActionsWhileFullscreenApplyAfterwards()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a), QRectF(0, 0, 1920, 1080));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(a).width(), 1253.0);
        VERIFY_INVARIANTS(fixture);
    }

    void openMaximizedToEdgesRuleOpensCoveringTheOutput()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("edges"));
        rule.openMaximizedToEdges = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto other = fixture.add(QStringLiteral("other"));
        const auto edges = fixture.add(QStringLiteral("edges"));
        QCOMPARE(fixture.state(edges).sizingMode, Layout::WindowMode::Maximized);
        QCOMPARE(fixture.frame(edges), QRectF(0, 0, 1920, 1080));
        QCOMPARE(fixture.state(edges).columnIndex, 1);
        QCOMPARE(fixture.frame(other), QRectF(-952, 16, 936, 1048));
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QCOMPARE(fixture.state(edges).sizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.frame(edges).size(), QSizeF(936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void openMaximizedToEdgesRuleStaysOutOfTheAppsStack()
    {
        Config::Config config = instantConfig();
        config.layout.groupAppWindows = Config::GroupAppWindows::Stack;
        Config::WindowRule rule = ruleFor(QStringLiteral("edges"));
        rule.openMaximizedToEdges = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("edges"));
        const auto second = fixture.add(QStringLiteral("edges"));
        QVERIFY(fixture.state(first).columnIndex != fixture.state(second).columnIndex);
        QCOMPARE(fixture.frame(second), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenDuringADragIsIgnoredAndTheDropStaysNormal()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto dragged = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.engine().beginWindowDrag(dragged, fixture.frame(dragged).center()));
        fixture.engine().updateWindowDrag(QPointF(200, 500), QStringLiteral("DP-1"));
        fixture.engine().setWindowFullscreen(dragged, true);
        QVERIFY(!fixture.perform(QStringLiteral("toggle-windowed-fullscreen"), {}, {{QStringLiteral("id"), QString::number(dragged)}}).ok);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(dragged).sizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.state(dragged).columnIndex, 0);
        fixture.engine().setWindowFullscreen(dragged, true);
        fixture.settle();
        QCOMPARE(fixture.frame(dragged), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenSurvivesAWorkspaceRoundTripNextToAnotherOutput()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
        fixture.add(QStringLiteral("a"));
        const auto full = fixture.add(QStringLiteral("b"));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto right = fixture.add(QStringLiteral("c"));
        fixture.engine().activateWindow(full);
        fixture.perform(QStringLiteral("fullscreen-window"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(!fixture.state(full).onActiveWorkspace);
        fixture.perform(QStringLiteral("focus-workspace-up"));
        QCOMPARE(fixture.frame(full), QRectF(0, 0, 1920, 1080));
        QCOMPARE(fixture.frame(right), QRectF(1936, 16, 936, 1048));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(full), QRectF(968, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFullscreenStack)
#include "test_layout_fullscreenstack.moc"
