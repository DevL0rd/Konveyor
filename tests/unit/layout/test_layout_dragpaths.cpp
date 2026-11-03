#include "helpers.h"

using namespace LayoutTest;

namespace
{

const QString Primary = QStringLiteral("DP-1");

Config::Config fixedWorkspaces()
{
    Config::Config config = instantConfig();
    config.layout.fixedWorkspaceCount = 3;
    return config;
}

Config::Config emptyAboveFirst()
{
    Config::Config config = instantConfig();
    config.layout.emptyWorkspaceAboveFirst = true;
    return config;
}

void dragToUnknownOutput(Fixture &fixture, Layout::WindowId id)
{
    QVERIFY(fixture.engine().beginWindowDrag(id, fixture.frame(id).center()));
    fixture.engine().updateWindowDrag(fixture.frame(id).center() + QPointF(400, 0), QStringLiteral("NOPE"));
    QCOMPARE(fixture.engine().movingWindow(), std::optional(id));
}

void expectWindowSurvivesOutputLoss(Fixture &fixture, Layout::WindowId id)
{
    fixture.settle();
    QVERIFY(fixture.engine().hasWindow(id));
    fixture.addOutput(Primary, QRectF(0, 0, 1920, 1080));
    QVERIFY(fixture.engine().windowState(id).has_value());
    VERIFY_INVARIANTS(fixture);
}

}

class TestLayoutDragPaths : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void detachingTowardsAnUnknownOutputKeepsThePrimary()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        dragToUnknownOutput(fixture, id);
        fixture.engine().updateWindowDrag(fixture.frame(id).center() + QPointF(420, 0), QStringLiteral("NOPE"));
        fixture.engine().endWindowDrag();
        QVERIFY(!fixture.engine().movingWindow());
        VERIFY_INVARIANTS(fixture);
    }

    void movingTowardsAnUnknownOutputIsIgnored()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(startMove(fixture, id, fixture.frame(id).center() + QPointF(400, 0)));
        fixture.engine().updateWindowDrag(QPointF(500, 500), QStringLiteral("NOPE"));
        fixture.engine().updateWindowDrag(QPointF(520, 500), Primary);
        fixture.engine().endWindowDrag();
        QCOMPARE(fixture.engine().windowState(id)->output, Primary);
        VERIFY_INVARIANTS(fixture);
    }

    void aFloatingDragCanBeToggledBeforeItMoves()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(id).isFloating);
        QVERIFY(fixture.engine().beginWindowDrag(id, fixture.frame(id).center()));
        QVERIFY(fixture.engine().toggleWindowDragFloating());
        QVERIFY(fixture.engine().movingWindow().has_value());
        QVERIFY(fixture.engine().toggleWindowDragFloating());
        fixture.engine().endWindowDrag();
        VERIFY_INVARIANTS(fixture);
    }

    void aTiledDragCanBecomeFloatingAndBack()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto id = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.engine().beginWindowDrag(id, fixture.frame(id).center()));
        QVERIFY(fixture.engine().toggleWindowDragFloating());
        QVERIFY(fixture.engine().toggleWindowDragFloating());
        QVERIFY(fixture.engine().toggleWindowDragFloating());
        fixture.engine().endWindowDrag();
        QVERIFY(fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
        QVERIFY(!Fixture().engine().toggleWindowDragFloating());
    }

    void cancellingARestoredFullscreenWindowRestoresItsMode()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("fullscreen-window"), {}, idProperty(id));
        QCOMPARE(fixture.state(id).requestedSizingMode, Layout::WindowMode::Fullscreen);
        QVERIFY(startMove(fixture, id, QPointF(900, 700)));
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).requestedSizingMode, Layout::WindowMode::Fullscreen);
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingAMaximizedWindowRestoresItsMode()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("maximize-window-to-edges"), {}, idProperty(id));
        QCOMPARE(fixture.state(id).requestedSizingMode, Layout::WindowMode::Maximized);
        QVERIFY(startMove(fixture, id, QPointF(900, 700)));
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).requestedSizingMode, Layout::WindowMode::Maximized);
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingAfterTheOriginWorkspaceVanishedMakesANewOne()
    {
        Fixture fixture;
        const auto lone = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto other = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("focus-workspace-up"));
        QVERIFY(fixture.state(lone).workspace != fixture.state(other).workspace);
        QVERIFY(startMove(fixture, lone, QPointF(900, 700)));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(1000);
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QVERIFY(fixture.engine().hasWindow(lone));
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingAfterEveryOutputWasRemovedKeepsTheWindow()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(startMove(fixture, id, QPointF(900, 700)));
        fixture.removeOutput(Primary);
        fixture.engine().cancelWindowDrag();
        expectWindowSurvivesOutputLoss(fixture, id);
    }

    void droppingAfterEveryOutputWasRemovedKeepsTheWindow()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(startMove(fixture, id, QPointF(900, 700)));
        fixture.removeOutput(Primary);
        fixture.engine().endWindowDrag();
        expectWindowSurvivesOutputLoss(fixture, id);
    }

    void droppingOnAnUnknownOutputAfterTheLastOutputWentKeepsTheWindow()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        dragToUnknownOutput(fixture, id);
        fixture.removeOutput(Primary);
        fixture.engine().endWindowDrag();
        expectWindowSurvivesOutputLoss(fixture, id);
    }

    void cancellingAfterTheOriginOutputWasRemovedUsesTheActiveOutput()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(startMove(fixture, id, QPointF(2900, 700), QStringLiteral("DP-2")));
        fixture.removeOutput(QStringLiteral("DP-2"));
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, Primary);
        VERIFY_INVARIANTS(fixture);
    }

    void otherWindowsStayReachableWhileADragMoves()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, id, QPointF(900, 700)));
        fixture.engine().updateWindowProperties(999, makeWindow(QStringLiteral("x")));
        fixture.engine().updateWindowProperties(id, makeWindow(QStringLiteral("a2")));
        fixture.engine().cancelWindowDrag();
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingADragFromTheSecondOutputReturnsThere()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.add(QStringLiteral("home"));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(startMove(fixture, id, QPointF(2900, 700), QStringLiteral("DP-2")));
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingFromTheLastWorkspaceRecreatesItAtTheEnd()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto lone = fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, lone, QPointF(900, 700)));
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.advance(1000);
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QVERIFY(fixture.engine().hasWindow(lone));
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingWhenNothingMovesJustEndsTheDrag()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.engine().cancelWindowDrag();
        QVERIFY(fixture.engine().beginWindowDrag(id, fixture.frame(id).center()));
        fixture.engine().cancelWindowDrag();
        QVERIFY(!fixture.engine().movingWindow());
        VERIFY_INVARIANTS(fixture);
    }

    void aSecondDragCannotStartAndUnknownWindowsCannotDrag()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        const auto other = fixture.add(QStringLiteral("b"));
        QVERIFY(!fixture.engine().beginWindowDrag(999, QPointF(10, 10)));
        QVERIFY(fixture.engine().beginWindowDrag(id, fixture.frame(id).center()));
        QVERIFY(!fixture.engine().beginWindowDrag(other, fixture.frame(other).center()));
        fixture.engine().endWindowDrag();
        VERIFY_INVARIANTS(fixture);
    }

    void droppingBetweenOverviewWorkspacesOpensANewOne_data()
    {
        QTest::addColumn<int>("config");
        QTest::newRow("dynamic") << 0;
        QTest::newRow("fixed") << 1;
        QTest::newRow("empty above") << 2;
    }

    void droppingBetweenOverviewWorkspacesOpensANewOne()
    {
        QFETCH(int, config);
        Config::Config prepared = config == 1 ? fixedWorkspaces() : config == 2 ? emptyAboveFirst() : instantConfig();
        Fixture fixture(prepared);
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.add(QStringLiteral("c"));
        fixture.engine().setOverviewOpen(true);
        fixture.advanceInSteps(500);
        int active = 0;
        int count = 0;
        for (const Layout::WorkspaceState &state : fixture.engine().workspaceStates()) {
            count += 1;
            active = state.isActive ? state.index : active;
        }
        constexpr double Pitch = 1188.0;
        QList<double> targets {-active * Pitch - 54.0, (count - 1 - active) * Pitch + 1100.0, 20.0};
        for (int index = 1; index < count; ++index) {
            targets.append((index - active) * Pitch - 54.0);
        }
        QVERIFY(startMove(fixture, first, QPointF(960, 200)));
        for (const double y : targets) {
            fixture.engine().updateWindowDrag(QPointF(960, y), Primary);
            QCOMPARE(fixture.engine().outputStates().size(), 1);
            fixture.engine().endWindowDrag();
            fixture.settle();
            QVERIFY(fixture.engine().hasWindow(first));
            VERIFY_INVARIANTS(fixture);
            QVERIFY(startMove(fixture, first, QPointF(960, 200)));
        }
        fixture.engine().endWindowDrag();
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutDragPaths)
#include "test_layout_dragpaths.moc"
