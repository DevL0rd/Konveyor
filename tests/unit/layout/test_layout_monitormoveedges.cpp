#include "helpers.h"

using namespace LayoutTest;

namespace
{

const QString Home = QStringLiteral("DP-1");
const QString Side = QStringLiteral("DP-2");

}

class TestLayoutMonitorMoveEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void movingEverythingWhileAWindowIsDraggedKeepsTheDraggedWindow()
    {
        Fixture fixture;
        fixture.addOutput(Side, QRectF(1920, 0, 1920, 1080));
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.engine().focusOutput(Side);
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, b, QPointF(2900, 700), Side));
        QVERIFY(fixture.perform(QStringLiteral("move-all-windows-to-monitor"), {Home}).ok);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QVERIFY(fixture.engine().hasWindow(a));
        QVERIFY(fixture.engine().hasWindow(b));
        VERIFY_INVARIANTS(fixture);
    }

    void movingEverythingToAnEmptyMonitorWithoutFocus()
    {
        Fixture fixture;
        fixture.addOutput(Side, QRectF(1920, 0, 1920, 1080));
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.engine().setLayoutFocused(false);
        QVERIFY(fixture.perform(QStringLiteral("move-all-windows-to-monitor"), {Side}).ok);
        QCOMPARE(fixture.state(a).output, Side);
        fixture.engine().setLayoutFocused(true);
        QVERIFY(fixture.perform(QStringLiteral("move-all-windows-to-monitor"), {Home}).ok);
        QCOMPARE(fixture.state(a).output, Home);
        VERIFY_INVARIANTS(fixture);
    }

    void movingToTheMonitorAlreadyActiveDoesNothing()
    {
        Fixture fixture;
        fixture.addOutput(Side, QRectF(1920, 0, 1920, 1080));
        const auto a = fixture.add(QStringLiteral("a"));
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-monitor"), {Home}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-monitor"), {Home}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-workspace-to-monitor"), {Home}).ok);
        QCOMPARE(fixture.state(a).output, Home);
        VERIFY_INVARIANTS(fixture);
    }

    void directionalMovesNeedAnotherMonitor()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        for (const char *direction : {"left", "right", "up", "down", "next", "previous"}) {
            const QString suffix = QString::fromLatin1(direction);
            fixture.perform(QStringLiteral("focus-monitor-") + suffix);
            fixture.perform(QStringLiteral("move-window-to-monitor-") + suffix);
            fixture.perform(QStringLiteral("move-column-to-monitor-") + suffix);
            fixture.perform(QStringLiteral("move-workspace-to-monitor-") + suffix);
        }
        QCOMPARE(fixture.state(a).output, Home);
        VERIFY_INVARIANTS(fixture);
    }

    void directionalMovesPickTheNearestMonitor()
    {
        Fixture fixture;
        fixture.addOutput(Side, QRectF(1920, 0, 1920, 1080));
        fixture.addOutput(QStringLiteral("DP-3"), QRectF(0, 1080, 1920, 1080));
        fixture.addOutput(QStringLiteral("DP-4"), QRectF(-1920, 0, 1920, 1080));
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("move-window-to-monitor-right"));
        QCOMPARE(fixture.state(a).output, Side);
        fixture.perform(QStringLiteral("move-window-to-monitor-left"));
        fixture.perform(QStringLiteral("move-window-to-monitor-down"));
        QCOMPARE(fixture.state(a).output, QStringLiteral("DP-3"));
        fixture.perform(QStringLiteral("move-window-to-monitor-up"));
        fixture.perform(QStringLiteral("move-window-to-monitor-left"));
        QCOMPARE(fixture.state(a).output, QStringLiteral("DP-4"));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMonitorMoveEdges)
#include "test_layout_monitormoveedges.moc"
