#include "helpers.h"

using namespace LayoutTest;

Q_DECLARE_METATYPE(Konveyor::Layout::ActivationPolicy)

namespace
{

QList<std::pair<int, int>> positions(Fixture &fixture, const QList<Layout::WindowId> &ids)
{
    QList<std::pair<int, int>> result;
    for (const Layout::WindowId id : ids) {
        const Layout::WindowState state = fixture.state(id);
        result.append({state.columnIndex, state.tileIndex});
    }
    return result;
}

Layout::WindowId restore(Fixture &fixture, Layout::WindowId id, const QString &appId)
{
    const std::optional<Layout::RestorePlacement> placement = fixture.engine().placementOf(id);
    fixture.remove(id);
    const Layout::WindowId restored = id + 100;
    fixture.engine().addWindow(restored, makeWindow(appId, appId), QString(), Layout::ActivationPolicy::Focus, placement);
    fixture.settle();
    return restored;
}

}

class TestLayoutRestore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresALoneColumnToItsIndexAndWidth()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        fixture.engine().activateWindow(b);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("30%")});
        const double width = fixture.frame(b).width();
        const auto restored = restore(fixture, b, QStringLiteral("b"));
        QCOMPARE(positions(fixture, {a, restored, c}), (QList<std::pair<int, int>> {{0, 0}, {1, 0}, {2, 0}}));
        QCOMPARE(fixture.frame(restored).width(), width);
        VERIFY_INVARIANTS(fixture);
    }

    void restoresAFullWidthColumnAtFullWidth()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        fixture.engine().activateWindow(b);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(b).width(), 1888.0);
        const auto restored = restore(fixture, b, QStringLiteral("b"));
        QCOMPARE(positions(fixture, {a, restored, c}), (QList<std::pair<int, int>> {{0, 0}, {1, 0}, {2, 0}}));
        QCOMPARE(fixture.frame(restored).width(), 1888.0);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(restored).width(), 936.0);
        VERIFY_INVARIANTS(fixture);
    }

    void restoresAStackedWindowIntoItsColumn()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        fixture.engine().activateWindow(c);
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QCOMPARE(positions(fixture, {b, c}), (QList<std::pair<int, int>> {{1, 0}, {1, 1}}));
        const auto restored = restore(fixture, c, QStringLiteral("c"));
        QCOMPARE(positions(fixture, {a, b, restored}), (QList<std::pair<int, int>> {{0, 0}, {1, 0}, {1, 1}}));
        VERIFY_INVARIANTS(fixture);
    }

    void restoresAFloatingWindowWhereItWas()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        Layout::WindowProperties properties = makeWindow(QStringLiteral("dialog"), QStringLiteral("dialog"), QSizeF(400, 300));
        properties.isDialog = true;
        const auto dialog = fixture.addWith(properties);
        fixture.engine().setFloatingFrame(dialog, QRectF(300, 200, 400, 300));
        fixture.settle();
        const QRectF before = fixture.frame(dialog);
        const std::optional<Layout::RestorePlacement> placement = fixture.engine().placementOf(dialog);
        QVERIFY(placement && placement->isFloating);
        fixture.remove(dialog);
        fixture.engine().addWindow(dialog + 100, properties, QString(), Layout::ActivationPolicy::Focus, placement);
        fixture.settle();
        QCOMPARE(fixture.frame(dialog + 100), before);
        VERIFY_INVARIANTS(fixture);
    }

    void restoresAFloatingWindowToItsSpotOnItsWorkspace_data()
    {
        QTest::addColumn<Layout::ActivationPolicy>("policy");
        QTest::addColumn<QString>("meanwhile");
        QTest::addColumn<QPointF>("shift");
        for (const auto &[name, policy] : {std::pair {"smart", Layout::ActivationPolicy::Smart},
                 std::pair {"focus", Layout::ActivationPolicy::Focus}, std::pair {"no focus", Layout::ActivationPolicy::NoFocus}}) {
            QTest::newRow(qPrintable(QStringLiteral("%1, workspace below").arg(QLatin1String(name))))
                << policy << QStringLiteral("focus-workspace-down") << QPointF();
            QTest::newRow(qPrintable(QStringLiteral("%1, workspace on the other monitor").arg(QLatin1String(name))))
                << policy << QStringLiteral("move-workspace-to-monitor-right") << QPointF(1920, 0);
        }
    }

    void restoresAFloatingWindowToItsSpotOnItsWorkspace()
    {
        QFETCH(Layout::ActivationPolicy, policy);
        QFETCH(QString, meanwhile);
        QFETCH(QPointF, shift);
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
        fixture.add(QStringLiteral("a"));
        Layout::WindowProperties properties = makeWindow(QStringLiteral("dialog"), QStringLiteral("dialog"), QSizeF(400, 300));
        properties.isDialog = true;
        const auto dialog = fixture.addWith(properties);
        fixture.engine().setFloatingFrame(dialog, QRectF(300, 200, 400, 300));
        fixture.settle();
        const QRectF before = fixture.frame(dialog);
        const std::optional<Layout::RestorePlacement> placement = fixture.engine().placementOf(dialog);
        fixture.remove(dialog);
        fixture.perform(meanwhile);
        fixture.advance(1);
        fixture.engine().addWindow(dialog + 100, properties, QString(), policy, placement);
        fixture.settle();
        fixture.engine().activateWindow(dialog + 100);
        fixture.advance(1);
        QCOMPARE(fixture.frame(dialog + 100), before.translated(shift));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutRestore)
#include "test_layout_restore.moc"
