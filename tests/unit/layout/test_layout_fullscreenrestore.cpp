#include "helpers.h"

using namespace LayoutTest;

namespace
{

Layout::WindowProperties lockedGame(QSizeF size = QSizeF(1920, 1080))
{
    auto properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), size);
    properties.isResizable = false;
    properties.minSize = properties.maxSize = size;
    properties.wantsFullscreen = true;
    return properties;
}

void restoreGame(Fixture &fixture, Layout::WindowId game, const Layout::WindowProperties &properties,
    const std::optional<Layout::RestorePlacement> &placement)
{
    fixture.engine().addWindow(game, properties, QString(), Layout::ActivationPolicy::Focus, placement);
    fixture.settle();
}

bool makeTabbedGame(Fixture &fixture, Layout::WindowId game)
{
    if (!fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok
        || !fixture.perform(QStringLiteral("toggle-column-tabbed-display")).ok) {
        return false;
    }
    fixture.engine().setWindowFullscreen(game, true);
    fixture.engine().setWindowFullscreen(game, false);
    return true;
}

void verifyNormalSizing(Fixture &fixture, Layout::WindowId game)
{
    QVERIFY(!fixture.state(game).isWindowedFullscreen);
    QCOMPARE(fixture.state(game).requestedSizingMode, Layout::WindowMode::Normal);
}

}

class TestLayoutFullscreenRestore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void outputLockedGameKeepsItsColumnAcrossFocusChanges_data()
    {
        QTest::addColumn<QSizeF>("size");
        QTest::newRow("standard") << QSizeF(1920, 1080);
        QTest::newRow("ultrawide") << QSizeF(5120, 1440);
    }

    void outputLockedGameKeepsItsColumnAcrossFocusChanges()
    {
        QFETCH(QSizeF, size);
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(QPointF(), size));
        const auto left = fixture.add();
        const auto game = fixture.addWith(lockedGame(size));
        const auto right = fixture.add();
        const auto farRight = fixture.add();
        const auto original = fixture.engine().placementOf(game);
        QVERIFY(original);
        QCOMPARE(original->columnIndex, std::size_t(1));
        for (const auto focused : {game, farRight, left, right}) {
            fixture.engine().activateWindow(focused);
            fixture.engine().setWindowFullscreen(game, false);
            fixture.settle();
            QVERIFY(!fixture.state(game).isFloating);
            QVERIFY(fixture.state(game).isWindowedFullscreen);
            QCOMPARE(fixture.engine().placementOf(game)->columnIndex, original->columnIndex);
            QCOMPARE(fixture.engine().placementOf(game)->width, original->width);
            fixture.engine().setWindowFullscreen(game, true);
            fixture.settle();
            QVERIFY(!fixture.state(game).isWindowedFullscreen);
            QVERIFY(!fixture.state(game).isFloating);
            QCOMPARE(fixture.engine().placementOf(game)->columnIndex, original->columnIndex);
            QCOMPARE(fixture.focused(), std::optional(focused));
            VERIFY_INVARIANTS(fixture);
        }
    }

    void persistentProtocolExitLetsFocusedColumnsOverlayTheGame()
    {
        Fixture fixture;
        const auto column = fixture.add();
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        fixture.settle();
        QVERIFY(!fixture.state(game).isFloating);
        QVERIFY(fixture.state(game).stackingIndex > fixture.state(column).stackingIndex);
        fixture.engine().activateWindow(column);
        fixture.settle();
        QVERIFY(fixture.state(column).stackingIndex > fixture.state(game).stackingIndex);
        fixture.engine().activateWindow(game);
        fixture.settle();
        QCOMPARE(fixture.frame(game), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.state(game).stackingIndex > fixture.state(column).stackingIndex);
        QVERIFY(fixture.state(game).isWindowedFullscreen);
        VERIFY_INVARIANTS(fixture);
    }

    void genuineSmallerWindowedModeEndsFallback()
    {
        Fixture fixture;
        fixture.add();
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        fixture.settle();
        auto properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(800, 600));
        fixture.engine().updateWindowProperties(game, properties);
        fixture.settle();
        QVERIFY(!fixture.state(game).isFloating);
        verifyNormalSizing(fixture, game);
        QVERIFY(fixture.frame(game).width() < 1920);
        VERIFY_INVARIANTS(fixture);
    }

    void outputResizeEndsObsoleteFallback()
    {
        Fixture fixture;
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        fixture.settle();
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(0, 0, 2560, 1440));
        verifyNormalSizing(fixture, game);
        VERIFY_INVARIANTS(fixture);
    }

    void explicitUserActionsLeaveFallback()
    {
        for (const auto action : {"fullscreen-window", "toggle-windowed-fullscreen", "toggle-window-floating"}) {
            Fixture fixture;
            const auto game = fixture.addWith(lockedGame());
            fixture.engine().setWindowFullscreen(game, false);
            fixture.settle();
            QVERIFY(fixture.perform(QString::fromLatin1(action)).ok);
            QCOMPARE(fixture.state(game).requestedSizingMode, Layout::WindowMode::Normal);
            QCOMPARE(fixture.state(game).isWindowedFullscreen, QString::fromLatin1(action) == QStringLiteral("toggle-windowed-fullscreen"));
            QVERIFY(!fixture.engine().placementOf(game)->isWindowedFullscreenFallback);
            if (QString::fromLatin1(action) == QStringLiteral("toggle-window-floating")) {
                QVERIFY(fixture.state(game).isFloating);
            }
            VERIFY_INVARIANTS(fixture);
        }
    }

    void hiddenGameRestoresItsPresentation_data()
    {
        QTest::addColumn<bool>("locked");
        QTest::newRow("still output locked") << true;
        QTest::newRow("now windowed") << false;
    }

    void hiddenGameRestoresItsPresentation()
    {
        QFETCH(bool, locked);
        Fixture fixture;
        fixture.add();
        const auto game = fixture.addWith(lockedGame());
        fixture.add();
        fixture.engine().setWindowFullscreen(game, false);
        const auto placement = fixture.engine().placementOf(game);
        QVERIFY(placement->isWindowedFullscreenFallback);
        fixture.remove(game);
        auto properties = locked ? lockedGame() : makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(800, 600));
        properties.wantsFullscreen = false;
        restoreGame(fixture, game, properties, placement);
        QVERIFY(!fixture.state(game).isFloating);
        QCOMPARE(fixture.state(game).isWindowedFullscreen, locked);
        QCOMPARE(fixture.state(game).requestedSizingMode, locked ? Layout::WindowMode::Fullscreen : Layout::WindowMode::Normal);
        QCOMPARE(fixture.engine().placementOf(game)->columnIndex, placement->columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void hiddenTabbedGameRespectsNormalColumnOnRestore()
    {
        Fixture fixture;
        const auto sibling = fixture.add();
        const auto game = fixture.addWith(lockedGame());
        QVERIFY(makeTabbedGame(fixture, game));
        const auto placement = fixture.engine().placementOf(game);
        fixture.remove(game);
        fixture.engine().activateWindow(sibling);
        QVERIFY(fixture.perform(QStringLiteral("fullscreen-window")).ok);
        auto properties = lockedGame();
        properties.wantsFullscreen = false;
        restoreGame(fixture, game, properties, placement);
        QVERIFY(!fixture.engine().placementOf(game)->isWindowedFullscreenFallback);
        verifyNormalSizing(fixture, game);
        VERIFY_INVARIANTS(fixture);
    }

    void outputMoveEndsObsoleteFallback()
    {
        Fixture fixture;
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(1920, 0, 1920, 1080));
        verifyNormalSizing(fixture, game);
        VERIFY_INVARIANTS(fixture);
    }

    void movingGameToAnotherOutputEndsFallback_data()
    {
        QTest::addColumn<QString>("action");
        for (const auto action : {"move-window-to-monitor-right", "move-column-to-monitor-right", "move-workspace-to-monitor-right"}) {
            QTest::newRow(action) << QString::fromLatin1(action);
        }
    }

    void movingGameToAnotherOutputEndsFallback()
    {
        QFETCH(QString, action);
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        QVERIFY(fixture.perform(action).ok);
        fixture.settle();
        verifyNormalSizing(fixture, game);
        QCOMPARE(fixture.state(game).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void movingTabbedColumnClearsInactiveFallback()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto sibling = fixture.add();
        const auto game = fixture.addWith(lockedGame());
        QVERIFY(makeTabbedGame(fixture, game));
        fixture.engine().activateWindow(sibling);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-monitor-right")).ok);
        fixture.settle();
        verifyNormalSizing(fixture, game);
        QCOMPARE(fixture.state(game).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void unpluggingOutputEndsFallback()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        fixture.engine().removeOutput(QStringLiteral("DP-1"));
        fixture.settle();
        verifyNormalSizing(fixture, game);
        VERIFY_INVARIANTS(fixture);
    }

    void mergingIntoNormalColumnEndsFallback()
    {
        Fixture fixture;
        fixture.add();
        const auto game = fixture.addWith(lockedGame());
        fixture.engine().setWindowFullscreen(game, false);
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        fixture.settle();
        QVERIFY(!fixture.engine().placementOf(game)->isWindowedFullscreenFallback);
        verifyNormalSizing(fixture, game);
        VERIFY_INVARIANTS(fixture);
    }

    void originallyFloatingLockedWindowReturnsToFloating_data()
    {
        QTest::addColumn<bool>("hidden");
        QTest::newRow("uninterrupted") << false;
        QTest::newRow("hidden and restored") << true;
    }

    void originallyFloatingLockedWindowReturnsToFloating()
    {
        QFETCH(bool, hidden);
        Fixture fixture;
        auto properties = lockedGame();
        properties.wantsFullscreen = false;
        const auto game = fixture.addWith(properties);
        QVERIFY(fixture.state(game).isFloating);
        fixture.engine().setWindowFullscreen(game, true);
        if (hidden) {
            const auto placement = fixture.engine().placementOf(game);
            fixture.remove(game);
            properties.wantsFullscreen = true;
            fixture.engine().addWindow(game, properties, QString(), Layout::ActivationPolicy::Focus, placement);
            fixture.settle();
        }
        fixture.engine().setWindowFullscreen(game, false);
        fixture.settle();
        QVERIFY(fixture.state(game).isFloating);
        verifyNormalSizing(fixture, game);
        VERIFY_INVARIANTS(fixture);
    }

    void ordinaryFloatingWindowKeepsItsBehavior()
    {
        Fixture fixture;
        const auto column = fixture.add();
        const auto floating = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        fixture.engine().activateWindow(column);
        fixture.settle();
        QVERIFY(fixture.state(floating).stackingIndex > fixture.state(column).stackingIndex);
        fixture.engine().setWindowFullscreen(floating, true);
        fixture.settle();
        fixture.engine().setWindowFullscreen(floating, false);
        fixture.settle();
        QVERIFY(fixture.state(floating).isFloating);
        QVERIFY(!fixture.state(floating).isWindowedFullscreen);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFullscreenRestore)
#include "test_layout_fullscreenrestore.moc"
