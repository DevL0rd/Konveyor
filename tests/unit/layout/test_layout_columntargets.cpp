#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

QList<Config::ColumnDisplay> displays(Fixture &fixture)
{
    return workspacesOn(fixture, QStringLiteral("DP-1")).first().columnDisplays;
}

}

class TestLayoutColumnTargets : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void columnMovesWithAnIdMoveThatColumnAndKeepFocus_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("window");
        QTest::addColumn<QList<int>>("order");
        QTest::newRow("left") << QStringLiteral("move-column-left") << 3 << QList<int> {0, 1, 3, 2};
        QTest::newRow("left at the start") << QStringLiteral("move-column-left") << 0 << QList<int> {0, 1, 2, 3};
        QTest::newRow("right") << QStringLiteral("move-column-right") << 0 << QList<int> {1, 0, 2, 3};
        QTest::newRow("right at the end") << QStringLiteral("move-column-right") << 3 << QList<int> {0, 1, 2, 3};
        QTest::newRow("first") << QStringLiteral("move-column-to-first") << 2 << QList<int> {2, 0, 1, 3};
        QTest::newRow("last") << QStringLiteral("move-column-to-last") << 0 << QList<int> {1, 2, 3, 0};
    }

    void columnMovesWithAnIdMoveThatColumnAndKeepFocus()
    {
        QFETCH(QString, name);
        QFETCH(int, window);
        QFETCH(QList<int>, order);
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 4);
        fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("2")});
        const QRectF focusedFrame = fixture.engine().windowState(ids[1])->targetFrame;
        QVERIFY(fixture.perform(name, {}, idProperty(ids[window])).ok);
        Columns expected;
        for (const int index : std::as_const(order)) {
            expected.append(WindowIds {ids[index]});
        }
        QCOMPARE(columns(fixture), expected);
        COMPARE_FOCUS(fixture, ids[1]);
        if (order.indexOf(1) == 1) {
            QCOMPARE(fixture.engine().windowState(ids[1])->targetFrame, focusedFrame);
        }
        VERIFY_INVARIANTS(fixture);
    }

    void columnMovesWithAFloatingOrUnknownIdChangeNothing()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating"), {}, idProperty(ids[0])).ok);
        const Columns before = columns(fixture);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-last"), {}, idProperty(ids[0])).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-column-left"), {}, idProperty(999)).ok);
        QCOMPARE(columns(fixture), before);
        VERIFY_INVARIANTS(fixture);
    }

    void columnDisplayWithAnIdChangesThatColumn()
    {
        Fixture fixture;
        const Layout::WindowId top = fixture.add(QStringLiteral("top"));
        const Layout::WindowId bottom = fixture.add(QStringLiteral("bottom"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const Layout::WindowId other = fixture.add(QStringLiteral("other"));
        QCOMPARE(columns(fixture), (Columns {{top, bottom}, {other}}));
        QCOMPARE(displays(fixture), (QList<Config::ColumnDisplay> {Config::ColumnDisplay::Normal, Config::ColumnDisplay::Normal}));
        QVERIFY(fixture.perform(QStringLiteral("toggle-column-tabbed-display"), {}, idProperty(bottom)).ok);
        QCOMPARE(displays(fixture), (QList<Config::ColumnDisplay> {Config::ColumnDisplay::Tabbed, Config::ColumnDisplay::Normal}));
        COMPARE_FOCUS(fixture, other);
        QVERIFY(fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("normal")}, idProperty(top)).ok);
        QCOMPARE(displays(fixture), (QList<Config::ColumnDisplay> {Config::ColumnDisplay::Normal, Config::ColumnDisplay::Normal}));
        QVERIFY(fixture.perform(QStringLiteral("toggle-column-tabbed-display")).ok);
        QCOMPARE(displays(fixture), (QList<Config::ColumnDisplay> {Config::ColumnDisplay::Normal, Config::ColumnDisplay::Tabbed}));
        QVERIFY(fixture.perform(QStringLiteral("toggle-column-tabbed-display"), {}, idProperty(999)).ok);
        QCOMPARE(displays(fixture), (QList<Config::ColumnDisplay> {Config::ColumnDisplay::Normal, Config::ColumnDisplay::Tabbed}));
        VERIFY_INVARIANTS(fixture);
    }

    void moveWindowToMonitorWithoutFocusLeavesFocusAlone()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 2);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
        fixture.engine().activateWindow(ids[1]);
        QVERIFY(fixture
                .perform(QStringLiteral("move-window-to-monitor"), {QStringLiteral("DP-2")},
                    {{QStringLiteral("id"), QString::number(ids[0])}, {QStringLiteral("focus"), QStringLiteral("false")}})
                .ok);
        QCOMPARE(fixture.state(ids[0]).output, QStringLiteral("DP-2"));
        COMPARE_FOCUS(fixture, ids[1]);
        QCOMPARE(fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-1")));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutColumnTargets)

#include "test_layout_columntargets.moc"
