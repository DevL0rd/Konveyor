#include "helpers.h"

using namespace LayoutTest;

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
};

QTEST_GUILESS_MAIN(TestLayoutRestore)
#include "test_layout_restore.moc"
