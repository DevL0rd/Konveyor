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

const Layout::OverviewView *viewFor(const QList<Layout::OverviewView> &views, const QString &output)
{
    for (const Layout::OverviewView &view : views) {
        if (view.output == output) {
            return &view;
        }
    }
    return nullptr;
}

std::optional<Layout::OverviewWindow> windowFor(const Layout::OverviewView &view, Layout::WindowId id)
{
    for (const Layout::OverviewWindow &window : view.windows) {
        if (window.id == id) {
            return window;
        }
    }
    return std::nullopt;
}

struct Scene
{
    Fixture fixture {overviewConfig()};
    Layout::WindowId a = fixture.add(QStringLiteral("a"));
    Layout::WindowId b = fixture.add(QStringLiteral("b"));
    Layout::WindowId c = addBelow(QStringLiteral("c"));

    Layout::WindowId addBelow(const QString &appId)
    {
        fixture.perform(QStringLiteral("focus-workspace-down"));
        return fixture.add(appId);
    }

    void open()
    {
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.engine().setOverviewOpen(true);
    }
};

}

class TestOverviewPointer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void panDeltaFollowsOrientationAndShift();
    void panDeltaScalesTouchpadDeltas();
    void pressOnWindowSelectsAndReleaseActivates();
    void clickWithoutTravelActivatesInsteadOfDragging();
    void pressOnCardSwitchesWorkspace();
    void pressOnTheBackdropClosesTheOverview();
    void travelStartsAWindowDragAndDropMovesIt();
    void cardTravelReordersWorkspaces();
    void motionWithoutPressIsIgnored();
    void pointerDoesNothingWhenClosed();
    void pressUsesTheOutputUnderThePointer();
    void verticalScrollStepsWorkspacesAndSelectsCenteredWindow();
    void horizontalScrollMovesColumnsAndUpdatesSelection();
    void hoverSelectsWindowAndToggleExitFocusesSelection();
};

void TestOverviewPointer::panDeltaFollowsOrientationAndShift()
{
    QCOMPARE(Layout::overviewPanDelta(Qt::Vertical, 15.0, 0, false, false), QPointF(0, 140));
    QCOMPARE(Layout::overviewPanDelta(Qt::Vertical, 0.0, 240, false, false), QPointF(0, 280));
    QCOMPARE(Layout::overviewPanDelta(Qt::Vertical, -15.0, 0, false, true), QPointF(-140, 0));
    QCOMPARE(Layout::overviewPanDelta(Qt::Horizontal, 30.0, 0, false, false), QPointF(280, 0));
    QVERIFY(Layout::overviewPanDelta(Qt::Vertical, 0.0, 0, false, false).isNull());
}

void TestOverviewPointer::panDeltaScalesTouchpadDeltas()
{
    QCOMPARE(Layout::overviewPanDelta(Qt::Vertical, 15.0, 0, true, false), QPointF(0, 45));
    QCOMPARE(Layout::overviewPanDelta(Qt::Vertical, 0.0, 60, true, false), QPointF(0, 22.5));
    QCOMPARE(Layout::overviewPanDelta(Qt::Horizontal, -15.0, 0, true, false), QPointF(-45, 0));
}

void TestOverviewPointer::pressOnWindowSelectsAndReleaseActivates()
{
    Scene scene;
    scene.open();
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();
    const auto window = windowFor(view, scene.a);
    QVERIFY(window);

    scene.fixture.engine().overviewPointerPressed(window->rect.center());
    QVERIFY(scene.fixture.engine().overviewPointerActive());
    QCOMPARE(scene.fixture.engine().overviewSelectedWindow(), std::optional(scene.a));

    scene.fixture.engine().overviewPointerReleased();
    QVERIFY(!scene.fixture.engine().overviewPointerActive());
    QVERIFY(!scene.fixture.engine().isOverviewOpen());
    QCOMPARE(scene.fixture.engine().focusedWindow(), std::optional(scene.a));
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::clickWithoutTravelActivatesInsteadOfDragging()
{
    Scene scene;
    scene.open();
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();
    const auto window = windowFor(view, scene.b);
    QVERIFY(window);

    scene.fixture.engine().overviewPointerPressed(window->rect.center());
    QVERIFY(!scene.fixture.engine().overviewPointerMoved(window->rect.center() + QPointF(1, 1)));
    QVERIFY(!scene.fixture.engine().movingWindow());
    scene.fixture.engine().overviewPointerReleased();

    QVERIFY(!scene.fixture.engine().isOverviewOpen());
    QCOMPARE(scene.fixture.engine().focusedWindow(), std::optional(scene.b));
    QCOMPARE(scene.fixture.state(scene.b).workspaceIndex, 1);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::pressOnCardSwitchesWorkspace()
{
    Scene scene;
    scene.open();
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();
    QCOMPARE(view.workspaceRects.size(), 3);

    QVERIFY(!view.workspaceRects[1].contains(windowFor(view, scene.a)->rect.center()));
    scene.fixture.engine().overviewPointerPressed(view.workspaceRects[1].center());
    QVERIFY(scene.fixture.engine().overviewPointerActive());
    scene.fixture.engine().overviewPointerReleased();

    QVERIFY(!scene.fixture.engine().isOverviewOpen());
    scene.fixture.engine().setOverviewOpen(true);
    QCOMPARE(scene.fixture.engine().overviewViews().first().activeWorkspace, 1);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::pressOnTheBackdropClosesTheOverview()
{
    Scene scene;
    scene.open();
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();
    const QPointF backdrop = view.outputGeometry.bottomRight() - QPointF(1, 1);
    for (const QRectF &card : view.workspaceRects) {
        QVERIFY(!card.contains(backdrop));
    }

    scene.fixture.engine().overviewPointerPressed(backdrop);
    QVERIFY(!scene.fixture.engine().overviewPointerActive());
    QVERIFY(!scene.fixture.engine().isOverviewOpen());
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::travelStartsAWindowDragAndDropMovesIt()
{
    Scene scene;
    const Layout::WorkspaceId target = scene.fixture.state(scene.c).workspace;
    scene.open();
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();
    const auto source = windowFor(view, scene.b);
    QVERIFY(source);

    scene.fixture.engine().overviewPointerPressed(source->rect.center());
    QVERIFY(scene.fixture.engine().overviewPointerMoved(view.workspaceRects[1].center()));
    QCOMPARE(scene.fixture.engine().movingWindow(), std::optional(scene.b));
    QCOMPARE(scene.fixture.engine().overviewViews().first().draggedWindow, std::optional(scene.b));

    scene.fixture.engine().overviewPointerReleased();
    scene.fixture.settle();
    QCOMPARE(scene.fixture.state(scene.b).workspace, target);
    QCOMPARE(scene.fixture.state(scene.b).workspaceIndex, 2);
    QVERIFY(!scene.fixture.engine().movingWindow());
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::cardTravelReordersWorkspaces()
{
    Scene scene;
    scene.open();
    QCOMPARE(scene.fixture.state(scene.a).workspaceIndex, 1);
    QCOMPARE(scene.fixture.state(scene.c).workspaceIndex, 2);
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();

    scene.fixture.engine().overviewPointerPressed(view.workspaceRects[0].center());
    const QPointF between
        = QPointF(view.workspaceRects[0].center().x(), (view.workspaceRects[0].center().y() + view.workspaceRects[1].center().y()) / 2.0);
    QVERIFY(scene.fixture.engine().overviewPointerMoved(between));
    QCOMPARE(scene.fixture.engine().overviewViews().first().draggedWorkspace, 0);

    scene.fixture.engine().overviewPointerReleased();
    scene.fixture.settle();
    QCOMPARE(scene.fixture.state(scene.a).workspaceIndex, 2);
    QCOMPARE(scene.fixture.state(scene.c).workspaceIndex, 1);
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::motionWithoutPressIsIgnored()
{
    Scene scene;
    scene.open();
    QVERIFY(!scene.fixture.engine().overviewPointerMoved(QPointF(10, 10)));
    QVERIFY(!scene.fixture.engine().overviewPointerActive());
    scene.fixture.engine().overviewPointerReleased();
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::pointerDoesNothingWhenClosed()
{
    Fixture fixture(overviewConfig());
    const auto a = fixture.add(QStringLiteral("a"));
    fixture.engine().overviewPointerPressed(QPointF(10, 10));
    QVERIFY(!fixture.engine().overviewPointerActive());
    QVERIFY(!fixture.engine().overviewPointerMoved(QPointF(50, 50)));
    fixture.engine().overviewPointerReleased();
    QVERIFY(!fixture.engine().isOverviewOpen());
    QCOMPARE(fixture.engine().focusedWindow(), std::optional(a));
    VERIFY_INVARIANTS(fixture);
}

void TestOverviewPointer::pressUsesTheOutputUnderThePointer()
{
    Config::Config config = overviewConfig();
    Fixture fixture(config);
    fixture.addOutput(QStringLiteral("HDMI-A-1"), QRectF(1920, 0, 1280, 720));
    fixture.engine().focusOutput(QStringLiteral("HDMI-A-1"));
    const auto there = fixture.add(QStringLiteral("there"));
    fixture.engine().focusOutput(QStringLiteral("DP-1"));
    fixture.add(QStringLiteral("here"));
    fixture.engine().setOverviewOpen(true);

    const Layout::OverviewView *external = viewFor(fixture.engine().overviewViews(), QStringLiteral("HDMI-A-1"));
    QVERIFY(external);
    const auto window = windowFor(*external, there);
    QVERIFY(window);
    QVERIFY(!external->outputGeometry.contains(QPointF(10, 10)));

    fixture.engine().overviewPointerPressed(window->rect.center());
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(there));
    fixture.engine().overviewPointerReleased();
    fixture.settle();
    QCOMPARE(fixture.engine().focusedWindow(), std::optional(there));
    QCOMPARE(fixture.state(there).output, QStringLiteral("HDMI-A-1"));
    VERIFY_INVARIANTS(fixture);
}

void TestOverviewPointer::verticalScrollStepsWorkspacesAndSelectsCenteredWindow()
{
    Scene scene;
    scene.open();
    const Layout::OverviewView before = scene.fixture.engine().overviewViews().first();
    QCOMPARE(before.activeWorkspace, 0);

    scene.fixture.engine().overviewScroll(QStringLiteral("DP-1"), Qt::Vertical, 15.0, 120, false);
    const Layout::OverviewView down = scene.fixture.engine().overviewViews().first();
    QCOMPARE(down.activeWorkspace, 1);
    QCOMPARE(scene.fixture.engine().overviewSelectedWindow(), std::optional(scene.c));

    scene.fixture.engine().overviewScroll(QStringLiteral("DP-1"), Qt::Vertical, -15.0, -120, false);
    const Layout::OverviewView up = scene.fixture.engine().overviewViews().first();
    QCOMPARE(up.activeWorkspace, 0);
    QVERIFY(scene.fixture.engine().overviewSelectedWindow().has_value());
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::horizontalScrollMovesColumnsAndUpdatesSelection()
{
    Scene scene;
    scene.open();
    scene.fixture.engine().overviewSelectWindow(scene.b);
    QCOMPARE(scene.fixture.engine().overviewSelectedWindow(), std::optional(scene.b));

    scene.fixture.engine().overviewScroll(QStringLiteral("DP-1"), Qt::Horizontal, -15.0, -120, false);
    QCOMPARE(scene.fixture.engine().overviewSelectedWindow(), std::optional(scene.a));

    scene.fixture.engine().overviewScroll(QStringLiteral("DP-1"), Qt::Horizontal, 15.0, 120, false);
    QCOMPARE(scene.fixture.engine().overviewSelectedWindow(), std::optional(scene.b));
    VERIFY_INVARIANTS(scene.fixture);
}

void TestOverviewPointer::hoverSelectsWindowAndToggleExitFocusesSelection()
{
    Scene scene;
    scene.open();
    const Layout::OverviewView view = scene.fixture.engine().overviewViews().first();
    const auto winA = windowFor(view, scene.a);
    QVERIFY(winA);

    QVERIFY(scene.fixture.engine().overviewHover(winA->rect.center()));
    QCOMPARE(scene.fixture.engine().overviewSelectedWindow(), std::optional(scene.a));

    scene.fixture.perform(QStringLiteral("toggle-overview"));
    scene.fixture.settle();
    QVERIFY(!scene.fixture.engine().isOverviewOpen());
    QCOMPARE(scene.fixture.engine().focusedWindow(), std::optional(scene.a));
    VERIFY_INVARIANTS(scene.fixture);
}

QTEST_MAIN(TestOverviewPointer)
#include "test_overview_pointer.moc"
