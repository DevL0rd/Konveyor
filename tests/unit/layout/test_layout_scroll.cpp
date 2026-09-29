#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config widthConfig(Config::CenterFocusedColumn mode, double width = 600)
{
    Config::Config config = instantConfig();
    config.layout.centerFocusedColumn = mode;
    config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {width});
    return config;
}

}

class TestLayoutScroll : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void closingTheLastColumnDoesNotLeaveDeadSpace()
    {
        Fixture fixture;
        fixture.add();
        fixture.add();
        const auto last = fixture.add();
        fixture.engine().activateWindow(last);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("90%")});
        fixture.remove(last);
        fixture.settle();
        double right = 0.0;
        for (const Layout::WindowState &state : fixture.engine().windowStates()) {
            right = std::max(right, state.targetFrame.right());
        }
        QVERIFY2(right >= 1903.5, qPrintable(QStringLiteral("strip right edge at %1").arg(right)));
        VERIFY_INVARIANTS(fixture);
    }

    void shrinkingTheLastColumnDoesNotLeaveDeadSpace()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        const auto last = fixture.add();
        fixture.engine().activateWindow(last);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("90%")});
        QCOMPARE(fixture.frame(last).right(), 1904.0);

        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        const QRectF shrunk = fixture.frame(last);
        const QString message = QStringLiteral("last column at x=%1 right=%2").arg(shrunk.left()).arg(shrunk.right());
        QVERIFY2(shrunk.right() >= 1903.5, qPrintable(message));
        QVERIFY2(fixture.frame(second).left() < shrunk.left(), "the column to the left must fill the freed space");
        QVERIFY(fixture.frame(first).left() < fixture.frame(second).left());
        VERIFY_INVARIANTS(fixture);
    }

    void cyclingPresetWidthsKeepsTheColumnOnScreen()
    {
        Fixture fixture;
        const QList<Layout::WindowId> windows {fixture.add(), fixture.add(), fixture.add()};
        for (const Layout::WindowId window : windows) {
            fixture.engine().activateWindow(window);
            for (int step = 0; step < 5; ++step) {
                fixture.perform(QStringLiteral("switch-preset-column-width"));
                const QRectF frame = fixture.frame(window);
                const QString message = QStringLiteral("window %1 step %2: x=%3 right=%4 w=%5")
                                            .arg(window)
                                            .arg(step)
                                            .arg(frame.left())
                                            .arg(frame.right())
                                            .arg(frame.width());
                QVERIFY2(frame.left() >= -0.5, qPrintable(message));
                QVERIFY2(frame.right() <= 1920.5, qPrintable(message));
            }
        }
        VERIFY_INVARIANTS(fixture);
    }

    void wideningActiveColumnScrollsItBackIntoView()
    {
        Fixture fixture;
        fixture.add();
        const auto second = fixture.add();
        fixture.engine().activateWindow(second);
        const QRectF before = fixture.frame(second);
        QVERIFY(before.right() <= 1920.0);

        for (const QString &width : {QStringLiteral("70%"), QStringLiteral("90%"), QStringLiteral("50%")}) {
            fixture.perform(QStringLiteral("set-column-width"), {width});
            const QRectF frame = fixture.frame(second);
            QVERIFY2(frame.left() >= -0.5, "widened column must not run off the left edge");
            QVERIFY2(frame.right() <= 1920.5, "widened column must not run off the right edge");
        }
        VERIFY_INVARIANTS(fixture);
    }

    void firstColumnGetsGapPadding()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Never));
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).x(), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysCenterFocusedColumn()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Always));
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).x(), (1920.0 - 600.0) / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysCenterKeepsFocusedColumnCentered()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Always));
        fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(second).x(), (1920.0 - 600.0) / 2.0);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(second).x(), (1920.0 - 600.0) / 2.0 + 600.0 + 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysCenterCentersTheFirstAndLastColumns()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Always));
        QList<Layout::WindowId> row;
        for (int i = 0; i < 4; ++i) {
            row.append(fixture.add(QStringLiteral("column")));
        }
        const double centered = (1920.0 - 600.0) / 2.0;
        QCOMPARE(fixture.frame(row.last()).x(), centered);
        fixture.perform(QStringLiteral("focus-column-first"));
        QCOMPARE(fixture.frame(row.first()).x(), centered);
        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.frame(row.at(1)).x(), centered);
        fixture.perform(QStringLiteral("focus-column-last"));
        QCOMPARE(fixture.frame(row.last()).x(), centered);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysCenterSingleColumnOnlyWhileItIsAlone()
    {
        Config::Config config = widthConfig(Config::CenterFocusedColumn::Never);
        config.layout.alwaysCenterSingleColumn = true;
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).x(), (1920.0 - 600.0) / 2.0);
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(id).x(), 16.0);
        QCOMPARE(fixture.frame(second).x(), 632.0);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysExpandSingleColumnOnlyWhileItIsAlone()
    {
        Config::Config config = widthConfig(Config::CenterFocusedColumn::Never);
        config.layout.alwaysExpandSingleColumn = true;
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id), QRectF(16.0, 16.0, 1888.0, 1048.0));
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(id).x(), 16.0);
        QCOMPARE(fixture.frame(id).width(), 600.0);
        QCOMPARE(fixture.frame(second).x(), 632.0);
        QCOMPARE(fixture.frame(second).width(), 600.0);
        fixture.engine().removeWindow(second);
        fixture.settle();
        QCOMPARE(fixture.frame(id), QRectF(16.0, 16.0, 1888.0, 1048.0));
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysExpandSingleColumnKeepsTheColumnsOwnWidth()
    {
        Config::Config config = widthConfig(Config::CenterFocusedColumn::Never);
        config.layout.alwaysExpandSingleColumn = true;
        Fixture fixture(config);
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("800")}).ok);
        QCOMPARE(fixture.frame(id).width(), 1888.0);
        fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(id).width(), 800.0);
        config.layout.alwaysExpandSingleColumn = false;
        fixture.setConfig(config);
        fixture.engine().removeWindow(fixture.focused().value());
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), 800.0);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysExpandSingleColumnLeavesFixedSizeAppsAlone()
    {
        Config::Config config = widthConfig(Config::CenterFocusedColumn::Never);
        config.layout.alwaysExpandSingleColumn = true;
        Fixture fixture(config);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);
        QCOMPARE(fixture.frame(id).size(), QSizeF(300, 200));
        QVERIFY(!fixture.state(id).isForceResizable);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysExpandSingleColumnFollowsTheSetting()
    {
        Config::Config config = widthConfig(Config::CenterFocusedColumn::Never);
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).width(), 600.0);
        config.layout.alwaysExpandSingleColumn = true;
        fixture.setConfig(config);
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), 1888.0);
        config.layout.alwaysExpandSingleColumn = false;
        fixture.setConfig(config);
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }

    void columnsScrollIntoView()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Never, 900));
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto third = fixture.add(QStringLiteral("c"));
        const QRectF frame = fixture.frame(third);
        QVERIFY(frame.x() >= 0.0);
        QVERIFY(frame.right() <= 1920.0);
        VERIFY_INVARIANTS(fixture);
    }

    void centerColumnCentersWithoutRevealingEmptySpace()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Never));
        const auto lone = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("center-column")).ok);
        QCOMPARE(fixture.frame(lone).x(), 16.0);

        QList<Layout::WindowId> row;
        for (int i = 0; i < 4; ++i) {
            row.append(fixture.add(QStringLiteral("more")));
        }
        const Layout::WindowId middle = row.at(1);
        fixture.engine().activateWindow(middle);
        QVERIFY(fixture.perform(QStringLiteral("center-column")).ok);
        QCOMPARE(fixture.frame(middle).x(), (1920.0 - 600.0) / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void centerVisibleColumnsNeverRevealsEmptySpace()
    {
        Fixture fixture(widthConfig(Config::CenterFocusedColumn::Never, 400));
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.perform(QStringLiteral("center-visible-columns")).ok);
        QCOMPARE(fixture.frame(first).x(), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenIgnoresStruts()
    {
        Config::Config config = widthConfig(Config::CenterFocusedColumn::Never);
        config.layout.struts = Config::Struts {40, 40, 40, 40};
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).x(), 56.0);
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void strutsShrinkTheWorkingArea()
    {
        Config::Config config = instantConfig();
        config.layout.struts = Config::Struts {100, 0, 50, 0};
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).y(), 50.0 + 16.0);
        QCOMPARE(fixture.frame(id).x(), 100.0 + 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void workAreaOffsetsTheLayout()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        Layout::Engine engine(clock);
        engine.setConfig(instantConfig());
        Layout::OutputInfo info = makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080));
        info.workArea = QRectF(0, 40, 1920, 1000);
        engine.addOutput(info);
        engine.addWindow(1, makeWindow(), QString(), Layout::ActivationPolicy::Focus);
        const auto state = engine.windowState(1);
        QVERIFY(state.has_value());
        QCOMPARE(state->targetFrame.y(), 56.0);
    }

    void outputOriginOffsetsGlobalCoordinates()
    {
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto id = fixture.add();
        QVERIFY(fixture.frame(id).x() >= 1920.0);
        VERIFY_INVARIANTS(fixture);
    }

    void windowAtFindsWindowsInGlobalCoordinates()
    {
        Fixture fixture;
        const auto id = fixture.add();
        const QRectF frame = fixture.frame(id);
        QCOMPARE(fixture.engine().windowAt(frame.center()), id);
        QCOMPARE(fixture.engine().windowAt(QPointF(5, 5)), std::optional<Layout::WindowId>());
    }
};

QTEST_GUILESS_MAIN(TestLayoutScroll)
#include "test_layout_scroll.moc"
