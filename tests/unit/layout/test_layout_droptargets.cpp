#include "helpers.h"

using namespace LayoutTest;

namespace
{

struct Three
{
    Fixture fixture;
    Layout::WindowId a = fixture.add(QStringLiteral("a"));
    Layout::WindowId b = fixture.add(QStringLiteral("b"));
    Layout::WindowId c = fixture.add(QStringLiteral("c"));

    explicit Three(const Config::Config &config = instantConfig())
        : fixture(config)
    { }

    void dropAt(Layout::WindowId id, QPointF pointer)
    {
        QVERIFY(startMove(fixture, id, pointer));
        fixture.engine().updateWindowDrag(pointer + QPointF(1, 1), QStringLiteral("DP-1"));
        fixture.engine().endWindowDrag();
        fixture.settle();
    }

    std::optional<QRectF> dropHint() { return fixture.engine().outputStates().constFirst().dropHint; }
};

Config::Config withoutInsertHint()
{
    Config::Config config = instantConfig();
    config.layout.insertHint.enabled = false;
    return config;
}

}

class TestLayoutDropTargets : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void droppingNearTheTopOfAColumnStacksIntoIt()
    {
        Three t;
        t.fixture.engine().activateWindow(t.a);
        const QRectF target = t.fixture.frame(t.a);
        t.dropAt(t.c, QPointF(target.center().x(), target.top() + 10));
        QCOMPARE(t.fixture.state(t.c).columnIndex, t.fixture.state(t.a).columnIndex);
        QCOMPARE(t.fixture.state(t.c).tileIndex, 0);
        QCOMPARE(t.fixture.state(t.a).tileIndex, 1);
        QCOMPARE(t.fixture.focused(), std::optional(t.c));
        VERIFY_INVARIANTS(t.fixture);
    }

    void droppingNearTheBottomOfAColumnStacksBelow()
    {
        Three t;
        t.fixture.engine().activateWindow(t.a);
        const QRectF target = t.fixture.frame(t.a);
        t.dropAt(t.c, QPointF(target.center().x(), target.bottom() - 5));
        QCOMPARE(t.fixture.state(t.c).columnIndex, t.fixture.state(t.a).columnIndex);
        QCOMPARE(t.fixture.state(t.c).tileIndex, 1);
        VERIFY_INVARIANTS(t.fixture);
    }

    void droppingBetweenColumnsOpensANewColumnThere()
    {
        Three t;
        t.fixture.engine().activateWindow(t.a);
        const double between = (t.fixture.frame(t.a).right() + t.fixture.frame(t.b).left()) / 2.0;
        t.dropAt(t.c, QPointF(between, 540));
        QCOMPARE(t.fixture.state(t.a).columnIndex, 0);
        QCOMPARE(t.fixture.state(t.c).columnIndex, 1);
        QCOMPARE(t.fixture.state(t.b).columnIndex, 2);
        QCOMPARE(t.fixture.state(t.c).tileIndex, 0);
        VERIFY_INVARIANTS(t.fixture);
    }

    void droppingAtTheLeftEdgeMakesTheFirstColumn()
    {
        Three t;
        t.fixture.engine().activateWindow(t.a);
        t.dropAt(t.c, QPointF(2, 540));
        QCOMPARE(t.fixture.state(t.c).columnIndex, 0);
        QCOMPARE(t.fixture.state(t.a).columnIndex, 1);
        VERIFY_INVARIANTS(t.fixture);
    }

    void dropHintFollowsThePointer()
    {
        Three t;
        t.fixture.engine().activateWindow(t.a);
        const QRectF column = t.fixture.frame(t.a);
        QVERIFY(startMove(t.fixture, t.c, QPointF(column.center().x(), column.top() + 10)));
        const std::optional<QRectF> stacked = t.dropHint();
        QVERIFY(stacked.has_value());
        QVERIFY(stacked->intersects(column));
        t.fixture.engine().updateWindowDrag(QPointF(2, 540), QStringLiteral("DP-1"));
        const std::optional<QRectF> first = t.dropHint();
        QVERIFY(first.has_value());
        QVERIFY(*first != *stacked);
        QVERIFY(first->left() < column.center().x());
        t.fixture.engine().endWindowDrag();
        QVERIFY(!t.dropHint().has_value());
    }

    void noDropHintWhenTheInsertHintIsOff()
    {
        Three t(withoutInsertHint());
        QVERIFY(startMove(t.fixture, t.c, QPointF(2, 540)));
        QVERIFY(!t.dropHint().has_value());
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QCOMPARE(t.fixture.state(t.c).columnIndex, 0);
    }

    void draggingAFloatingWindowMovesItToThePointer()
    {
        Three t;
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        t.fixture.advance(1000);
        const QPointF start = t.fixture.frame(t.c).center();
        QVERIFY(t.fixture.engine().beginWindowDrag(t.c, start));
        t.fixture.engine().updateWindowDrag(start + QPointF(-300, 120), QStringLiteral("DP-1"));
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QVERIFY(t.fixture.state(t.c).isFloating);
        QCOMPARE(t.fixture.frame(t.c).center(), start + QPointF(-300, 120));
        VERIFY_INVARIANTS(t.fixture);
    }

    void togglingFloatingMidDragDropsItFloatingAtThePointer()
    {
        Three t;
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        QVERIFY(t.dropHint().has_value());
        t.fixture.engine().toggleWindowDragFloating();
        QVERIFY(!t.dropHint().has_value());
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QVERIFY(t.fixture.state(t.c).isFloating);
        QVERIFY(t.fixture.frame(t.c).contains(QPointF(700, 500)));
        VERIFY_INVARIANTS(t.fixture);
    }

    void togglingFloatingTwiceMidDragDropsItTiled()
    {
        Three t;
        QVERIFY(startMove(t.fixture, t.c, QPointF(2, 540)));
        t.fixture.engine().toggleWindowDragFloating();
        t.fixture.engine().toggleWindowDragFloating();
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QVERIFY(!t.fixture.state(t.c).isFloating);
        QCOMPARE(t.fixture.state(t.c).columnIndex, 0);
    }

    void togglingFloatingBeforeTheWindowDetachesDoesNothing()
    {
        Three t;
        const QPointF start = t.fixture.frame(t.c).center();
        QVERIFY(t.fixture.engine().beginWindowDrag(t.c, start));
        t.fixture.engine().toggleWindowDragFloating();
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QVERIFY(!t.fixture.state(t.c).isFloating);
        QCOMPARE(t.fixture.state(t.c).columnIndex, 2);
    }

    void draggingAFullscreenWindowLeavesFullscreen_data()
    {
        QTest::addColumn<QString>("action");
        QTest::newRow("fullscreen") << QStringLiteral("fullscreen-window");
        QTest::newRow("maximized") << QStringLiteral("maximize-window-to-edges");
    }

    void draggingAFullscreenWindowLeavesFullscreen()
    {
        QFETCH(QString, action);
        Three t;
        QVERIFY(t.fixture.perform(action).ok);
        QVERIFY(t.fixture.state(t.c).requestedSizingMode != Layout::WindowMode::Normal);
        t.dropAt(t.c, QPointF(2, 540));
        QCOMPARE(t.fixture.state(t.c).requestedSizingMode, Layout::WindowMode::Normal);
        QCOMPARE(t.fixture.state(t.c).columnIndex, 0);
        VERIFY_INVARIANTS(t.fixture);
    }

    void onlyOneDragAtATime()
    {
        Three t;
        QVERIFY(t.fixture.engine().beginWindowDrag(t.c, t.fixture.frame(t.c).center()));
        QVERIFY(!t.fixture.engine().beginWindowDrag(t.b, t.fixture.frame(t.b).center()));
        t.fixture.engine().endWindowDrag();
        QVERIFY(t.fixture.engine().beginWindowDrag(t.b, t.fixture.frame(t.b).center()));
        t.fixture.engine().endWindowDrag();
        QVERIFY(!t.fixture.engine().beginWindowDrag(99, QPointF()));
        VERIFY_INVARIANTS(t.fixture);
    }

    void dragCallsWithoutADragDoNothing()
    {
        Three t;
        const QRectF before = t.fixture.frame(t.c);
        t.fixture.engine().updateWindowDrag(QPointF(2, 540), QStringLiteral("DP-1"));
        t.fixture.engine().toggleWindowDragFloating();
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QCOMPARE(t.fixture.frame(t.c), before);
        QVERIFY(!t.dropHint().has_value());
    }

    void droppingWithTheOverviewOpenKeepsTheActiveWorkspace()
    {
        Three t;
        QVERIFY(t.fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        QVERIFY(t.fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        t.fixture.engine().setOverviewOpen(true);
        const Layout::WorkspaceId active = t.fixture.state(t.a).workspace;
        t.dropAt(t.b, QPointF(2, 540));
        QCOMPARE(t.fixture.state(t.b).workspace, active);
        QVERIFY(t.fixture.state(t.a).onActiveWorkspace);
        VERIFY_INVARIANTS(t.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutDropTargets)
#include "test_layout_droptargets.moc"
