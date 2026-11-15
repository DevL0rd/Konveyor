#include "overviewhelpers.h"

#include <QTest>

using namespace LayoutTest;

class TestOverviewState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void disabledAnimationsJumpStraightToOpenAndClosed();
    void animatedProgressRampsBothWays();
    void progressIsMonotonicWhileOpeningAndClosing();
    void openFlagTracksRequestsNotTheAnimation();
    void hooksSeeEveryOpenAndClose();
    void selectionIsSeededFromFocusAndCleared();
    void selectionSurvivesUnknownWindows();
    void camerasSeedOnEachOutputsActiveWorkspace();
    void disablingTheConfigClosesTheOverview();
    void emptyOverviewIsHarmless();
};

void TestOverviewState::disabledAnimationsJumpStraightToOpenAndClosed()
{
    Fixture fixture(overviewConfig());
    fixture.add(QStringLiteral("a"));

    fixture.engine().setOverviewOpen(true);
    QCOMPARE(fixture.engine().overviewProgress(), 1.0);
    QVERIFY(!fixture.engine().overviewViews().isEmpty());

    fixture.engine().setOverviewOpen(false);
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
    QVERIFY(fixture.engine().overviewViews().isEmpty());
}

void TestOverviewState::animatedProgressRampsBothWays()
{
    Fixture fixture(overviewConfig(true));
    fixture.add(QStringLiteral("a"));

    fixture.engine().setOverviewOpen(true);
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
    QVERIFY(fixture.engine().isAnimating());

    fixture.advance(100);
    const double opening = fixture.engine().overviewProgress();
    QVERIFY(opening > 0.1);
    QVERIFY(opening < 0.9);
    QVERIFY(!fixture.engine().overviewViews().isEmpty());

    fixture.advance(200);
    QCOMPARE(fixture.engine().overviewProgress(), 1.0);
    QVERIFY(!fixture.engine().isAnimating());

    fixture.engine().setOverviewOpen(false);
    QVERIFY(fixture.engine().isAnimating());
    fixture.advance(100);
    const double closing = fixture.engine().overviewProgress();
    QVERIFY(closing > 0.1);
    QVERIFY(closing < 0.9);

    fixture.advance(200);
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
    QVERIFY(fixture.engine().overviewViews().isEmpty());
}

void TestOverviewState::progressIsMonotonicWhileOpeningAndClosing()
{
    Fixture fixture(overviewConfig(true));
    fixture.add(QStringLiteral("a"));

    fixture.engine().setOverviewOpen(true);
    double previous = fixture.engine().overviewProgress();
    for (int step = 0; step < 16; ++step) {
        fixture.advance(16);
        const double current = fixture.engine().overviewProgress();
        QVERIFY(current >= previous);
        previous = current;
    }
    QCOMPARE(previous, 1.0);

    fixture.engine().setOverviewOpen(false);
    previous = fixture.engine().overviewProgress();
    for (int step = 0; step < 16; ++step) {
        fixture.advance(16);
        const double current = fixture.engine().overviewProgress();
        QVERIFY(current <= previous);
        previous = current;
    }
    QCOMPARE(previous, 0.0);
}

void TestOverviewState::openFlagTracksRequestsNotTheAnimation()
{
    Fixture fixture(overviewConfig(true));
    fixture.add(QStringLiteral("a"));

    QVERIFY(!fixture.engine().isOverviewOpen());
    fixture.engine().setOverviewOpen(true);
    QVERIFY(fixture.engine().isOverviewOpen());
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);

    fixture.advance(400);
    QVERIFY(fixture.engine().isOverviewOpen());

    fixture.engine().setOverviewOpen(false);
    QVERIFY(!fixture.engine().isOverviewOpen());
    QVERIFY(fixture.engine().overviewProgress() > 0.0);
}

void TestOverviewState::hooksSeeEveryOpenAndClose()
{
    QList<bool> calls;
    Layout::Hooks hooks;
    hooks.setOverviewOpen = [&calls](bool open) { calls.append(open); };
    Fixture fixture(overviewConfig(), QRectF(0, 0, 1920, 1080), hooks);
    fixture.add(QStringLiteral("a"));

    fixture.engine().setOverviewOpen(true);
    fixture.engine().setOverviewOpen(true);
    fixture.engine().setOverviewOpen(false);
    fixture.engine().setOverviewOpen(false);
    QCOMPARE(calls, (QList<bool> {true, false}));
}

void TestOverviewState::selectionIsSeededFromFocusAndCleared()
{
    FocusedPair pair;
    QCOMPARE(pair.fixture.focused(), std::optional(pair.b));
    pair.fixture.engine().setOverviewOpen(true);
    QCOMPARE(pair.fixture.engine().overviewSelectedWindow(), std::optional(pair.b));

    pair.fixture.engine().overviewSelectWindow(pair.a);
    QCOMPARE(pair.fixture.engine().overviewSelectedWindow(), std::optional(pair.a));

    pair.fixture.engine().setOverviewOpen(false);
    QVERIFY(!pair.fixture.engine().overviewSelectedWindow());

    fixture.engine().setOverviewOpen(true);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(b));
}

void TestOverviewState::selectionSurvivesUnknownWindows()
{
    Fixture fixture(overviewConfig());
    const auto a = fixture.add(QStringLiteral("a"));

    fixture.engine().overviewSelectWindow(9999);
    QVERIFY(!fixture.engine().overviewSelectedWindow());

    fixture.engine().setOverviewOpen(true);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(a));
    fixture.engine().overviewSelectWindow(9999);
    QCOMPARE(fixture.engine().overviewSelectedWindow(), std::optional(a));

    fixture.engine().overviewSelectWindow(std::nullopt);
    QVERIFY(!fixture.engine().overviewSelectedWindow());
}

void TestOverviewState::camerasSeedOnEachOutputsActiveWorkspace()
{
    Config::Config config = overviewConfig();
    config.workspaces = {namedWorkspace(QStringLiteral("web")), namedWorkspace(QStringLiteral("chat"))};
    Fixture fixture(config);
    fixture.addOutput(QStringLiteral("HDMI-A-1"), QRectF(1920, 0, 1280, 720));
    const auto laptop = fixture.add(QStringLiteral("laptop"));
    fixture.engine().focusOutput(QStringLiteral("HDMI-A-1"));
    fixture.add(QStringLiteral("external"));
    fixture.engine().focusOutput(QStringLiteral("DP-1"));
    QCOMPARE(fixture.state(laptop).output, QStringLiteral("DP-1"));

    fixture.engine().setOverviewOpen(true);
    const QList<Layout::OverviewView> views = fixture.engine().overviewViews();
    QCOMPARE(views.size(), 2);
    for (const Layout::OverviewView &view : views) {
        const QRectF viewport = QRectF(view.camera, view.outputGeometry.size());
        const QRectF active = view.workspaceRects.at(view.activeWorkspace).translated(-view.outputGeometry.topLeft());
        QVERIFY(viewport.intersects(active));
    }

    const Layout::OverviewView *laptopView = viewFor(views, QStringLiteral("DP-1"));
    const QPointF laptopCamera = laptopView->camera;
    const QPointF externalCamera = viewFor(views, QStringLiteral("HDMI-A-1"))->camera;
    fixture.engine().overviewPan(QStringLiteral("HDMI-A-1"), QPointF(0, 30));
    const QList<Layout::OverviewView> moved = fixture.engine().overviewViews();
    QCOMPARE(viewFor(moved, QStringLiteral("DP-1"))->camera, laptopCamera);
    QCOMPARE(viewFor(moved, QStringLiteral("HDMI-A-1"))->camera, externalCamera + QPointF(0, 30));
}

void TestOverviewState::disablingTheConfigClosesTheOverview()
{
    Fixture fixture(overviewConfig());
    fixture.add(QStringLiteral("a"));
    fixture.engine().setOverviewOpen(true);
    QVERIFY(fixture.engine().isOverviewOpen());
    QCOMPARE(fixture.engine().overviewProgress(), 1.0);

    Config::Config disabled = overviewConfig();
    disabled.overview.enabled = false;
    fixture.setConfig(disabled);
    QVERIFY(!fixture.engine().isOverviewEnabled());
    QVERIFY(!fixture.engine().isOverviewOpen());
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
    QVERIFY(fixture.engine().overviewViews().isEmpty());
    QVERIFY(!fixture.engine().overviewSelectedWindow());
}

void TestOverviewState::emptyOverviewIsHarmless()
{
    Fixture fixture(overviewConfig());
    fixture.engine().setOverviewOpen(true);
    QVERIFY(!fixture.engine().overviewViews().isEmpty());
    QVERIFY(!fixture.engine().overviewSelectedWindow());

    fixture.engine().overviewMoveSelection(Layout::OverviewDirection::Down);
    fixture.engine().overviewCycleSelection(true);
    QVERIFY(!fixture.engine().overviewSelectedWindow());
    QVERIFY(!fixture.engine().overviewActivateSelected());
    fixture.engine().overviewBeginWorkspaceDrag(QStringLiteral("DP-1"), 99);
    fixture.engine().overviewEndDrag();
    VERIFY_INVARIANTS(fixture);
}

QTEST_MAIN(TestOverviewState)
#include "test_overview_state.moc"
