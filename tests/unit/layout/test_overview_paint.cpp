#include <QTest>

#include <layout/overview/overviewpaint.h>

#include <cmath>
#include <limits>

namespace Layout = Konveyor::Layout;

namespace
{

Layout::OverviewView sampleView()
{
    Layout::OverviewView view;
    view.output = QStringLiteral("DP-1");
    view.outputGeometry = QRectF(0, 0, 1000, 600);
    view.reserved = QMarginsF(0, 40, 0, 0);
    view.activeWorkspace = 1;
    view.workspaceRects = {QRectF(20, 20, 400, 240), QRectF(20, 280, 400, 240), QRectF(20, 540, 400, 240)};
    view.offscreen = {Layout::OverviewSide::Left, Layout::OverviewSide::Bottom};
    return view;
}

Layout::OverviewWindow window(Layout::WindowId id, const QRectF &rect, int workspaceIndex, bool selected = false)
{
    Layout::OverviewWindow result;
    result.id = id;
    result.rect = rect;
    result.workspaceIndex = workspaceIndex;
    result.selected = selected;
    return result;
}

}

class TestOverviewPaint : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void blendClampsProgress();
    void rectInterpolatesBetweenFrames();
    void outlineInflatesWindow();
    void closedOverviewPaintsNothing();
    void cardsCarryFlags();
    void cardsFadeWithProgress();
    void windowsBlendFromTheirNormalFrame();
    void missingNormalFrameFallsBackToOverviewRect();
    void draggedWindowStaysOpaque();
    void edgesFollowOffscreenSides();
    void edgesSkipAnEmptyWorkArea();
    void zoomsWindowsFromOutputCenter();
};

void TestOverviewPaint::blendClampsProgress()
{
    QCOMPARE(Layout::overviewPaintBlend(-0.5), 0.0);
    QCOMPARE(Layout::overviewPaintBlend(0.25), 0.25);
    QCOMPARE(Layout::overviewPaintBlend(4.0), 1.0);
    QCOMPARE(Layout::overviewPaintBlend(std::numeric_limits<double>::quiet_NaN()), 0.0);
}

void TestOverviewPaint::rectInterpolatesBetweenFrames()
{
    const QRectF from(0, 0, 100, 100);
    const QRectF to(100, 200, 300, 400);
    QCOMPARE(Layout::overviewPaintRect(from, to, 0.0), from);
    QCOMPARE(Layout::overviewPaintRect(from, to, 1.0), to);
    QCOMPARE(Layout::overviewPaintRect(from, to, 0.5), QRectF(50, 100, 200, 250));
}

void TestOverviewPaint::outlineInflatesWindow()
{
    const QRectF windowRect(10, 20, 100, 200);
    QCOMPARE(Layout::overviewOutlineRect(windowRect, 4.0), QRectF(6, 16, 108, 208));
    QCOMPARE(Layout::overviewOutlineRect(windowRect, 0.0), windowRect);
    QCOMPARE(Layout::overviewOutlineRect(windowRect, std::numeric_limits<double>::infinity()), windowRect);
}

void TestOverviewPaint::closedOverviewPaintsNothing()
{
    const Layout::OverviewView view = sampleView();
    const Layout::OverviewPaintView paint = Layout::overviewPaintView(view, {}, 0.0);
    QCOMPARE(paint.blend, 0.0);
    QVERIFY(paint.cards.isEmpty());
    QVERIFY(paint.windows.isEmpty());
    QVERIFY(paint.edges.isEmpty());
}

void TestOverviewPaint::cardsCarryFlags()
{
    Layout::OverviewView view = sampleView();
    view.dropWorkspace = 2;
    view.draggedWorkspace = 0;

    const Layout::OverviewPaintView paint = Layout::overviewPaintView(view, {}, 1.0);
    QCOMPARE(paint.cards.size(), 3);
    QVERIFY(paint.cards[1].active);
    QVERIFY(paint.cards[2].hovered);
    QVERIFY(paint.cards[0].dragged);
    QVERIFY(!paint.cards[0].active);
}

void TestOverviewPaint::cardsFadeWithProgress()
{
    const Layout::OverviewView view = sampleView();
    const Layout::OverviewPaintView paint = Layout::overviewPaintView(view, {}, 0.5);
    QCOMPARE(paint.blend, 0.5);
    QCOMPARE(paint.scrimOpacity, 0.5);
    QCOMPARE(paint.cards.size(), 3);
    for (const Layout::OverviewPaintCard &card : paint.cards) {
        const double base = card.active ? Layout::kOverviewCardActiveAlpha : Layout::kOverviewCardAlpha;
        QCOMPARE(card.opacity, base * 0.5);
    }
}

void TestOverviewPaint::windowsBlendFromTheirNormalFrame()
{
    Layout::OverviewView view = sampleView();
    view.windows = {window(7, QRectF(200, 100, 100, 50), 2), window(9, QRectF(300, 400, 100, 50), 3, true)};
    const Layout::OverviewNormalFrames normal {{7, QRectF(0, 0, 400, 200)}, {9, QRectF(100, 700, 400, 200)}};

    const Layout::OverviewPaintView paint = Layout::overviewPaintView(view, normal, 0.5);
    QCOMPARE(paint.windows.size(), 2);
    QCOMPARE(paint.windows[0].id, Layout::WindowId(7));
    QCOMPARE(paint.windows[0].workspaceIndex, 2);
    QCOMPARE(paint.windows[0].rect, QRectF(100, 50, 250, 125));
    QVERIFY(!paint.windows[0].selected);
    QVERIFY(paint.windows[1].selected);
    QCOMPARE(paint.windows[1].rect, QRectF(200, 550, 250, 125));
}

void TestOverviewPaint::missingNormalFrameFallsBackToOverviewRect()
{
    Layout::OverviewView view = sampleView();
    view.windows = {window(11, QRectF(50, 60, 80, 90), 1)};
    const Layout::OverviewPaintView paint = Layout::overviewPaintView(view, {}, 0.25);
    QCOMPARE(paint.windows[0].rect, QRectF(50, 60, 80, 90));
    QCOMPARE(paint.windows[0].opacity, 0.25);
}

void TestOverviewPaint::draggedWindowStaysOpaque()
{
    Layout::OverviewView view = sampleView();
    view.windows = {window(13, QRectF(70, 80, 90, 100), 1)};
    view.draggedWindow = 13;
    const Layout::OverviewPaintView paint = Layout::overviewPaintView(view, {}, 0.25);
    QVERIFY(paint.windows[0].dragging);
    QCOMPARE(paint.windows[0].opacity, 1.0);
}

void TestOverviewPaint::edgesFollowOffscreenSides()
{
    const Layout::OverviewView view = sampleView();
    const QList<Layout::OverviewPaintEdge> edges = Layout::overviewPaintEdges(view);
    QCOMPARE(edges.size(), 2);
    QCOMPARE(edges[0].side, Layout::OverviewSide::Left);
    QCOMPARE(edges[1].side, Layout::OverviewSide::Bottom);
    const QRectF area = view.outputGeometry.adjusted(0, 40, 0, 0);
    for (const Layout::OverviewPaintEdge &edge : edges) {
        QVERIFY(area.contains(edge.rect));
        QVERIFY(edge.rect.width() > 0.0);
        QVERIFY(edge.rect.height() > 0.0);
    }
    QVERIFY(edges[0].rect.left() >= area.left());
    QVERIFY(edges[1].rect.bottom() <= area.bottom());
}

void TestOverviewPaint::edgesSkipAnEmptyWorkArea()
{
    Layout::OverviewView view = sampleView();
    view.outputGeometry = QRectF(0, 0, 100, 40);
    view.reserved = QMarginsF(0, 40, 0, 0);
    QVERIFY(Layout::overviewPaintEdges(view).isEmpty());
}

void TestOverviewPaint::zoomsWindowsFromOutputCenter()
{
    Layout::OverviewView view = sampleView();
    view.outputGeometry = QRectF(0, 0, 1000, 600);
    view.scale = 0.5;
    Layout::OverviewWindow win = window(1, QRectF(300, 200, 400, 200), 1);
    win.normalRect = QRectF(100, 100, 800, 400);
    view.windows = {win};

    const Layout::OverviewPaintView half = Layout::overviewPaintView(view, {}, 0.5);
    QCOMPARE(half.windows.size(), 1);
    // At progress 0.5, scale = 0.75, zoom factor = 0.75 / 0.5 = 1.5 around center (500, 300).
    QCOMPARE(half.windows[0].rect, QRectF(200, 150, 600, 300));
    QCOMPARE(half.windows[0].opacity, 1.0);
}

QTEST_MAIN(TestOverviewPaint)
#include "test_overview_paint.moc"
