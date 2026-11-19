#include "helpers.h"

#include <QTest>

using namespace LayoutTest;

namespace
{

Config::Config overviewConfig()
{
    Config::Config config = instantConfig();
    config.overview.enabled = true;
    return config;
}

std::optional<Layout::OverviewWindow> selectedWindow(const QList<Layout::OverviewView> &views, std::optional<Layout::WindowId> id)
{
    for (const Layout::OverviewView &view : views) {
        for (const Layout::OverviewWindow &window : view.windows) {
            if (window.id == id) {
                return window;
            }
        }
    }
    return std::nullopt;
}

struct Stacked
{
    Fixture fixture {overviewConfig()};
    Layout::WindowId a = fixture.add(QStringLiteral("a"));
    Layout::WindowId b = fixture.add(QStringLiteral("b"));
    Layout::WindowId c = addOnNextWorkspace(QStringLiteral("c"));
    Layout::WindowId d = addOnNextWorkspace(QStringLiteral("d"));

    Layout::WindowId addOnNextWorkspace(const QString &appId)
    {
        fixture.perform(QStringLiteral("focus-workspace-down"));
        return fixture.add(appId);
    }
};

}

class TestOverviewKeyboard : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void selectionMovesInEachDirection();
    void selectionStopsAtTheEdges();
    void selectionWithoutCurrentStartsAtTheEdge();
    void selectionCyclesForwardAndBackward();
    void selectionScrollsOffscreenWindowIntoView();
    void activatingClosesTheOverviewAndFocuses();
    void activatingWithoutSelectionKeepsTheOverviewOpen();
};

void TestOverviewKeyboard::selectionMovesInEachDirection()
{
    Stacked stacked;
    Fixture &fixture = stacked.fixture;

    fixture.engine().setOverviewOpen(true);
    fixture.engine().overviewSelectWindow(stacked.b);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.b));

    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Left);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.a));

    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Right);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.b));

    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Down);
    const auto below = selectedWindow(fixture.engine().overviewViews(), fixture.engine().overviewSelectedWindow());
    QVERIFY(below);
    QCOMPARE(below->workspaceIndex, 2);

    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Up);
    const auto above = selectedWindow(fixture.engine().overviewViews(), fixture.engine().overviewSelectedWindow());
    QVERIFY(above);
    QCOMPARE(above->workspaceIndex, 1);
}

void TestOverviewKeyboard::selectionStopsAtTheEdges()
{
    Stacked stacked;
    Fixture &fixture = stacked.fixture;

    fixture.engine().setOverviewOpen(true);
    fixture.engine().overviewSelectWindow(stacked.a);
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Up);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.a));
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Left);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.a));

    fixture.engine().overviewSelectWindow(stacked.d);
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Down);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.d));
}

void TestOverviewKeyboard::selectionWithoutCurrentStartsAtTheEdge()
{
    Stacked stacked;
    Fixture &fixture = stacked.fixture;
    fixture.engine().setOverviewOpen(true);

    fixture.engine().overviewSelectWindow(std::nullopt);
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Left);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.a));

    fixture.engine().overviewSelectWindow(std::nullopt);
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Right);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.b));

    fixture.engine().overviewSelectWindow(std::nullopt);
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Down);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(stacked.d));

    fixture.engine().overviewSelectWindow(std::nullopt);
    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Up);
    const auto top = selectedWindow(fixture.engine().overviewViews(), fixture.engine().overviewSelectedWindow());
    QVERIFY(top);
    QCOMPARE(top->workspaceIndex, 1);
}

void TestOverviewKeyboard::selectionCyclesForwardAndBackward()
{
    Stacked stacked;
    Fixture &fixture = stacked.fixture;
    fixture.engine().setOverviewOpen(true);

    QList<Layout::WindowId> order;
    for (const Layout::OverviewWindow &window : fixture.engine().overviewViews().first().windows) {
        order.append(window.id);
    }
    QCOMPARE(order.size(), 4);

    fixture.engine().overviewSelectWindow(std::nullopt);
    for (const Layout::WindowId expected : order) {
        fixture.engine().overviewCycleSelection(true);
        QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(expected));
    }
    fixture.engine().overviewCycleSelection(true);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(order.first()));

    fixture.engine().overviewCycleSelection(false);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(order.last()));

    fixture.engine().overviewSelectWindow(std::nullopt);
    fixture.engine().overviewCycleSelection(false);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(order.last()));
}

void TestOverviewKeyboard::selectionScrollsOffscreenWindowIntoView()
{
    Stacked stacked;
    Fixture &fixture = stacked.fixture;
    QCOMPARE(fixture.state(stacked.a).workspaceIndex, 1);
    QCOMPARE(fixture.state(stacked.d).workspaceIndex, 3);

    fixture.engine().setOverviewOpen(true);
    fixture.engine().overviewPan(QStringLiteral("DP-1"), QPointF(0, 1e6));
    const Layout::OverviewView bottom = fixture.engine().overviewViews().first();
    QVERIFY(bottom.camera.y() > 0.0);

    fixture.engine().overviewSelectWindow(stacked.a);
    const Layout::OverviewView revealed = fixture.engine().overviewViews().first();
    QVERIFY(revealed.camera.y() < bottom.camera.y());
    const QRectF viewport(revealed.camera, revealed.outputGeometry.size());
    const auto window = selectedWindow(fixture.engine().overviewViews(), std::optional(stacked.a));
    QVERIFY(window);
    QVERIFY(viewport.contains(window->rect.center()));
    QVERIFY(revealed.workspaceRects.first().intersects(viewport));
}

void TestOverviewKeyboard::activatingClosesTheOverviewAndFocuses()
{
    Stacked stacked;
    Fixture &fixture = stacked.fixture;
    fixture.engine().setOverviewOpen(true);
    fixture.engine().overviewSelectWindow(stacked.c);
    QVERIFY(fixture.engine().overviewActivateSelected());
    QCOMPARE(fixture.focused(), std::optional(stacked.c));
    QCOMPARE(fixture.state(stacked.c).workspaceIndex, 2);
    QVERIFY(!fixture.engine().isOverviewOpen());
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
    VERIFY_INVARIANTS(fixture);
}

void TestOverviewKeyboard::activatingWithoutSelectionKeepsTheOverviewOpen()
{
    Fixture fixture(overviewConfig());
    fixture.add(QStringLiteral("a"));
    fixture.engine().setOverviewOpen(true);
    fixture.engine().overviewSelectWindow(std::nullopt);
    QVERIFY(!fixture.engine().overviewActivateSelected());
    QVERIFY(fixture.engine().isOverviewOpen());
}

QTEST_MAIN(TestOverviewKeyboard)
#include "test_overview_keyboard.moc"
