#include "helpers.h"

using namespace LayoutTest;

namespace
{

struct Row
{
    Fixture fixture;
    Layout::WindowId a = fixture.add(QStringLiteral("a"));
    Layout::WindowId b = fixture.add(QStringLiteral("b"));
    Layout::WindowId c = fixture.add(QStringLiteral("c"));

    explicit Row(const Config::Config &config = instantConfig())
        : fixture(config)
    { }

    QList<std::pair<int, int>> places()
    {
        QList<std::pair<int, int>> result;
        for (const Layout::WindowId id : {a, b, c}) {
            result.append({fixture.state(id).columnIndex, fixture.state(id).tileIndex});
        }
        return result;
    }

    void cancel()
    {
        fixture.engine().cancelWindowDrag();
        fixture.settle();
    }
};

}

class TestLayoutDragCancel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cancellingPutsTheWindowBackInItsColumn()
    {
        Row row;
        const auto before = row.places();
        const Layout::WorkspaceId workspace = row.fixture.state(row.c).workspace;
        QVERIFY(startMove(row.fixture, row.c, QPointF(row.fixture.frame(row.a).center().x(), 30)));
        QVERIFY(row.fixture.engine().outputStates().constFirst().dropHint);
        row.cancel();
        QCOMPARE(row.places(), before);
        QCOMPARE(row.fixture.state(row.c).workspace, workspace);
        QCOMPARE(row.fixture.engine().movingWindow(), std::nullopt);
        QVERIFY(!row.fixture.engine().outputStates().constFirst().dropHint);
        QCOMPARE(row.fixture.focused(), std::optional(row.c));
        QCOMPARE(row.fixture.state(row.c).renderAlpha, 1.0);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingPutsAStackedWindowBackAtItsPlaceInTheStack()
    {
        Row row;
        row.fixture.engine().activateWindow(row.b);
        QVERIFY(row.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        const auto before = row.places();
        QCOMPARE(before[1], std::pair(0, 1));
        QVERIFY(startMove(row.fixture, row.b, QPointF(1900, 540)));
        row.cancel();
        QCOMPARE(row.places(), before);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingBeforeTheWindowDetachesLeavesEverything()
    {
        Row row;
        const auto before = row.places();
        const QPointF start = row.fixture.frame(row.b).center();
        QVERIFY(row.fixture.engine().beginWindowDrag(row.b, start));
        row.fixture.engine().updateWindowDrag(start + QPointF(30, 0), QStringLiteral("DP-1"));
        row.cancel();
        QCOMPARE(row.places(), before);
        QCOMPARE(row.fixture.frame(row.b).center(), start);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingRestoresFullscreenAndMaximized_data()
    {
        QTest::addColumn<QString>("action");
        QTest::addColumn<Layout::WindowMode>("mode");
        QTest::newRow("fullscreen") << QStringLiteral("fullscreen-window") << Layout::WindowMode::Fullscreen;
        QTest::newRow("maximized") << QStringLiteral("maximize-window-to-edges") << Layout::WindowMode::Maximized;
    }

    void cancellingRestoresFullscreenAndMaximized()
    {
        QFETCH(QString, action);
        QFETCH(Layout::WindowMode, mode);
        Row row;
        QVERIFY(row.fixture.perform(action).ok);
        const auto before = row.places();
        QVERIFY(startMove(row.fixture, row.c, QPointF(2, 540)));
        QCOMPARE(row.fixture.state(row.c).requestedSizingMode, Layout::WindowMode::Normal);
        row.cancel();
        QCOMPARE(row.fixture.state(row.c).requestedSizingMode, mode);
        QCOMPARE(row.places(), before);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingBringsTheWindowBackToAWorkspaceItEmptied()
    {
        Row row;
        QVERIFY(row.fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        const int index = row.fixture.state(row.c).workspaceIndex;
        QVERIFY(row.fixture.state(row.c).onActiveWorkspace);
        QVERIFY(startMove(row.fixture, row.c, QPointF(2, 540)));
        row.cancel();
        QCOMPARE(row.fixture.state(row.c).workspaceIndex, index);
        QVERIFY(row.fixture.state(row.c).workspace != row.fixture.state(row.a).workspace);
        QVERIFY(row.fixture.state(row.c).onActiveWorkspace);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingOnAnotherOutputBringsTheWindowHome()
    {
        Row row;
        row.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto before = row.places();
        QVERIFY(startMove(row.fixture, row.c, QPointF(2500, 540), QStringLiteral("DP-2")));
        row.cancel();
        QCOMPARE(row.fixture.state(row.c).output, QStringLiteral("DP-1"));
        QCOMPARE(row.places(), before);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingAfterTogglingFloatingPutsTheWindowBackTiled()
    {
        Row row;
        const auto before = row.places();
        QVERIFY(startMove(row.fixture, row.c, QPointF(700, 500)));
        row.fixture.engine().toggleWindowDragFloating();
        row.cancel();
        QVERIFY(!row.fixture.state(row.c).isFloating);
        QCOMPARE(row.places(), before);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingAFloatingWindowDragPutsItBackWhereItWas()
    {
        Row row;
        QVERIFY(row.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        row.fixture.advance(1000);
        const QRectF frame = row.fixture.frame(row.c);
        QVERIFY(row.fixture.engine().beginWindowDrag(row.c, frame.center()));
        row.fixture.engine().updateWindowDrag(frame.center() + QPointF(-300, 100), QStringLiteral("DP-1"));
        row.cancel();
        QVERIFY(row.fixture.state(row.c).isFloating);
        QCOMPARE(row.fixture.frame(row.c), frame);
        VERIFY_INVARIANTS(row.fixture);
    }

    void cancellingWithoutADragDoesNothing()
    {
        Row row;
        const auto before = row.places();
        row.cancel();
        QCOMPARE(row.places(), before);
    }

    void cancellingWithAnAnimatedLayoutSettlesInPlace()
    {
        Row row(linearAnimationConfig());
        row.fixture.advance(1000);
        const auto before = row.places();
        const QRectF frame = row.fixture.frame(row.c);
        QVERIFY(startMove(row.fixture, row.c, QPointF(2, 540)));
        row.fixture.advance(1000);
        row.cancel();
        row.fixture.advance(1000);
        QCOMPARE(row.places(), before);
        QCOMPARE(row.fixture.frame(row.c), frame);
        QCOMPARE(row.fixture.state(row.c).renderAlpha, 1.0);
        VERIFY_INVARIANTS(row.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutDragCancel)
#include "test_layout_dragcancel.moc"
