#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config pinnedConfig(Config::Config config, const QString &appId, Config::ColumnPosition position)
{
    Config::WindowRule rule;
    Config::Match match;
    match.appId = QRegularExpression(QStringLiteral("^%1$").arg(appId));
    rule.matches.append(match);
    rule.columnPosition = position;
    config.windowRules.append(rule);
    return config;
}

Config::Config leftConfig(Config::Config config = instantConfig())
{
    config.layout.newColumnPosition = Config::NewColumnPosition::Left;
    return pinnedConfig(config, QStringLiteral("pinned"), Config::ColumnPosition::Start);
}

QList<Layout::WindowId> rowOrder(Fixture &fixture, const QList<Layout::WindowId> &ids)
{
    QList<Layout::WindowId> order = ids;
    std::ranges::sort(order, [&fixture](Layout::WindowId a, Layout::WindowId b) { return fixture.frame(a).x() < fixture.frame(b).x(); });
    return order;
}

}

class TestLayoutPins : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void newColumnsOpenLeftOfTheFocusedColumn()
    {
        Fixture fixture(leftConfig());
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        const auto third = fixture.add(QStringLiteral("c"));
        QCOMPARE(rowOrder(fixture, {first, second, third}), (QList<Layout::WindowId> {third, second, first}));
        QCOMPARE(fixture.focused(), std::optional(third));
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedStartColumnSendsNewColumnsToTheOtherSide()
    {
        Fixture fixture(leftConfig());
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        const auto opened = fixture.add(QStringLiteral("a"));
        QCOMPARE(rowOrder(fixture, {pinned, opened}), (QList<Layout::WindowId> {pinned, opened}));
        QCOMPARE(fixture.focused(), std::optional(opened));
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedEndColumnSendsNewColumnsToTheOtherSide()
    {
        Config::Config config = pinnedConfig(instantConfig(), QStringLiteral("pinned"), Config::ColumnPosition::End);
        Fixture fixture(config);
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        const auto opened = fixture.add(QStringLiteral("a"));
        QCOMPARE(rowOrder(fixture, {pinned, opened}), (QList<Layout::WindowId> {opened, pinned}));
        VERIFY_INVARIANTS(fixture);
    }

    void movingIntoAPinnedSlotDoesNothingAndDoesNotAnimate()
    {
        Config::Config config = leftConfig(Config::Config {});
        Fixture fixture(config);
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        const auto other = fixture.add(QStringLiteral("a"));
        fixture.advance(5000);
        QVERIFY(!fixture.engine().isAnimating());
        const QRectF before = fixture.frame(other);

        fixture.perform(QStringLiteral("move-column-left"));
        QVERIFY(!fixture.engine().isAnimating());
        fixture.perform(QStringLiteral("move-column-to-first"));
        QVERIFY(!fixture.engine().isAnimating());
        QCOMPARE(fixture.frame(other), before);
        QCOMPARE(rowOrder(fixture, {pinned, other}), (QList<Layout::WindowId> {pinned, other}));
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedColumnCannotLeaveItsSlot()
    {
        Fixture fixture(leftConfig());
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        const auto other = fixture.add(QStringLiteral("a"));
        fixture.engine().activateWindow(pinned);
        fixture.perform(QStringLiteral("move-column-right"));
        QCOMPARE(rowOrder(fixture, {pinned, other}), (QList<Layout::WindowId> {pinned, other}));
        VERIFY_INVARIANTS(fixture);
    }

    void closingALeftOpenedWindowReturnsToThePreviousColumn()
    {
        Fixture fixture(leftConfig());
        fixture.add(QStringLiteral("a"));
        const auto previous = fixture.add(QStringLiteral("b"));
        const auto opened = fixture.add(QStringLiteral("c"));
        QCOMPARE(fixture.focused(), std::optional(opened));
        fixture.remove(opened);
        QCOMPARE(fixture.focused(), std::optional(previous));
        VERIFY_INVARIANTS(fixture);
    }

    void windowThatBecomesPinnedMovesToItsSlot()
    {
        Fixture fixture(instantConfig());
        const auto first = fixture.add(QStringLiteral("a"));
        const auto later = fixture.add(QStringLiteral("pinned"));
        QCOMPARE(rowOrder(fixture, {first, later}), (QList<Layout::WindowId> {first, later}));
        fixture.setConfig(leftConfig());
        fixture.settle();
        QCOMPARE(rowOrder(fixture, {first, later}), (QList<Layout::WindowId> {later, first}));
        VERIFY_INVARIANTS(fixture);
    }

    void swappingAPinnedWindowIntoAnotherColumnKeepsBothWindows()
    {
        Fixture fixture(pinnedConfig(instantConfig(), QStringLiteral("pinned"), Config::ColumnPosition::End));
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QCOMPARE(fixture.state(pinned).columnIndex, fixture.state(second).columnIndex);
        QCOMPARE(fixture.focused(), std::optional(pinned));
        fixture.perform(QStringLiteral("swap-window-left"));
        QCOMPARE(fixture.focused(), std::optional(pinned));
        QCOMPARE(fixture.state(pinned).columnIndex, 1);
        QCOMPARE(fixture.state(pinned).tileIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(first).tileIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void pinningOnReloadKeepsTheRowInView()
    {
        Fixture fixture(instantConfig());
        fixture.add(QStringLiteral("a"));
        const auto later = fixture.add(QStringLiteral("pinned"));
        fixture.setConfig(leftConfig());
        QCOMPARE(fixture.frame(later).x(), 16.0);
        QCOMPARE(fixture.focused(), std::optional(later));
    }
    void columnThatLosesItsPinnedWindowLeavesThePinnedEnd()
    {
        Config::Config config = pinnedConfig(instantConfig(), QStringLiteral("pinned"), Config::ColumnPosition::End);
        config.layout.newWindowPlacement = Config::NewWindowPlacement::Stack;
        config.layout.maxRowsPerColumn = 2;
        Fixture fixture(config);
        const auto host = fixture.add(QStringLiteral("pinned"));
        Client unchanging;
        unchanging.fixedSize = QSizeF(500, 400);
        const auto guest = fixture.addClient(makeWindow(QStringLiteral("guest")), unchanging);
        const auto other = fixture.add(QStringLiteral("pinned"));
        QCOMPARE(fixture.state(guest).columnIndex, fixture.state(host).columnIndex);
        fixture.engine().activateWindow(host);
        QVERIFY(fixture.perform(QStringLiteral("move-column-right")).ok);
        QCOMPARE(rowOrder(fixture, {host, other}), (QList<Layout::WindowId> {other, host}));
        fixture.remove(host);
        QCOMPARE(rowOrder(fixture, {guest, other}), (QList<Layout::WindowId> {guest, other}));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutPins)
#include "test_layout_pins.moc"
