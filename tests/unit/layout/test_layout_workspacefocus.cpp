#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Main = QStringLiteral("DP-1");
const QString Other = QStringLiteral("DP-2");

struct ThreeWorkspaces
{
    Fixture fixture;
    WindowIds ids = fill();

    WindowIds fill()
    {
        WindowIds result;
        for (int idx = 0; idx < 3; ++idx) {
            result.append(fixture.add());
            act(fixture, QStringLiteral("focus-workspace-down"));
        }
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("1")});
        return result;
    }

    void go(const QString &name, const QStringList &arguments, int expectedIndex)
    {
        QVERIFY2(act(fixture, name, arguments).ok, qPrintable(name));
        QCOMPARE(activeWorkspaceOn(fixture, Main), expectedIndex);
        const auto expected = expectedIndex <= ids.size() ? std::optional(ids[expectedIndex - 1]) : std::nullopt;
        QCOMPARE(fixture.focused(), expected);
    }
};

}

class TestLayoutWorkspaceFocus : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusWorkspaceByIndexClampsToTheTrailingEmptyWorkspace()
    {
        ThreeWorkspaces f;
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("3")}, 3);
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("99")}, 4);
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("0")}, 1);
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 4);
        QVERIFY(!act(f.fixture, QStringLiteral("focus-workspace"), {QStringLiteral("256")}).ok);
        QVERIFY(!act(f.fixture, QStringLiteral("focus-workspace"), {QStringLiteral("nowhere")}).ok);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
    }

    void focusWorkspaceUpAndDownStopAtTheEnds()
    {
        ThreeWorkspaces f;
        f.go(QStringLiteral("focus-workspace-up"), {}, 1);
        for (const int index : {2, 3, 4, 4}) {
            f.go(QStringLiteral("focus-workspace-down"), {}, index);
        }
        f.go(QStringLiteral("focus-workspace-up"), {}, 3);
    }

    void focusWorkspacePreviousTogglesBetweenTheLastTwo()
    {
        ThreeWorkspaces f;
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("3")}, 3);
        f.go(QStringLiteral("focus-workspace-previous"), {}, 1);
        f.go(QStringLiteral("focus-workspace-previous"), {}, 3);
        f.go(QStringLiteral("focus-workspace-down"), {}, 4);
        f.go(QStringLiteral("focus-workspace-previous"), {}, 3);
    }

    void focusWorkspacePreviousFollowsTheWorkspaceAfterItMoves()
    {
        ThreeWorkspaces f;
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("3")}, 3);
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-up")).ok);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
        QVERIFY(act(f.fixture, QStringLiteral("focus-workspace-previous")).ok);
        COMPARE_FOCUS(f.fixture, f.ids[0]);
        QVERIFY(act(f.fixture, QStringLiteral("focus-workspace-previous")).ok);
        COMPARE_FOCUS(f.fixture, f.ids[2]);
    }

    void focusWorkspacePreviousDoesNothingOnceThatWorkspaceIsGone()
    {
        Fixture fixture;
        const auto id = fixture.add();
        act(fixture, QStringLiteral("focus-workspace-down"));
        const auto gone = fixture.add();
        act(fixture, QStringLiteral("focus-workspace-up"));
        fixture.remove(gone);
        fixture.advance(1);
        QCOMPARE(workspacesOn(fixture, Main).size(), 2);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-previous")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
        COMPARE_FOCUS(fixture, id);
        VERIFY_INVARIANTS(fixture);
    }

    void autoBackAndForthReturnsFromTheCurrentWorkspace()
    {
        Config::Config config = instantConfig();
        config.input.workspaceAutoBackAndForth = true;
        ThreeWorkspaces f;
        f.fixture.setConfig(config);
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("2")}, 2);
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("2")}, 1);
        f.go(QStringLiteral("focus-workspace"), {QStringLiteral("1")}, 2);
    }

    void focusWorkspaceByNameSwitchesToTheOutputThatHasIt()
    {
        Fixture fixture;
        const auto main = fixture.add();
        addOutputAt(fixture, Other, QRectF(1920, 0, 1920, 1080));
        const auto other = addOn(fixture, Other);
        act(fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("side")});
        fixture.engine().focusOutput(Main);
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("side")});
        QCOMPARE(focusedOutput(fixture), Other);
        COMPARE_FOCUS(fixture, other);
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("1")});
        QCOMPARE(focusedOutput(fixture), Other);
        act(fixture, QStringLiteral("focus-monitor-left"));
        COMPARE_FOCUS(fixture, main);
    }

    void workspaceNamesCanTargetAnotherWorkspaceByReference()
    {
        ThreeWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("three")},
            {{QStringLiteral("workspace"), QStringLiteral("3")}})
                .ok);
        QCOMPARE(namesOn(f.fixture, Main).value(2), QStringLiteral("three"));
        QVERIFY(!act(
            f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("x")}, {{QStringLiteral("workspace"), QStringLiteral("nope")}})
                .ok);
        QVERIFY(act(f.fixture, QStringLiteral("unset-workspace-name"), {QStringLiteral("three")}).ok);
        QCOMPARE(namesOn(f.fixture, Main).value(2), QString());
        QVERIFY(!act(f.fixture, QStringLiteral("unset-workspace-name"), {QStringLiteral("three")}).ok);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
    }

    void namedEmptyWorkspaceSurvivesUntilItsNameIsUnset()
    {
        Fixture fixture;
        fixture.add();
        act(fixture, QStringLiteral("focus-workspace-down"));
        act(fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("keep")});
        act(fixture, QStringLiteral("focus-workspace-up"));
        QCOMPARE(namesOn(fixture, Main), QStringList({QString(), QStringLiteral("keep"), QString()}));
        act(fixture, QStringLiteral("unset-workspace-name"), {QStringLiteral("keep")});
        QCOMPARE(namesOn(fixture, Main), QStringList({QString(), QString()}));
        VERIFY_INVARIANTS(fixture);
    }

    void namingTheEmptyWorkspaceAboveFirstKeepsAnEmptyOneAbove()
    {
        Config::Config config = instantConfig();
        config.layout.emptyWorkspaceAboveFirst = true;
        Fixture fixture(config);
        const auto id = fixture.add();
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("1")});
        act(fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("top")});
        QCOMPARE(namesOn(fixture, Main), QStringList({QString(), QStringLiteral("top"), QString(), QString()}));
        QCOMPARE(activeWorkspaceOn(fixture, Main), 2);
        QCOMPARE(placeOf(fixture, id), std::pair(Main, 3));
        VERIFY_INVARIANTS(fixture);
    }

    void aNameAlreadyUsedByAnotherWorkspaceIsRejected()
    {
        ThreeWorkspaces f;
        QVERIFY(act(
            f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("dup")}, {{QStringLiteral("workspace"), QStringLiteral("2")}})
                .ok);
        QVERIFY(!act(f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("DUP")}).ok);
        QVERIFY(act(f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("dup")},
            {{QStringLiteral("workspace"), QStringLiteral("dup")}})
                .ok);
        QCOMPARE(namesOn(f.fixture, Main), QStringList({QString(), QStringLiteral("dup"), QString(), QString()}));
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveWorkspaceToIndexClampsAndCanTargetAnotherWorkspace()
    {
        ThreeWorkspaces f;
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-index"), {QStringLiteral("99")}).ok);
        QCOMPARE(placeOf(f.fixture, f.ids[0]), std::pair(Main, 3));
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 4);
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-index"), {QStringLiteral("1")},
            {{QStringLiteral("reference"), QStringLiteral("2")}})
                .ok);
        QCOMPARE(placeOf(f.fixture, f.ids[2]), std::pair(Main, 1));
        QCOMPARE(placeOf(f.fixture, f.ids[1]), std::pair(Main, 2));
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 3);
        COMPARE_FOCUS(f.fixture, f.ids[0]);
        QVERIFY(!act(f.fixture, QStringLiteral("move-workspace-to-index"), {QStringLiteral("x")}).ok);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWorkspaceFocus)
#include "test_layout_workspacefocus.moc"
