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
};

QTEST_GUILESS_MAIN(TestLayoutPins)
#include "test_layout_pins.moc"
