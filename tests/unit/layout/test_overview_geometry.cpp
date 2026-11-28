#include "layout/overview/overviewgeometry.h"

#include <QTest>

using namespace Konveyor::Layout;

namespace
{

constexpr double kEpsilon = 1e-6;

bool near(double a, double b)
{
    return qAbs(a - b) < kEpsilon;
}

OverviewGeometryInput inputFor(int workspaces, double minimumScale = 0.3)
{
    OverviewGeometryInput input;
    input.viewSize = QSizeF(1920, 1080);
    input.minimumScale = minimumScale;
    for (int index = 0; index < workspaces; ++index) {
        input.windowFrames.append(QList<QRectF> {});
    }
    return input;
}

}

class TestOverviewGeometry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singleWorkspaceStaysLarge();
    void scaleShrinksWithStackAndClampsToMinimum();
    void reservedInsetsShrinkScale();
    void cardsStackVerticallyWithGap();
    void windowFramesScaleIntoEachCard();
    void cameraClampsToContent();
    void cameraSeedsOnActiveWorkspace();
    void offscreenSidesReportHiddenContent();
    void hitTestingPrefersLaterWorkspaces();
    void nearestCardFollowsVerticalDistance();
    void workspacePointUnmapsThroughCard();
    void emptyInputIsHarmless();
};

void TestOverviewGeometry::singleWorkspaceStaysLarge()
{
    OverviewGeometryInput input = inputFor(1);
    input.windowFrames[0].append(QRectF(100, 80, 600, 400));

    const OverviewGeometry geometry = computeOverviewGeometry(input);
    QVERIFY(near(geometry.scale, 0.94));
    QCOMPARE(geometry.cards.size(), 1);
    QVERIFY(geometry.windows[0][0].width() > 500);
    QVERIFY(geometry.windows[0][0].height() > 350);
}

void TestOverviewGeometry::scaleShrinksWithStackAndClampsToMinimum()
{
    const OverviewGeometry two = computeOverviewGeometry(inputFor(2));
    QVERIFY(near(two.scale, 1015.2 / 2246.4));
    QVERIFY(two.scale < 0.94);

    const OverviewGeometry four = computeOverviewGeometry(inputFor(4));
    QVERIFY(near(four.scale, 0.3));

    const OverviewGeometry eight = computeOverviewGeometry(inputFor(8, 0.5));
    QVERIFY(near(eight.scale, 0.5));
}

void TestOverviewGeometry::reservedInsetsShrinkScale()
{
    OverviewGeometryInput input = inputFor(2);
    const double unreserved = computeOverviewGeometry(input).scale;

    input.reserved = QMarginsF(0, 0, 0, 216);
    const OverviewGeometry reserved = computeOverviewGeometry(input);
    QVERIFY(reserved.scale < unreserved);
    QVERIFY(near(reserved.scale, 799.2 / 2246.4));
}

void TestOverviewGeometry::cardsStackVerticallyWithGap()
{
    const OverviewGeometry geometry = computeOverviewGeometry(inputFor(3));
    QCOMPARE(geometry.cards.size(), 3);
    QVERIFY(near(geometry.gap, 86.4));
    QVERIFY(near(geometry.cards[0].top(), 0));
    QVERIFY(near(geometry.cards[0].width(), 1920 * geometry.scale));
    QVERIFY(near(geometry.cards[0].height(), 1080 * geometry.scale));
    for (int index = 1; index < geometry.cards.size(); ++index) {
        const double expected = geometry.cards[index - 1].bottom() + geometry.gap;
        QVERIFY(near(geometry.cards[index].top(), expected));
    }
    QVERIFY(near(geometry.contentBounds.height(), geometry.cards.last().bottom() + 2 * 32.4));
}

void TestOverviewGeometry::windowFramesScaleIntoEachCard()
{
    OverviewGeometryInput input = inputFor(2);
    input.windowFrames[0].append(QRectF(100, 80, 3000, 400));
    input.windowFrames[1].append(QRectF(0, 0, 480, 270));

    const OverviewGeometry geometry = computeOverviewGeometry(input);
    const QRectF first = geometry.windows[0][0];
    QVERIFY(near(first.left(), 100 * geometry.scale));
    QVERIFY(near(first.top(), geometry.cards[0].top() + 80 * geometry.scale));
    QVERIFY(near(first.width(), 3000 * geometry.scale));
    QVERIFY(first.width() > geometry.cards[0].width());

    const QRectF second = geometry.windows[1][0];
    QVERIFY(near(second.top(), geometry.cards[1].top()));
    QVERIFY(near(second.width(), 480 * geometry.scale));
    QVERIFY(geometry.contentBounds.left() < 0);
}

void TestOverviewGeometry::cameraClampsToContent()
{
    const QRectF content(0, 0, 3000, 2000);
    const QSizeF view(1920, 1080);
    QCOMPARE(clampOverviewCamera(content, view, QPointF(5000, 5000)), QPointF(1080, 920));
    QCOMPARE(clampOverviewCamera(content, view, QPointF(-100, -100)), QPointF(0, 0));
    QCOMPARE(clampOverviewCamera(content, view, QPointF(400, 300)), QPointF(400, 300));

    const QRectF small(0, 0, 1000, 500);
    QCOMPARE(clampOverviewCamera(small, view, QPointF(999, 999)), QPointF(-460, -290));
}

void TestOverviewGeometry::cameraSeedsOnActiveWorkspace()
{
    OverviewGeometryInput input = inputFor(4);
    input.activeWorkspace = 2;
    const OverviewGeometry geometry = computeOverviewGeometry(input);

    const QRectF viewport = overviewViewport(geometry.camera, input.viewSize);
    QVERIFY(geometry.cards[2].center().y() > viewport.top());
    QVERIFY(geometry.cards[2].center().y() < viewport.bottom());
}

void TestOverviewGeometry::offscreenSidesReportHiddenContent()
{
    const QRectF content(0, 0, 3000, 2000);
    const QList<OverviewSide> sides = overviewOffscreenSides(content, QRectF(1000, 0, 1920, 1080));
    QVERIFY2(sides.contains(OverviewSide::Left), "content extends left of the viewport");
    QVERIFY(sides.contains(OverviewSide::Right));
    QVERIFY(sides.contains(OverviewSide::Bottom));
    QVERIFY(!sides.contains(OverviewSide::Top));

    QVERIFY(overviewOffscreenSides(content, QRectF(-10, -10, 4000, 3000)).isEmpty());
    const QList<OverviewSide> topLeft = overviewOffscreenSides(content, QRectF(0, 500, 1920, 1080));
    QVERIFY(topLeft.contains(OverviewSide::Top));
    QVERIFY(!topLeft.contains(OverviewSide::Left));
}

void TestOverviewGeometry::hitTestingPrefersLaterWorkspaces()
{
    OverviewGeometry geometry;
    geometry.cards = {QRectF(0, 0, 100, 100), QRectF(0, 150, 100, 100)};
    geometry.windows = {{QRectF(10, 10, 200, 200)}, {QRectF(10, 150, 50, 50)}};

    QVERIFY(overviewCardAt(geometry, QPointF(50, 50)) == 0);
    QVERIFY(!overviewCardAt(geometry, QPointF(50, 120)));
    QVERIFY(overviewWindowAt(geometry, QPointF(15, 15)) == qMakePair(0, 0));
    QVERIFY(overviewWindowAt(geometry, QPointF(15, 165)) == qMakePair(1, 0));
    QVERIFY(overviewWindowAt(geometry, QPointF(50, 60)) == qMakePair(0, 0));
    QVERIFY(!overviewWindowAt(geometry, QPointF(5, 5)));
    QCOMPARE(overviewContentPoint(geometry, QPointF(5, 7)), QPointF(5, 7));
}

void TestOverviewGeometry::nearestCardFollowsVerticalDistance()
{
    OverviewGeometry geometry;
    geometry.cards = {QRectF(0, 0, 100, 100), QRectF(0, 150, 100, 100)};

    QVERIFY(overviewNearestCard(geometry, QPointF(50, 120)) == 0);
    QVERIFY(overviewNearestCard(geometry, QPointF(50, 140)) == 1);
    QVERIFY(overviewNearestCard(geometry, QPointF(50, 400)) == 1);
    QVERIFY(overviewNearestCard(geometry, QPointF(50, -50)) == 0);
    QVERIFY(!overviewNearestCard(OverviewGeometry {}, QPointF(0, 0)));
}

void TestOverviewGeometry::workspacePointUnmapsThroughCard()
{
    OverviewGeometry geometry;
    geometry.scale = 0.5;
    geometry.cards = {QRectF(0, 0, 100, 100), QRectF(0, 150, 100, 100)};

    QCOMPARE(overviewWorkspacePoint(geometry, 1, QPointF(25, 175)), QPointF(50, 50));
    QCOMPARE(overviewWorkspacePoint(geometry, 0, QPointF(25, 25)), QPointF(50, 50));
    QCOMPARE(overviewWorkspacePoint(geometry, 2, QPointF(25, 25)), QPointF(25, 25));
    QCOMPARE(overviewWorkspacePoint(geometry, -1, QPointF(25, 25)), QPointF(25, 25));

    geometry.scale = 0.0;
    QCOMPARE(overviewWorkspacePoint(geometry, 0, QPointF(25, 25)), QPointF(25, 25));
}

void TestOverviewGeometry::emptyInputIsHarmless()
{
    const OverviewGeometry geometry = computeOverviewGeometry(OverviewGeometryInput {});
    QVERIFY(geometry.cards.isEmpty());
    QVERIFY(geometry.windows.isEmpty());
    QVERIFY(!overviewCardAt(geometry, QPointF(0, 0)));
    QVERIFY(!overviewNearestCard(geometry, QPointF(0, 0)));
    QVERIFY(!overviewWindowAt(geometry, QPointF(0, 0)));
}

QTEST_MAIN(TestOverviewGeometry)
#include "test_overview_geometry.moc"
