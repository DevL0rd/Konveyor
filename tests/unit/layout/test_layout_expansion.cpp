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

    void cycleExpansionGoesThroughWhatShowsThenBackToNormal_data()
    {
        QTest::addColumn<bool>("alone");
        QTest::addColumn<QList<QRectF>>("steps");
        QTest::newRow("beside another column: full width, edges")
            << false << QList<QRectF> {QRectF(16, 16, 1888, 1048), QRectF(0, 0, 1920, 1080)};
        QTest::newRow("alone and already expanded: edges") << true << QList<QRectF> {QRectF(0, 0, 1920, 1080)};
    }

    void cycleExpansionGoesThroughWhatShowsThenBackToNormal()
    {
        QFETCH(bool, alone);
        QFETCH(QList<QRectF>, steps);
        Config::Config config = instantConfig();
        config.layout.alwaysExpandSingleColumn = alone;
        Fixture fixture(config);
        if (!alone) {
            fixture.add();
        }
        const auto id = fixture.add();
        const QRectF normal = fixture.frame(id);
        for (const QRectF &step : std::as_const(steps)) {
            fixture.perform(QStringLiteral("cycle-window-expansion"));
            QCOMPARE(fixture.frame(id), step);
        }
        fixture.perform(QStringLiteral("cycle-window-expansion"));
        QCOMPARE(fixture.frame(id), normal);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        fixture.add();
        QCOMPARE(fixture.frame(id).width(), 936.0);
        VERIFY_INVARIANTS(fixture);
    }

    void cycleExpansionTemporarilyResizesFixedWindowThenRestoresNativeSize_data()
    {
        QTest::addColumn<bool>("alwaysExpand");
        QTest::newRow("always-expand-single-column off") << false;
        QTest::newRow("always-expand-single-column on") << true;
    }

    void cycleExpansionTemporarilyResizesFixedWindowThenRestoresNativeSize()
    {
        QFETCH(bool, alwaysExpand);
        Config::Config config = instantConfig();
        config.layout.alwaysExpandSingleColumn = alwaysExpand;
        Fixture fixture(config);
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
