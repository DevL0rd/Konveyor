#include "helpers.h"

using namespace LayoutTest;

namespace
{

struct Three : ThreeWindows
{
    using ThreeWindows::ThreeWindows;

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

Config::Config withSecondOutputHint(bool enabled, const QColor &color)
{
    Config::Config config = instantConfig();
    config.layout.insertHint.paint.color = QColor(1, 2, 3);
    Config::OutputConfig second;
    second.name = QStringLiteral("DP-2");
    Config::Paint paint = config.layout.insertHint.paint;
    paint.color = color;
    second.layout = Config::LayoutPart {};
    second.layout->insertHint = Config::InsertHintPart {enabled, paint};
    config.outputs.append(second);
    return config;
}

std::optional<Layout::OutputState> outputState(Fixture &fixture, const QString &name)
{
    for (const Layout::OutputState &state : fixture.engine().outputStates()) {
        if (state.name == name) {
            return state;
        }
    }
    return std::nullopt;
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

    void newColumnHintAtAnEdgeStaysHalfOnScreen_data()
    {
        QTest::addColumn<bool>("leftEdge");
        QTest::newRow("left") << true;
        QTest::newRow("right") << false;
    }

    void newColumnHintAtAnEdgeStaysHalfOnScreen()
    {
        QFETCH(bool, leftEdge);
        Three t;
        t.fixture.engine().activateWindow(leftEdge ? t.a : t.c);
        const Layout::WindowId dragged = leftEdge ? t.c : t.a;
        QVERIFY(startMove(t.fixture, dragged, QPointF(leftEdge ? 1 : 1919, 540)));
        const std::optional<QRectF> hint = t.dropHint();
        QVERIFY(hint.has_value());
        QVERIFY2(hint->left() >= -hint->width() / 2.0, qPrintable(QString::number(hint->left())));
        QVERIFY2(hint->right() <= 1920 + hint->width() / 2.0, qPrintable(QString::number(hint->right())));
        t.fixture.engine().endWindowDrag();
    }

    void theDraggedWindowFadesWhileItIsHeldStill()
    {
        Three t(linearAnimationConfig());
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        t.fixture.advance(1000);
        QVERIFY(!t.fixture.engine().isAnimating());
        QCOMPARE(t.fixture.state(t.c).renderAlpha, 0.75);
        t.fixture.engine().toggleWindowDragFloating();
        QVERIFY(t.fixture.engine().isAnimating());
        t.fixture.advance(100);
        QVERIFY(t.fixture.engine().isAnimating());
        QVERIFY(t.fixture.state(t.c).renderAlpha > 0.75 && t.fixture.state(t.c).renderAlpha < 1.0);
        t.fixture.advance(1000);
        QVERIFY(!t.fixture.engine().isAnimating());
        QCOMPARE(t.fixture.state(t.c).renderAlpha, 1.0);
        t.fixture.engine().endWindowDrag();
    }

    void eachOutputUsesItsOwnInsertHint()
    {
        Three t(withSecondOutputHint(true, QColor(4, 5, 6)));
        t.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QVERIFY(startMove(t.fixture, t.c, QPointF(2500, 540), QStringLiteral("DP-2")));
        QVERIFY(outputState(t.fixture, QStringLiteral("DP-2"))->dropHint.has_value());
        QVERIFY(!outputState(t.fixture, QStringLiteral("DP-1"))->dropHint.has_value());
        QCOMPARE(outputState(t.fixture, QStringLiteral("DP-2"))->dropHintPaint.color, QColor(4, 5, 6));
        QCOMPARE(outputState(t.fixture, QStringLiteral("DP-1"))->dropHintPaint.color, QColor(1, 2, 3));
        t.fixture.engine().updateWindowDrag(QPointF(2, 540), QStringLiteral("DP-1"));
        QVERIFY(outputState(t.fixture, QStringLiteral("DP-1"))->dropHint.has_value());
        QVERIFY(!outputState(t.fixture, QStringLiteral("DP-2"))->dropHint.has_value());
        t.fixture.engine().endWindowDrag();
    }

    void anOutputCanTurnTheInsertHintOff()
    {
        Three t(withSecondOutputHint(false, QColor(4, 5, 6)));
        t.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QVERIFY(startMove(t.fixture, t.c, QPointF(2500, 540), QStringLiteral("DP-2")));
        QVERIFY(!outputState(t.fixture, QStringLiteral("DP-2"))->dropHint.has_value());
        t.fixture.engine().updateWindowDrag(QPointF(2, 540), QStringLiteral("DP-1"));
        QVERIFY(outputState(t.fixture, QStringLiteral("DP-1"))->dropHint.has_value());
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QCOMPARE(t.fixture.state(t.c).output, QStringLiteral("DP-1"));
        QCOMPARE(t.fixture.state(t.c).columnIndex, 0);
    }

    void turningTheInsertHintOffMidDragHidesIt()
    {
        Three t;
        QVERIFY(startMove(t.fixture, t.c, QPointF(2, 540)));
        QVERIFY(t.dropHint().has_value());
        t.fixture.engine().setConfig(withoutInsertHint());
        QVERIFY(!t.dropHint().has_value());
        t.fixture.engine().setConfig(instantConfig());
        QVERIFY(t.dropHint().has_value());
        t.fixture.engine().endWindowDrag();
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

    void togglingFloatingBeforeTheWindowDetachesLiftsItFloating()
    {
        Three t;
        const QRectF before = t.fixture.frame(t.c);
        QVERIFY(t.fixture.engine().beginWindowDrag(t.c, before.center()));
        QVERIFY(t.fixture.engine().toggleWindowDragFloating());
        QCOMPARE(t.fixture.engine().movingWindow(), std::optional(t.c));
        t.fixture.engine().endWindowDrag();
        t.fixture.settle();
        QVERIFY(t.fixture.state(t.c).isFloating);
        QVERIFY(t.fixture.frame(t.c).contains(before.center()));
        VERIFY_INVARIANTS(t.fixture);
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
        QVERIFY(!t.fixture.engine().toggleWindowDragFloating());
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
