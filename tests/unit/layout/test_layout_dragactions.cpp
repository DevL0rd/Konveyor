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
};

QTEST_GUILESS_MAIN(TestLayoutDragActions)
#include "test_layout_dragactions.moc"
