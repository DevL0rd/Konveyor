#include "helpers.h"

#include <QJsonArray>
#include <QJsonDocument>

using namespace LayoutTest;

namespace
{

Layout::ActionResult order(Fixture &fixture, const QList<QList<Layout::WindowId>> &groups, const QString &output = QStringLiteral("DP-1"))
{
    QJsonArray rows;
    for (const auto &group : groups) {
        QJsonArray ids;
        for (const auto id : group) {
            ids.append(static_cast<qint64>(id));
        }
        rows.append(ids);
    }
    return fixture.perform(
        QStringLiteral("order-taskbar-columns"), {output, QString::fromUtf8(QJsonDocument(rows).toJson(QJsonDocument::Compact))});
}

}

class TestLayoutTaskbar : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pinnedOrderIgnoresLaunchOrderAndPlacement_data()
    {
        QTest::addColumn<bool>("left");
        QTest::newRow("right") << false;
        QTest::newRow("left") << true;
    }

    void pinnedOrderIgnoresLaunchOrderAndPlacement()
    {
        QFETCH(bool, left);
        auto config = instantConfig();
        config.layout.newColumnPosition = left ? Config::NewColumnPosition::Left : Config::NewColumnPosition::Right;
        Fixture fixture(config);
        const auto c = fixture.add(QStringLiteral("c"));
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto result = order(fixture, {{a}, {b}, {c}});
        QVERIFY2(result.ok, qPrintable(result.error));
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(b).columnIndex, 1);
        QCOMPARE(fixture.state(c).columnIndex, 2);
        QCOMPARE(fixture.focused(), std::optional(b));
        VERIFY_INVARIANTS(fixture);
    }

    void manualIconMoveOrdersExistingColumns()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(order(fixture, {{c}, {a}, {b}}).ok);
        QCOMPARE(fixture.state(c).columnIndex, 0);
        QCOMPARE(fixture.state(a).columnIndex, 1);
        QCOMPARE(fixture.state(b).columnIndex, 2);
        QCOMPARE(fixture.focused(), std::optional(c));
        QVERIFY(order(fixture, {{c}, {a}, {b}}).ok);
        VERIFY_INVARIANTS(fixture);
    }

    void groupedColumnsKeepTheirRelativeOrder()
    {
        auto config = instantConfig();
        config.layout.groupAppWindows = Config::GroupAppWindows::Off;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto a2 = fixture.add(QStringLiteral("a"));
        QVERIFY(order(fixture, {{a2, a}, {b}}).ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(a2).columnIndex, 1);
        QCOMPARE(fixture.state(b).columnIndex, 2);
        QVERIFY(order(fixture, {{a2}, {b}, {a}}).ok);
        QCOMPARE(fixture.state(a2).columnIndex, 0);
        QCOMPARE(fixture.state(b).columnIndex, 1);
        QCOMPARE(fixture.state(a).columnIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void mixedStacksAndExplicitPinsStayPut()
    {
        auto config = instantConfig();
        auto pinRule = ruleFor(QStringLiteral("pinned"));
        pinRule.columnPosition = Config::ColumnPosition::Start;
        config.windowRules.append(pinRule);
        Fixture fixture(config);
        const auto pinned = fixture.add(QStringLiteral("pinned"));
        const auto a = fixture.add(QStringLiteral("a"));
        const auto x = fixture.add(QStringLiteral("x"));
        const auto y = fixture.add(QStringLiteral("y"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        const auto b = fixture.add(QStringLiteral("b"));
        const int mixedIndex = fixture.state(x).columnIndex;
        QVERIFY(order(fixture, {{b}, {y}, {x}, {a}, {pinned}}).ok);
        QCOMPARE(fixture.state(pinned).columnIndex, 0);
        QCOMPARE(fixture.state(x).columnIndex, mixedIndex);
        QCOMPARE(fixture.state(y).columnIndex, mixedIndex);
        QCOMPARE(fixture.state(b).columnIndex, 1);
        QCOMPARE(fixture.state(a).columnIndex, 3);
        VERIFY_INVARIANTS(fixture);
    }

    void repeatedWindowsInOneAppStackUseTheFirstIcon()
    {
        auto config = instantConfig();
        config.layout.groupAppWindows = Config::GroupAppWindows::Off;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto a2 = fixture.add(QStringLiteral("a"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(order(fixture, {{a2}, {c}, {a}, {b}, {c}}).ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(a2).columnIndex, 0);
        QCOMPARE(fixture.state(c).columnIndex, 1);
        QCOMPARE(fixture.state(b).columnIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void unknownAppsAndOtherOutputsStayPut()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto unknown = fixture.add(QString());
        const auto b = fixture.add(QStringLiteral("b"));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto c = fixture.add(QStringLiteral("c"));
        const auto d = fixture.add(QStringLiteral("d"));
        QVERIFY(order(fixture, {{d}, {b}, {unknown}, {a}, {c}}).ok);
        QCOMPARE(fixture.state(b).columnIndex, 0);
        QCOMPARE(fixture.state(unknown).columnIndex, 1);
        QCOMPARE(fixture.state(a).columnIndex, 2);
        QCOMPARE(fixture.state(c).columnIndex, 0);
        QCOMPARE(fixture.state(d).columnIndex, 1);
        QCOMPARE(fixture.focused(), std::optional(d));
        VERIFY_INVARIANTS(fixture);
    }

    void synchronizationWaitsForInteractiveMovesAndResizing()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.engine().beginResize(b, static_cast<quint8>(Layout::ResizeEdge::Right)));
        QVERIFY(order(fixture, {{b}, {a}}).ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        fixture.engine().endResize();
        QVERIFY(fixture.engine().beginWindowDrag(b, fixture.frame(b).center()));
        QVERIFY(order(fixture, {{b}, {a}}).ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        fixture.engine().endWindowDrag();
        QVERIFY(order(fixture, {{b}, {a}}).ok);
        QCOMPARE(fixture.state(b).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void effectiveGroupingUsesRulesAndWorkspaceLayout()
    {
        auto config = instantConfig();
        config.layout.groupAppWindows = Config::GroupAppWindows::Off;
        auto grouped = ruleFor(QStringLiteral("a"));
        grouped.groupAppWindows = Config::GroupAppWindows::Stack;
        config.windowRules.append(grouped);
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(a).taskbarGrouping, Config::GroupAppWindows::Stack);
        QCOMPARE(fixture.state(b).taskbarGrouping, Config::GroupAppWindows::Off);
        QCOMPARE(fixture.engine().workspaceStates().first().taskbarGrouping, Config::GroupAppWindows::Off);
        QVERIFY(fixture.state(a).taskbarEligible);
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QVERIFY(!fixture.state(a).taskbarEligible);
        QVERIFY(!fixture.state(b).taskbarEligible);
        VERIFY_INVARIANTS(fixture);
    }

    void scopedRevisionsDistinguishLaunchesAndBothEditDirections()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        auto state = fixture.engine().workspaceStates().first();
        QCOMPARE(state.taskbarSource, QStringLiteral("launch"));
        const auto launched = state.taskbarRevision;
        QVERIFY(order(fixture, {{b}, {a}}).ok);
        state = fixture.engine().workspaceStates().first();
        QCOMPARE(state.taskbarSource, QStringLiteral("taskbar"));
        QVERIFY(state.taskbarRevision > launched);
        const auto reordered = state.taskbarRevision;
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-index"), {QStringLiteral("2")}).ok);
        state = fixture.engine().workspaceStates().first();
        QCOMPARE(state.taskbarSource, QStringLiteral("layout"));
        QVERIFY(state.taskbarRevision > reordered);
        const QString ids = QStringLiteral("[[%1],[%2]]").arg(b).arg(a);
        QVERIFY(!fixture
                .perform(QStringLiteral("order-taskbar-columns"),
                    {QStringLiteral("DP-1"), ids, QString::number(state.id), QString::number(reordered)})
                .ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QVERIFY(
            !fixture.perform(QStringLiteral("order-taskbar-columns"), {QStringLiteral("DP-1"), ids, QString::number(state.id + 100)}).ok);
        VERIFY_INVARIANTS(fixture);
    }

    void effectiveGroupingFollowsOutputAndNamedWorkspaceOverrides()
    {
        auto config = instantConfig();
        Config::OutputConfig output;
        output.name = QStringLiteral("DP-1");
        output.layout.emplace();
        output.layout->groupAppWindows = Config::GroupAppWindows::Off;
        config.outputs.append(output);
        auto workspace = namedWorkspace(QStringLiteral("grouped"));
        workspace.layout.emplace();
        workspace.layout->groupAppWindows = Config::GroupAppWindows::Stack;
        config.workspaces.append(workspace);
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.state(a).taskbarGrouping, Config::GroupAppWindows::Stack);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(b).taskbarGrouping, Config::GroupAppWindows::Off);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QVERIFY(!fixture.state(b).taskbarEligible);
        VERIFY_INVARIANTS(fixture);
    }

    void orderDoesNotTouchInactiveWorkspacesOrFloatingWindows()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        const auto c = fixture.add(QStringLiteral("c"));
        const auto d = fixture.add(QStringLiteral("d"));
        const auto floating = fixture.add(QStringLiteral("floating"));
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        const auto rect = fixture.frame(floating);
        QVERIFY(order(fixture, {{b}, {floating}, {d}, {a}, {c}}).ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(b).columnIndex, 1);
        QCOMPARE(fixture.state(d).columnIndex, 0);
        QCOMPARE(fixture.state(c).columnIndex, 1);
        QCOMPARE(fixture.frame(floating), rect);
        QCOMPARE(fixture.focused(), std::optional(floating));
        VERIFY_INVARIANTS(fixture);
    }

    void invalidInputDoesNotMoveAnything()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        for (const QString &input : {QStringLiteral("broken"), QStringLiteral("{}"), QStringLiteral("[1]"), QStringLiteral("[[0]]"),
                 QStringLiteral("[[1.5]]"), QStringLiteral("[[true]]"), QStringLiteral("[[9007199254740992]]")}) {
            QVERIFY(!fixture.perform(QStringLiteral("order-taskbar-columns"), {QStringLiteral("DP-1"), input}).ok);
        }
        QVERIFY(order(fixture, {}).ok);
        QVERIFY(!order(fixture, {{b}, {a}}, QStringLiteral("missing")).ok);
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(b).columnIndex, 1);
    }
};

QTEST_GUILESS_MAIN(TestLayoutTaskbar)
#include "test_layout_taskbar.moc"
