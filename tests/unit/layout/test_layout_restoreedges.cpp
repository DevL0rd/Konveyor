#include "helpers.h"

using namespace LayoutTest;

class TestLayoutRestoreEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unknownWindowsHaveNoPlacement()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        QVERIFY(!fixture.engine().placementOf(999).has_value());
    }

    void addingAKnownWindowAgainChangesNothing()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        const QRectF before = fixture.frame(id);
        fixture.engine().addWindow(id, makeWindow(QStringLiteral("b")), QString(), Layout::ActivationPolicy::Focus);
        QCOMPARE(fixture.engine().windowStates().size(), 1);
        QCOMPARE(fixture.frame(id), before);
    }

    void restoringIntoAWorkspaceThatIsGoneFallsBackToPlacement()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        std::optional<Layout::RestorePlacement> placement = fixture.engine().placementOf(first);
        QVERIFY(placement);
        placement->workspace = 987654;
        placement->tileIndex = 0;
        fixture.engine().addWindow(100, makeWindow(QStringLiteral("c")), QString(), Layout::ActivationPolicy::Focus, placement);
        fixture.settle();
        QVERIFY(fixture.engine().hasWindow(100));
        VERIFY_INVARIANTS(fixture);
    }

    void restoringIntoAWorkspaceWithoutAnOutputPlacesTheTile()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const std::optional<Layout::RestorePlacement> placement = fixture.engine().placementOf(second);
        QVERIFY(placement && placement->tileIndex);
        fixture.removeOutput(QStringLiteral("DP-1"));
        fixture.engine().addWindow(100, makeWindow(QStringLiteral("c")), QString(), Layout::ActivationPolicy::Focus, placement);
        QVERIFY(fixture.engine().hasWindow(100));
        QVERIFY(fixture.engine().hasWindow(first));
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void floatingFramesApplyOnlyToFloatingWindows()
    {
        Fixture fixture;
        const auto tiled = fixture.add(QStringLiteral("a"));
        const QRectF before = fixture.frame(tiled);
        fixture.engine().setFloatingFrame(tiled, QRectF(10, 10, 300, 200));
        fixture.engine().setFloatingFrame(999, QRectF(10, 10, 300, 200));
        QCOMPARE(fixture.frame(tiled), before);
        VERIFY_INVARIANTS(fixture);
    }

    void restoredFloatingWindowsThatAreTiledKeepTheirPlace()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        const std::optional<Layout::RestorePlacement> base = fixture.engine().placementOf(first);
        QVERIFY(base);
        Layout::RestorePlacement placement = *base;
        placement.isFloating = true;
        placement.floatingFrame = QRectF(100, 100, 400, 300);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("b"));
        properties.wantsFullscreen = true;
        fixture.engine().addWindow(100, properties, QString(), Layout::ActivationPolicy::Focus, placement);
        QVERIFY(fixture.engine().hasWindow(100));
        QVERIFY(!fixture.state(100).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void restoringIntoTheSecondOutputsStack()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.add(QStringLiteral("home"));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const std::optional<Layout::RestorePlacement> placement = fixture.engine().placementOf(second);
        QVERIFY(placement && placement->tileIndex);
        fixture.remove(second);
        fixture.engine().addWindow(second, makeWindow(QStringLiteral("a")), QString(), Layout::ActivationPolicy::Focus, placement);
        QCOMPARE(fixture.state(second).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void removingUnknownWindowsChangesNothing()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        fixture.engine().removeWindow(999);
        QCOMPARE(fixture.engine().windowStates().size(), 1);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutRestoreEdges)
#include "test_layout_restoreedges.moc"
