#include "helpers.h"

using namespace LayoutTest;

class TestLayoutWindowEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unknownWindowsAreIgnoredByEveryWindowCall()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        Layout::Engine &engine = fixture.engine();
        engine.updateWindowProperties(999, makeWindow(QStringLiteral("x")));
        engine.windowSizeCommitted(999, QSizeF(100, 100));
        engine.setWindowFullscreen(999, true);
        engine.setWindowUrgent(999, true);
        engine.toggleWindowFillWidth(999);
        engine.activateWindow(999);
        QCOMPARE(fixture.focused(), std::optional(id));
        QVERIFY(!fixture.state(id).isUrgent);
        VERIFY_INVARIANTS(fixture);
    }

    void openFocusedRulesDecideActivation_data()
    {
        QTest::addColumn<bool>("focused");
        QTest::newRow("focused") << true;
        QTest::newRow("unfocused") << false;
    }

    void openFocusedRulesDecideActivation()
    {
        QFETCH(bool, focused);
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("rule"));
        rule.openFocused = focused;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.addWith(makeWindow(QStringLiteral("rule")), Layout::ActivationPolicy::Smart);
        QCOMPARE(fixture.focused(), focused ? second : first);
        VERIFY_INVARIANTS(fixture);
    }

    void childWindowsIgnoreTheOpenAtColumnRule()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("dialog"));
        rule.openAtColumn = 1;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto parent = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        Layout::WindowProperties child = makeWindow(QStringLiteral("dialog"));
        child.parent = parent;
        const auto id = fixture.addWith(child);
        QVERIFY(fixture.state(id).isFloating);
        const auto plain = fixture.addWith(makeWindow(QStringLiteral("dialog")));
        QCOMPARE(fixture.state(plain).columnIndex, 0);
        Layout::WindowProperties tiledChild = makeWindow(QStringLiteral("dialog"));
        tiledChild.parent = parent;
        Config::Config tiledConfig = instantConfig();
        Config::WindowRule tiledRule = ruleFor(QStringLiteral("dialog"));
        tiledRule.openAtColumn = 1;
        tiledRule.openFloating = false;
        tiledConfig.windowRules.append(tiledRule);
        Fixture other(tiledConfig);
        const auto otherParent = other.add(QStringLiteral("a"));
        other.add(QStringLiteral("b"));
        tiledChild.parent = otherParent;
        const auto child2 = other.addWith(tiledChild);
        QVERIFY(other.engine().hasWindow(child2));
        VERIFY_INVARIANTS(other);
        VERIFY_INVARIANTS(fixture);
    }

    void childWindowsAreCheckedForAppSiblings()
    {
        Config::Config config = instantConfig();
        config.layout.floatChildWindows = true;
        Fixture fixture(config);
        fixture.add(QString());
        const auto second = fixture.add(QString());
        QVERIFY(!fixture.state(second).isFloating);
        const auto app = fixture.add(QStringLiteral("app"));
        const auto sibling = fixture.add(QStringLiteral("app"));
        QVERIFY(!fixture.state(app).isFloating);
        QVERIFY(fixture.state(sibling).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void changingParentsRestacksChildren()
    {
        Fixture fixture;
        const auto parent = fixture.add(QStringLiteral("a"));
        Layout::WindowProperties child = makeWindow(QStringLiteral("c"));
        child.parent = parent;
        const auto id = fixture.addWith(child);
        Layout::WindowProperties orphan = child;
        orphan.parent = std::nullopt;
        fixture.engine().updateWindowProperties(id, orphan);
        fixture.engine().updateWindowProperties(id, child);
        VERIFY_INVARIANTS(fixture);
    }

    void windowsBecomingOutputSizedCoverTheOutputInTheirColumn()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto id = fixture.add(QStringLiteral("game"));
        Layout::WindowProperties locked = makeWindow(QStringLiteral("game"), {}, QSizeF(1920, 1080));
        locked.isResizable = false;
        locked.minSize = QSizeF(1920, 1080);
        locked.maxSize = QSizeF(1920, 1080);
        fixture.engine().updateWindowProperties(id, locked);
        fixture.settle();
        QVERIFY(!fixture.state(id).isFloating);
        QVERIFY(fixture.state(id).isWindowedFullscreen);
        fixture.engine().updateWindowProperties(id, locked);
        Layout::WindowProperties smaller = locked;
        smaller.minSize = QSizeF(800, 600);
        fixture.engine().updateWindowProperties(id, smaller);
        QVERIFY(!fixture.state(id).isFloating);
        QVERIFY(!fixture.state(id).isWindowedFullscreen);
        VERIFY_INVARIANTS(fixture);
    }

    void onlyExactlyScreenSizedWindowsAreLocked()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto id = fixture.add(QStringLiteral("game"));
        Layout::WindowProperties wide = makeWindow(QStringLiteral("game"), {}, QSizeF(1920, 500));
        wide.isResizable = false;
        wide.minSize = QSizeF(1920, 500);
        wide.maxSize = QSizeF(1920, 500);
        fixture.engine().updateWindowProperties(id, wide);
        QVERIFY(!fixture.state(id).isFloating);
        Layout::WindowProperties unbounded = wide;
        unbounded.minSize = QSizeF(1920, 1080);
        unbounded.maxSize = QSizeF(2000, 1080);
        fixture.engine().updateWindowProperties(id, unbounded);
        QVERIFY(!fixture.state(id).isFloating);
        Layout::WindowProperties resizable = wide;
        resizable.isResizable = true;
        fixture.engine().updateWindowProperties(id, resizable);
        QVERIFY(!fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void windowsOfOrphanedWorkspacesAreNeverOutputSized()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.removeOutput(QStringLiteral("DP-1"));
        Layout::WindowProperties locked = makeWindow(QStringLiteral("a"), {}, QSizeF(1920, 1080));
        locked.isResizable = false;
        locked.minSize = QSizeF(1920, 1080);
        locked.maxSize = QSizeF(1920, 1080);
        fixture.engine().updateWindowProperties(id, locked);
        fixture.engine().windowSizeCommitted(id, QSizeF(500, 400));
        QVERIFY(fixture.engine().hasWindow(id));
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void enginesWithoutOutputsStillRateWindows()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        Layout::Engine engine(clock);
        engine.setConfig(instantConfig());
        QVERIFY(engine.wantsWindow(makeWindow(QStringLiteral("a"))));
        engine.addWindow(1, makeWindow(QStringLiteral("a")), QString(), Layout::ActivationPolicy::Focus);
        QVERIFY(engine.hasWindow(1));
        QCOMPARE(engine.checkConsistency(), QString());
    }
};

QTEST_GUILESS_MAIN(TestLayoutWindowEdges)
#include "test_layout_windowedges.moc"
