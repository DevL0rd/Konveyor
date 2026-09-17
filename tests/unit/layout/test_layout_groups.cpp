#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config groupConfig(Config::GroupAppWindows mode, int maxRows = 3)
{
    Config::Config config = instantConfig();
    config.layout.groupAppWindows = mode;
    config.layout.maxRowsPerColumn = maxRows;
    return config;
}

QList<int> rowsPerColumn(Fixture &fixture, const QList<Layout::WindowId> &ids)
{
    QMap<int, int> rows;
    for (const Layout::WindowId id : ids) {
        rows[fixture.state(id).columnIndex] += 1;
    }
    return rows.values();
}

Config::WindowRule ruleFor(const QString &appId)
{
    Config::WindowRule rule;
    Config::Match match;
    match.appId = QRegularExpression(QStringLiteral("^") + appId + QStringLiteral("$"));
    rule.matches.append(match);
    return rule;
}

}

class TestLayoutGroups : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void besideOpensRightOfTheAppsLastColumn()
    {
        Fixture fixture(groupConfig(Config::GroupAppWindows::Beside));
        const auto chat = fixture.add(QStringLiteral("chat"));
        const auto editor = fixture.add(QStringLiteral("editor"));
        fixture.engine().activateWindow(editor);
        fixture.settle();
        const auto secondChat = fixture.add(QStringLiteral("chat"));
        QCOMPARE(fixture.state(chat).columnIndex, 0);
        QCOMPARE(fixture.state(secondChat).columnIndex, 1);
        QCOMPARE(fixture.state(editor).columnIndex, 2);
        QCOMPARE(fixture.focused(), std::optional(secondChat));
        VERIFY_INVARIANTS(fixture);
    }

    void stackFillsTheColumnUpToMaxRows()
    {
        Fixture fixture(groupConfig(Config::GroupAppWindows::Stack, 3));
        QList<Layout::WindowId> terms;
        for (int i = 0; i < 3; ++i) {
            terms.append(fixture.add(QStringLiteral("term")));
        }
        QCOMPARE(rowsPerColumn(fixture, terms), QList<int>({3}));
        QCOMPARE(fixture.focused(), std::optional(terms.last()));
        VERIFY_INVARIANTS(fixture);
    }

    void overflowOpensAColumnAndRebalancesLeftHeavy()
    {
        Fixture fixture(groupConfig(Config::GroupAppWindows::Stack, 3));
        QList<Layout::WindowId> terms;
        const QList<QList<int>> expected {{1}, {2}, {3}, {2, 2}, {3, 2}, {3, 3}, {3, 2, 2}};
        for (const QList<int> &rows : expected) {
            terms.append(fixture.add(QStringLiteral("term")));
            QCOMPARE(rowsPerColumn(fixture, terms), rows);
            QCOMPARE(fixture.focused(), std::optional(terms.last()));
            VERIFY_INVARIANTS(fixture);
        }
    }

    void rebalancingLeavesOtherAppsAlone()
    {
        Fixture fixture(groupConfig(Config::GroupAppWindows::Stack, 2));
        const auto editor = fixture.add(QStringLiteral("editor"));
        QList<Layout::WindowId> terms;
        for (int i = 0; i < 3; ++i) {
            terms.append(fixture.add(QStringLiteral("term")));
        }
        QCOMPARE(fixture.state(editor).columnIndex, 0);
        QCOMPARE(rowsPerColumn(fixture, terms), QList<int>({2, 1}));
        const auto editorTwo = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.state(editorTwo).columnIndex, 0);
        QCOMPARE(fixture.state(editor).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void perAppRulesOverrideModeAndMaxRows()
    {
        Config::Config config = groupConfig(Config::GroupAppWindows::Beside, 3);
        Config::WindowRule terminal = ruleFor(QStringLiteral("term"));
        terminal.groupAppWindows = Config::GroupAppWindows::Stack;
        terminal.maxRowsPerColumn = 2;
        config.windowRules.append(terminal);
        Fixture fixture(config);
        QList<Layout::WindowId> terms;
        for (int i = 0; i < 3; ++i) {
            terms.append(fixture.add(QStringLiteral("term")));
        }
        QCOMPARE(rowsPerColumn(fixture, terms), QList<int>({2, 1}));
        const auto chat = fixture.add(QStringLiteral("chat"));
        const auto secondChat = fixture.add(QStringLiteral("chat"));
        QVERIFY(fixture.state(chat).columnIndex != fixture.state(secondChat).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void floatChildWindowsFloatsAnAppsLaterWindows()
    {
        Config::Config config = groupConfig(Config::GroupAppWindows::Beside);
        config.layout.floatChildWindows = true;
        Fixture fixture(config);
        const auto main = fixture.add(QStringLiteral("steam"));
        const auto friends = fixture.add(QStringLiteral("steam"));
        const auto editor = fixture.add(QStringLiteral("editor"));
        QVERIFY(!fixture.state(main).isFloating);
        QVERIFY(fixture.state(friends).isFloating);
        QVERIFY(!fixture.state(editor).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void floatChildWindowsRuleAppliesPerApp()
    {
        Config::Config config = groupConfig(Config::GroupAppWindows::Beside);
        Config::WindowRule steam = ruleFor(QStringLiteral("steam"));
        steam.floatChildWindows = true;
        config.windowRules.append(steam);
        Fixture fixture(config);
        fixture.add(QStringLiteral("steam"));
        const auto chat = fixture.add(QStringLiteral("steam"));
        fixture.add(QStringLiteral("term"));
        const auto secondTerm = fixture.add(QStringLiteral("term"));
        QVERIFY(fixture.state(chat).isFloating);
        QVERIFY(!fixture.state(secondTerm).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void offKeepsTheOldBehaviour()
    {
        Fixture fixture(groupConfig(Config::GroupAppWindows::Off));
        const auto first = fixture.add(QStringLiteral("term"));
        const auto other = fixture.add(QStringLiteral("editor"));
        const auto second = fixture.add(QStringLiteral("term"));
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(other).columnIndex, 1);
        QCOMPARE(fixture.state(second).columnIndex, 2);
    }
};

QTEST_GUILESS_MAIN(TestLayoutGroups)
#include "test_layout_groups.moc"
