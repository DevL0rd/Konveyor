#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Primary = QStringLiteral("DP-1");

const std::pair<QString, QString> NoFocus {QStringLiteral("focus"), QStringLiteral("false")};

struct TwoWorkspaces
{
    Fixture fixture;
    Layout::WindowId upper = fixture.add(QStringLiteral("upper"));
    Layout::WindowId lower = addBelow();

    Layout::WindowId addBelow()
    {
        act(fixture, QStringLiteral("focus-workspace-down"));
        const Layout::WindowId id = fixture.add(QStringLiteral("lower"));
        act(fixture, QStringLiteral("focus-workspace-up"));
        return id;
    }
};

void compareActiveWorkspace(Fixture &fixture, int index, std::optional<Layout::WindowId> focused)
{
    QCOMPARE(activeWorkspaceOn(fixture, Primary), index);
    QCOMPARE(fixture.focused(), focused);
    VERIFY_INVARIANTS(fixture);
}

}

class TestLayoutWorkspaceMoves : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveWindowToWorkspaceByIndexTakesFocusAlong()
    {
        TwoWorkspaces f;
        const auto second = f.fixture.add();
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("2")}).ok);
        QCOMPARE(columnsOn(f.fixture, Primary, 2), (Columns {{f.lower}, {second}}));
        compareActiveWorkspace(f.fixture, 2, second);
    }

    void moveWindowToWorkspaceWithoutFocusStaysBehind()
    {
        TwoWorkspaces f;
        const auto second = f.fixture.add();
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("2")}, {NoFocus}).ok);
        QCOMPARE(placeOf(f.fixture, second), std::pair(Primary, 2));
        compareActiveWorkspace(f.fixture, 1, f.upper);
    }

    void moveWindowToWorkspacePastTheEndOpensANewWorkspace()
    {
        TwoWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("99")}).ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 2));
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Primary, 1));
        QCOMPARE(workspacesOn(f.fixture, Primary).size(), 3);
        compareActiveWorkspace(f.fixture, 2, f.upper);
    }

    void moveWindowToItsOwnWorkspaceChangesNothing()
    {
        TwoWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("1")}).ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 1));
        compareActiveWorkspace(f.fixture, 1, f.upper);
    }

    void moveWindowToWorkspaceReportsBadReferences()
    {
        TwoWorkspaces f;
        QVERIFY(!act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("missing")}).ok);
        QVERIFY(!act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("300")}).ok);
        QVERIFY(!act(f.fixture, QStringLiteral("move-window-to-workspace")).ok);
        QVERIFY(!act(
            f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("1")}, {{QStringLiteral("id"), QStringLiteral("77")}})
                .ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 1));
    }

    void moveWindowToWorkspaceByIdMovesThatWindow()
    {
        TwoWorkspaces f;
        const auto second = f.fixture.add();
        const QString id = QString::number(f.upper);
        QVERIFY(
            act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("2")}, {{QStringLiteral("id"), id}, NoFocus}).ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 2));
        compareActiveWorkspace(f.fixture, 1, second);
    }

    void moveWindowToANamedWorkspace()
    {
        TwoWorkspaces f;
        act(f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("mail")},
            {{QStringLiteral("workspace"), QStringLiteral("2")}});
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace"), {QStringLiteral("mail")}).ok);
        QCOMPARE(workspacesOn(f.fixture, Primary).first().name, QStringLiteral("mail"));
        QCOMPARE(columnsOn(f.fixture, Primary, 1), (Columns {{f.lower}, {f.upper}}));
        compareActiveWorkspace(f.fixture, 1, f.upper);
    }

    void moveColumnToWorkspaceDownCarriesTheStack()
    {
        Fixture fixture;
        const WindowIds stack = stackOf(fixture, 2);
        const auto other = fixture.add();
        act(fixture, QStringLiteral("focus-column-first"));
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-down")).ok);
        QCOMPARE(columnsOn(fixture, Primary, 1), (Columns {{other}}));
        QCOMPARE(columnsOn(fixture, Primary, 2), (Columns {stack}));
        compareActiveWorkspace(fixture, 2, stack[1]);
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-up")).ok);
        QCOMPARE(columnsOn(fixture, Primary, 1).size(), 2);
        QCOMPARE(workspacesOn(fixture, Primary).size(), 2);
        compareActiveWorkspace(fixture, 1, stack[1]);
    }

    void moveColumnToWorkspaceWithoutFocusStaysBehind()
    {
        TwoWorkspaces f;
        const auto second = f.fixture.add();
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-workspace-down"), {}, {NoFocus}).ok);
        QCOMPARE(columnsOn(f.fixture, Primary, 2), (Columns {{f.lower}, {second}}));
        compareActiveWorkspace(f.fixture, 1, f.upper);
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-workspace"), {QStringLiteral("2")}, {NoFocus}).ok);
        QCOMPARE(columnsOn(f.fixture, Primary, 2), (Columns {{f.lower}, {f.upper}, {second}}));
        QCOMPARE(workspacesOn(f.fixture, Primary).size(), 3);
        compareActiveWorkspace(f.fixture, 1, std::nullopt);
    }

    void moveColumnToWorkspaceUpFromTheFirstWorkspaceChangesNothing()
    {
        TwoWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-workspace-up")).ok);
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace-up")).ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 1));
        compareActiveWorkspace(f.fixture, 1, f.upper);
    }

    void moveColumnToWorkspaceDownFromTheTrailingEmptyWorkspaceChangesNothing()
    {
        TwoWorkspaces f;
        act(f.fixture, QStringLiteral("focus-workspace"), {QStringLiteral("3")});
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-workspace-down")).ok);
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-workspace-up")).ok);
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace-up")).ok);
        QCOMPARE(workspacesOn(f.fixture, Primary).size(), 3);
        compareActiveWorkspace(f.fixture, 3, std::nullopt);
    }

    void moveColumnToWorkspaceMovesAFocusedFloatingWindow()
    {
        Fixture fixture;
        const auto tiled = fixture.add();
        const auto floating = addFloating(fixture);
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-down")).ok);
        QCOMPARE(placeOf(fixture, floating), std::pair(Primary, 2));
        QVERIFY(fixture.state(floating).isFloating);
        QCOMPARE(placeOf(fixture, tiled), std::pair(Primary, 1));
        compareActiveWorkspace(fixture, 2, floating);
    }

    void moveWindowToWorkspaceDownKeepsAFloatingWindowFloating()
    {
        Fixture fixture;
        fixture.add();
        const auto floating = addFloating(fixture);
        const QRectF frame = fixture.frame(floating);
        QVERIFY(act(fixture, QStringLiteral("move-window-to-workspace-down"), {}, {NoFocus}).ok);
        QVERIFY(fixture.state(floating).isFloating);
        QCOMPARE(fixture.frame(floating).size(), frame.size());
        QCOMPARE(fixture.frame(floating).x(), frame.x());
        QCOMPARE(placeOf(fixture, floating), std::pair(Primary, 2));
        QCOMPARE(activeWorkspaceOn(fixture, Primary), 1);
    }

    void moveWindowToWorkspaceUpWithoutFocusStaysBehind()
    {
        TwoWorkspaces f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        const auto second = f.fixture.add();
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace-up"), {}, {NoFocus}).ok);
        QCOMPARE(placeOf(f.fixture, second), std::pair(Primary, 1));
        compareActiveWorkspace(f.fixture, 2, f.lower);
    }

    void moveWorkspaceDownSwapsWithTheWorkspaceBelow()
    {
        TwoWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-down")).ok);
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Primary, 1));
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 2));
        compareActiveWorkspace(f.fixture, 2, f.upper);
    }

    void moveWorkspaceDownFromTheLastOccupiedWorkspaceKeepsItLast()
    {
        TwoWorkspaces f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-down")).ok);
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Primary, 2));
        QCOMPARE(workspacesOn(f.fixture, Primary).size(), 3);
        compareActiveWorkspace(f.fixture, 2, f.lower);
    }

    void moveWorkspaceUpFromTheTrailingEmptyWorkspaceLeavesAnEmptyGapUntilItIsLeft()
    {
        TwoWorkspaces f;
        act(f.fixture, QStringLiteral("focus-workspace"), {QStringLiteral("3")});
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-down")).ok);
        QCOMPARE(workspacesOn(f.fixture, Primary).size(), 3);
        compareActiveWorkspace(f.fixture, 3, std::nullopt);
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-up")).ok);
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Primary, 3));
        compareActiveWorkspace(f.fixture, 2, std::nullopt);
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Primary, 2));
        QCOMPARE(workspacesOn(f.fixture, Primary).size(), 3);
        compareActiveWorkspace(f.fixture, 2, f.lower);
    }

    void moveWorkspaceUpFromTheFirstWorkspaceChangesNothing()
    {
        TwoWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-up")).ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Primary, 1));
        compareActiveWorkspace(f.fixture, 1, f.upper);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWorkspaceMoves)
#include "test_layout_workspacemoves.moc"
