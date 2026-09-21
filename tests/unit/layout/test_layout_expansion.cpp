#include "helpers.h"

using namespace LayoutTest;

class TestLayoutExpansion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maximizeRequestTogglesOnlyTheColumnWidthAndKeepsItOnScreen()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        const auto third = fixture.add();
        fixture.engine().activateWindow(second);
        const QRectF before = fixture.frame(second);

        fixture.engine().toggleWindowFillWidth(second);
        fixture.settle();
        QCOMPARE(fixture.frame(second), QRectF(16, 16, 1888, 1048));
        QCOMPARE(fixture.state(second).sizingMode, Layout::WindowMode::Normal);

        fixture.engine().toggleWindowFillWidth(second);
        fixture.settle();
        QCOMPARE(fixture.frame(second).size(), before.size());
        QVERIFY(fixture.frame(second).left() >= 0.0 && fixture.frame(second).right() <= 1920.0);

        fixture.engine().activateWindow(third);
        fixture.settle();
        fixture.engine().toggleWindowFillWidth(first);
        fixture.settle();
        QCOMPARE(fixture.focused(), std::optional(first));
        QCOMPARE(fixture.frame(first), QRectF(16, 16, 1888, 1048));
        QCOMPARE(fixture.state(third).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void windowsThatOpenMaximizedTakeTheFullColumnWidthNotTheEdges()
    {
        Fixture fixture;
        Layout::WindowProperties maximized = LayoutTest::makeWindow(QStringLiteral("app"));
        maximized.wantsMaximized = true;
        const auto id = fixture.addWith(maximized);
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 1888, 1048));
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionGoesMaxSizeThenEdgesThenNormal()
    {
        Fixture fixture;
        fixture.add();
        const auto id = fixture.add();
        fixture.engine().activateWindow(id);
        const QRectF normal = fixture.frame(id);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 1888, 1048));

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id).size(), normal.size());
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionTemporarilyResizesFixedWindowThenRestoresNativeSize()
    {
        Fixture fixture;
        Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 1888, 1048));
        QVERIFY(fixture.state(id).isExpansionForceResizable);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.state(id).isExpansionForceResizable);

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id).size(), QSizeF(300, 200));
        QVERIFY(!fixture.state(id).isForceResizable);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionReturnsForceResizableWindowToItsPreset()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("game"));
        rule.forceResizable = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);
        fixture.perform(QStringLiteral("switch-preset-column-width"), {}, {{QStringLiteral("from-native"), QStringLiteral("true")}});
        const QSizeF preset = fixture.frame(id).size();

        fixture.perform(QStringLiteral("cycle-window-expansion"));
        fixture.perform(QStringLiteral("cycle-window-expansion"));
        fixture.perform(QStringLiteral("cycle-window-expansion"));

        QCOMPARE(fixture.frame(id).size(), preset);
        QVERIFY(fixture.state(id).isForceResizableByRule);
        QVERIFY(!fixture.state(id).isExpansionForceResizable);
        VERIFY_INVARIANTS(fixture);
    }

    void expandedWindowsCoverTheWholeOutputWithoutGaps()
    {
        Fixture fixture;
        fixture.add();
        const auto id = fixture.add();
        fixture.engine().activateWindow(id);
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutExpansion)
#include "test_layout_expansion.moc"
