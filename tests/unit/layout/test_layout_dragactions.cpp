#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config withHalfOpaqueC()
{
    Config::Config config = instantConfig();
    Config::WindowRule rule = ruleFor(QStringLiteral("c"));
    rule.opacity = 0.5;
    config.windowRules.append(rule);
    return config;
}

struct Three : ThreeWindows
{
    using ThreeWindows::ThreeWindows;

    void drop()
    {
        fixture.engine().endWindowDrag();
        fixture.settle();
    }
};

}

class TestLayoutDragActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void theDraggedWindowKeepsItsRuleOpacity()
    {
        Three t(withHalfOpaqueC());
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        QCOMPARE(t.fixture.state(t.c).ruleOpacity, 0.5);
        t.drop();
        QCOMPARE(t.fixture.state(t.c).ruleOpacity, 0.5);
    }

    void theRuleOpacityCanBeToggledMidDrag()
    {
        Three t(withHalfOpaqueC());
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-rule-opacity")).ok);
        QCOMPARE(t.fixture.state(t.c).ruleOpacity, 1.0);
        t.drop();
        QCOMPARE(t.fixture.state(t.c).ruleOpacity, 1.0);
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-rule-opacity"), {}, idProperty(t.c)).ok);
        QCOMPARE(t.fixture.state(t.c).ruleOpacity, 0.5);
        VERIFY_INVARIANTS(t.fixture);
    }

    void urgencyReachesTheDraggedWindow_data()
    {
        QTest::addColumn<bool>("byAction");
        QTest::newRow("action") << true;
        QTest::newRow("app") << false;
    }

    void urgencyReachesTheDraggedWindow()
    {
        QFETCH(bool, byAction);
        Three t;
        t.fixture.engine().activateWindow(t.b);
        QVERIFY(startMove(t.fixture, t.a, QPointF(1900, 500)));
        QVERIFY(!t.fixture.state(t.a).isFocused);
        if (byAction) {
            QVERIFY(t.fixture.perform(QStringLiteral("set-window-urgent"), {}, idProperty(t.a)).ok);
        } else {
            t.fixture.engine().setWindowUrgent(t.a, true);
        }
        QVERIFY(t.fixture.state(t.a).isUrgent);
        t.drop();
        VERIFY_INVARIANTS(t.fixture);
    }

    void floatingActionsSwitchTheDrag_data()
    {
        QTest::addColumn<QStringList>("actions");
        QTest::addColumn<bool>("floating");
        QTest::newRow("toggle") << QStringList {QStringLiteral("toggle-window-floating")} << true;
        QTest::newRow("toggle twice") << QStringList {QStringLiteral("toggle-window-floating"), QStringLiteral("toggle-window-floating")}
                                      << false;
        QTest::newRow("to floating") << QStringList {QStringLiteral("move-window-to-floating")} << true;
        QTest::newRow("to floating twice") << QStringList {QStringLiteral("move-window-to-floating"),
            QStringLiteral("move-window-to-floating")}
                                           << true;
        QTest::newRow("to tiling") << QStringList {QStringLiteral("move-window-to-tiling")} << false;
        QTest::newRow("there and back") << QStringList {QStringLiteral("move-window-to-floating"), QStringLiteral("move-window-to-tiling")}
                                        << false;
    }

    void floatingActionsSwitchTheDrag()
    {
        QFETCH(QStringList, actions);
        QFETCH(bool, floating);
        Three t;
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        for (const QString &name : std::as_const(actions)) {
            QVERIFY(t.fixture.perform(name).ok);
        }
        QCOMPARE(t.fixture.state(t.c).isFloating, floating);
        QCOMPARE(t.fixture.engine().outputStates().constFirst().dropHint.has_value(), !floating);
        QCOMPARE(t.fixture.engine().movingWindow(), std::optional(t.c));
        t.drop();
        QCOMPARE(t.fixture.state(t.c).isFloating, floating);
        QVERIFY(t.fixture.frame(t.c).contains(QPointF(700, 500)) || !floating);
        VERIFY_INVARIANTS(t.fixture);
    }

    void aFloatingActionForAnotherWindowLeavesTheDragAlone()
    {
        Three t;
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-floating"), {}, idProperty(t.a)).ok);
        QVERIFY(t.fixture.state(t.a).isFloating);
        QVERIFY(!t.fixture.state(t.c).isFloating);
        QVERIFY(t.fixture.engine().outputStates().constFirst().dropHint.has_value());
        t.drop();
        QVERIFY(!t.fixture.state(t.c).isFloating);
        VERIFY_INVARIANTS(t.fixture);
    }

    void aFloatingWindowDraggedTiledDropsWhereThePointerIs()
    {
        Three t;
        t.fixture.engine().activateWindow(t.c);
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        t.fixture.advance(1000);
        const QRectF frame = t.fixture.frame(t.c);
        const QPointF grab = frame.topLeft() + QPointF(40, 40);
        QVERIFY(t.fixture.engine().beginWindowDrag(t.c, grab));
        t.fixture.engine().setFloatingFrame(t.c, frame.translated(-frame.left() + 20, 0));
        t.fixture.advance(1000);
        QVERIFY(t.fixture.engine().toggleWindowDragFloating());
        QVERIFY(!t.fixture.state(t.c).isFloating);
        QVERIFY(t.fixture.engine().outputStates().constFirst().dropHint->left() < t.fixture.frame(t.a).center().x());
        t.drop();
        QVERIFY(!t.fixture.state(t.c).isFloating);
        QCOMPARE(t.fixture.state(t.c).columnIndex, 0);
        VERIFY_INVARIANTS(t.fixture);
    }

    void draggingFloatingBringsBackTheFloatingSize()
    {
        Three t;
        t.fixture.engine().activateWindow(t.c);
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QVERIFY(t.fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("500")}).ok);
        QVERIFY(t.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("400")}).ok);
        t.fixture.settle();
        QCOMPARE(t.fixture.frame(t.c).size(), QSizeF(500, 400));
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        t.fixture.settle();
        QVERIFY(t.fixture.frame(t.c).size() != QSizeF(500, 400));
        QVERIFY(startMove(t.fixture, t.c, QPointF(700, 500)));
        QVERIFY(t.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        t.drop();
        QVERIFY(t.fixture.state(t.c).isFloating);
        QCOMPARE(t.fixture.frame(t.c).size(), QSizeF(500, 400));
        VERIFY_INVARIANTS(t.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutDragActions)
#include "test_layout_dragactions.moc"
