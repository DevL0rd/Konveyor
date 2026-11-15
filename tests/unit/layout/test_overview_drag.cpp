#include "overviewhelpers.h"

#include <QTest>

using namespace LayoutTest;

class TestOverviewDrag : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void windowDragShowsGhostOnTheTargetCard();
    void windowDropMovesItToTheTargetWorkspace();
    void windowDropOnBlankCardCreatesTheWorkspace();
    void windowDragAcrossOutputsMovesTheWindow();
    void droppingOnTheSameWorkspaceKeepsItThere();
    void cancelRestoresTheOriginalWorkspace();
    void workspaceDragReordersCards();
    void workspaceDragCancelKeepsTheOrder();
    void dragActionsDoNothingWhenClosed();
};

void TestOverviewDrag::windowDragShowsGhostOnTheTargetCard()
{
    Scene scene;
    const Layout::OverviewView view = scene.openView();
    QVERIFY(windowFor(view, scene.b));
    QCOMPARE(view.workspaceRects.size(), 3);

    scene.grab(scene.b, view);
    QCOMPARE(scene.fixture.engine().movingWindow(), std::optional(scene.b));
    scene.fixture.engine().overviewUpdateDrag(view.workspaceRects[1].center());

    const Layout::OverviewView during = scene.fixture.engine().overviewViews().first();
    QCOMPARE(during.draggedWindow, std::optional(scene.b));
    QCOMPARE(during.dropWorkspace, 1);
    const auto ghost = windowFor(during, scene.b);
    QVERIFY(ghost);
    QCOMPARE(ghost->workspaceIndex, 2);
    QVERIFY(ghost->selected);
    QVERIFY(windowFor(during, scene.a));
    QVERIFY(!windowFor(during, scene.c) || windowFor(during, scene.c)->workspaceIndex == 2);
    QVERIFY(during.dropWorkspace >= 0);
}

void TestOverviewDrag::windowDropMovesItToTheTargetWorkspace()
{
    Scene scene;
    const Layout::WorkspaceId target = scene.fixture.state(scene.c).workspace;
    const Layout::OverviewView view = scene.openView();

    scene.grab(scene.b, view);
    scene.fixture.engine().overviewUpdateDrag(view.workspaceRects[1].center());
    scene.finish();

    QCOMPARE(scene.fixture.state(scene.b).workspace, target);
    QCOMPARE(scene.fixture.state(scene.b).workspaceIndex, 2);
    QCOMPARE(scene.fixture.state(scene.b).output, QStringLiteral("DP-1"));
    QCOMPARE(scene.fixture.state(scene.a).workspaceIndex, 1);
    QVERIFY(!scene.fixture.engine().movingWindow());
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewDrag::windowDropOnBlankCardCreatesTheWorkspace()
{
    Scene scene;
    const QList<Layout::WorkspaceState> before = scene.fixture.engine().workspaceStates();
    QVERIFY(before.size() >= 3);
    const Layout::WorkspaceId blank = before.last().id;
    const Layout::OverviewView view = scene.openView();
    const int blankCard = static_cast<int>(view.workspaceRects.size()) - 1;

    scene.grab(scene.b, view);
    scene.fixture.engine().overviewUpdateDrag(view.workspaceRects[blankCard].center());
    scene.finish();

    QCOMPARE(scene.fixture.state(scene.b).workspace, blank);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewDrag::windowDragAcrossOutputsMovesTheWindow()
{
    Config::Config config = overviewConfig();
    Fixture fixture(config);
    fixture.addOutput(QStringLiteral("HDMI-A-1"), QRectF(1920, 0, 1280, 720));
    fixture.engine().focusOutput(QStringLiteral("HDMI-A-1"));
    const auto there = fixture.add(QStringLiteral("there"));
    const Layout::WorkspaceId target = fixture.state(there).workspace;
    fixture.engine().focusOutput(QStringLiteral("DP-1"));
    const auto here = fixture.add(QStringLiteral("here"));
    fixture.add(QStringLiteral("other"));
    fixture.engine().setOverviewOpen(true);

    const QList<Layout::OverviewView> views = fixture.engine().overviewViews();
    const Layout::OverviewView *laptop = viewFor(views, QStringLiteral("DP-1"));
    const Layout::OverviewView *external = viewFor(views, QStringLiteral("HDMI-A-1"));
    QVERIFY(laptop);
    QVERIFY(external);
    const auto source = windowFor(*laptop, here);
    QVERIFY(source);

    fixture.engine().overviewBeginWindowDrag(here, QStringLiteral("DP-1"), source->rect.center());
    fixture.engine().overviewUpdateDrag(external->workspaceRects.first().center());

    const Layout::OverviewView *during = viewFor(fixture.engine().overviewViews(), QStringLiteral("HDMI-A-1"));
    QVERIFY(during);
    QCOMPARE(during->dropWorkspace, 0);
    QCOMPARE(during->draggedWindow, std::optional(here));

    fixture.engine().overviewEndDrag();
    fixture.settle();
    QCOMPARE(fixture.state(here).output, QStringLiteral("HDMI-A-1"));
    QCOMPARE(fixture.state(here).workspace, target);
    VERIFY_INVARIANTS(fixture);
}

void TestOverviewDrag::droppingOnTheSameWorkspaceKeepsItThere()
{
    Scene scene;
    const Layout::WorkspaceId origin = scene.fixture.state(scene.b).workspace;
    const Layout::OverviewView view = scene.openView();
    const QPointF center = scene.grab(scene.b, view);

    scene.fixture.engine().overviewUpdateDrag(center + QPointF(30, 10));
    scene.finish();

    QCOMPARE(scene.fixture.state(scene.b).workspace, origin);
    QCOMPARE(scene.fixture.state(scene.b).workspaceIndex, 1);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewDrag::cancelRestoresTheOriginalWorkspace()
{
    Scene scene;
    const Layout::WorkspaceId origin = scene.fixture.state(scene.b).workspace;
    const Layout::OverviewView view = scene.openView();
    scene.grab(scene.b, view);

    scene.fixture.engine().overviewUpdateDrag(view.workspaceRects[1].center());
    scene.cancel();

    QCOMPARE(scene.fixture.state(scene.b).workspace, origin);
    QCOMPARE(scene.fixture.state(scene.b).workspaceIndex, 1);
    QVERIFY(!scene.fixture.engine().movingWindow());
    const Layout::OverviewView after = scene.fixture.engine().overviewViews().first();
    QVERIFY(!after.draggedWindow);
    QCOMPARE(after.dropWorkspace, -1);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewDrag::workspaceDragReordersCards()
{
    Scene scene;
    const Layout::OverviewView view = scene.openView();
    QCOMPARE(scene.fixture.state(scene.a).workspaceIndex, 1);
    QCOMPARE(scene.fixture.state(scene.c).workspaceIndex, 2);

    const QPointF between = Scene::betweenCards(view, 0, 1);
    scene.fixture.engine().overviewBeginWorkspaceDrag(QStringLiteral("DP-1"), 0);
    scene.fixture.engine().overviewUpdateDrag(between);

    const Layout::OverviewView during = scene.fixture.engine().overviewViews().first();
    QCOMPARE(during.draggedWorkspace, 0);
    QCOMPARE(during.dropWorkspace, 1);

    scene.finish();
    QCOMPARE(scene.fixture.state(scene.c).workspaceIndex, 1);
    QCOMPARE(scene.fixture.state(scene.a).workspaceIndex, 2);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewDrag::workspaceDragCancelKeepsTheOrder()
{
    Scene scene;
    const Layout::OverviewView view = scene.openView();
    const QPointF between = Scene::betweenCards(view, 0, 1);
    scene.fixture.engine().overviewBeginWorkspaceDrag(QStringLiteral("DP-1"), 0);
    scene.fixture.engine().overviewUpdateDrag(between);
    scene.cancel();

    QCOMPARE(scene.fixture.state(scene.a).workspaceIndex, 1);
    QCOMPARE(scene.fixture.state(scene.c).workspaceIndex, 2);
    const Layout::OverviewView after = scene.fixture.engine().overviewViews().first();
    QCOMPARE(after.draggedWorkspace, -1);
    QCOMPARE(after.dropWorkspace, -1);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewDrag::dragActionsDoNothingWhenClosed()
{
    Fixture fixture(overviewConfig());
    fixture.add(QStringLiteral("a"));
    fixture.engine().overviewBeginWorkspaceDrag(QStringLiteral("DP-1"), 0);
    fixture.engine().overviewUpdateDrag(QPointF(10, 10));
    fixture.engine().overviewEndDrag();
    QVERIFY(!fixture.engine().movingWindow());
    const Layout::OverviewView view
        = fixture.engine().overviewViews().isEmpty() ? Layout::OverviewView() : fixture.engine().overviewViews().first();
    QCOMPARE(view.draggedWorkspace, -1);
    fixture.engine().overviewBeginWindowDrag(1, QStringLiteral("DP-1"), QPointF(10, 10));
    QVERIFY(!fixture.engine().movingWindow());
    VERIFY_INVARIANTS(fixture);
}

QTEST_MAIN(TestOverviewDrag)
#include "test_overview_drag.moc"
