#include "helpers.h"

using namespace LayoutTest;

class TestLayoutWindowDrag : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void interactiveMoveWithinOffsetOutputKeepsTheWorkspace_data()
    {
        QTest::addColumn<QRectF>("geometry");
        QTest::newRow("beside, top aligned") << QRectF(1920, 0, 1024, 600);
        QTest::newRow("beside, bottom aligned") << QRectF(1920, 480, 1024, 600);
        QTest::newRow("below") << QRectF(0, 1080, 1024, 600);
    }

    void interactiveMoveWithinOffsetOutputKeepsTheWorkspace()
    {
        QFETCH(QRectF, geometry);
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), geometry));
        fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("DP-2")});
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        const Layout::WorkspaceId workspace = fixture.state(id).workspace;
        const int column = fixture.state(id).columnIndex;
        const QPointF start = fixture.frame(id).center();
        QVERIFY(fixture.engine().beginWindowDrag(id, start));
        fixture.engine().updateWindowDrag(start + QPointF(0, 300), QStringLiteral("DP-2"));
        fixture.engine().updateWindowDrag(start + QPointF(10, 20), QStringLiteral("DP-2"));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        QCOMPARE(fixture.state(id).workspace, workspace);
        QCOMPARE(fixture.state(id).columnIndex, column);
        QVERIFY(fixture.state(id).onActiveWorkspace);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWindowDrag)
#include "test_layout_windowdrag.moc"
