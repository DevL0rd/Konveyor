#include "helpers.h"

using namespace LayoutTest;

namespace
{

Layout::ActionResult moveFloating(Fixture &fixture, const QString &x, const QString &y)
{
    return fixture.perform(QStringLiteral("move-floating-window"), {}, {{QStringLiteral("x"), x}, {QStringLiteral("y"), y}});
}

void verifyTiledWindowKeepsSize(const Layout::WindowProperties &properties)
{
    Fixture fixture;
    const QSizeF expected = properties.frameSize;
    const auto id = fixture.addWith(properties);
    QVERIFY(!fixture.state(id).isFloating);
    QCOMPARE(fixture.frame(id).size(), expected);
    QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("600")}).ok);
    QCOMPARE(fixture.frame(id).width(), expected.width());
    VERIFY_INVARIANTS(fixture);
}

Config::Config forceResizableConfig()
{
    Config::Config config = instantConfig();
    Config::WindowRule rule = ruleFor(QStringLiteral("game"));
    rule.forceResizable = true;
    config.windowRules.append(rule);
    return config;
}

Layout::WindowProperties nonResizableGame()
{
    Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
    properties.isResizable = false;
    return properties;
}

}

class TestLayoutFloating : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dialogsFloatAutomatically()
    {
        Fixture fixture;
        fixture.add();

        Layout::WindowProperties dialog = LayoutTest::makeWindow(QStringLiteral("popup"));
        dialog.isDialog = true;
        const auto asDialog = fixture.addWith(dialog);
        QVERIFY2(fixture.state(asDialog).isFloating, "a dialog must not join the tiling");

        Layout::WindowProperties fixedWidth = LayoutTest::makeWindow(QStringLiteral("fixed"));
        fixedWidth.minSize = QSizeF(400, 0);
        fixedWidth.maxSize = QSizeF(400, 0);
        const auto asFixed = fixture.addWith(fixedWidth);
        QVERIFY2(!fixture.state(asFixed).isFloating, "a fixed-size window can remain tiled");

        const auto normal = fixture.add(QStringLiteral("normal"));
        QVERIFY2(!fixture.state(normal).isFloating, "an ordinary window still tiles");
        VERIFY_INVARIANTS(fixture);
    }

    void toggleWindowFloating()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(!fixture.state(id).isFloating);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QVERIFY(fixture.state(id).isFloating);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QVERIFY(!fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void moveWindowToFloatingAndTiling()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-floating")).ok);
        QVERIFY(fixture.state(id).isFloating);
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-floating")).ok);
        QVERIFY(fixture.state(id).isFloating);
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-tiling")).ok);
        QVERIFY(!fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void windowWithParentAutoFloats()
    {
        Fixture fixture;
        const auto parent = fixture.add();
        Layout::WindowProperties child = makeWindow(QStringLiteral("dialog"), QStringLiteral("dialog"), QSizeF(300, 200));
        child.parent = parent;
        const auto id = fixture.addWith(child);
        QVERIFY(fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedSizeWindowTiles()
    {
        Layout::WindowProperties properties = makeWindow(QStringLiteral("fixed"), QStringLiteral("fixed"), QSizeF(300, 200));
        properties.minSize = QSizeF(300, 200);
        properties.maxSize = QSizeF(300, 200);
        verifyTiledWindowKeepsSize(properties);
    }

    void nonResizableWindowWithoutSizeHintsKeepsItsSize()
    {
        Layout::WindowProperties properties = makeWindow(QStringLiteral("fixed"), QStringLiteral("fixed"), QSizeF(300, 200));
        properties.isResizable = false;
        verifyTiledWindowKeepsSize(properties);
    }

    void disablingForceResizeUsesCurrentAppLimits()
    {
        Fixture fixture(forceResizableConfig());
        Layout::WindowProperties properties = nonResizableGame();
        const auto id = fixture.addWith(properties);
        QVERIFY(fixture.frame(id).size() != properties.frameSize);

        properties.minSize = QSizeF(420, 270);
        properties.maxSize = QSizeF(420, 270);
        properties.frameSize = fixture.frame(id).size();
        fixture.engine().updateWindowProperties(id, properties);
        fixture.setConfig(instantConfig());
        fixture.settle();
        QCOMPARE(fixture.frame(id).size(), QSizeF(420, 270));
        VERIFY_INVARIANTS(fixture);
    }

    void disablingForceResizeRestoresNativeSizeWithoutAppLimits()
    {
        Fixture fixture(forceResizableConfig());
        Layout::WindowProperties properties = nonResizableGame();
        const auto id = fixture.addWith(properties);
        const QSizeF forcedSize = fixture.frame(id).size();
        QVERIFY(forcedSize != properties.frameSize);

        properties.frameSize = forcedSize;
        fixture.engine().updateWindowProperties(id, properties);
        fixture.setConfig(instantConfig());
        fixture.settle();
        QCOMPARE(fixture.frame(id).size(), QSizeF(300, 200));
        VERIFY_INVARIANTS(fixture);
    }

    void nativeFloatingWidthCycleUsesClosestPreset()
    {
        Fixture fixture(nativeWidthCycleConfig(true));
        Layout::WindowProperties properties = nonResizableGame();
        properties.frameSize = QSizeF(550, 300);
        const auto id = fixture.addWith(properties);

        verifyNativeWidthCycle(fixture, id);
        VERIFY_INVARIANTS(fixture);
    }

    void openFloatingRuleWins()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^float$"));
        rule.matches.append(match);
        rule.openFloating = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("float"));
        QVERIFY(fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowIsCenteredByDefault()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        const QRectF frame = fixture.frame(id);
        QVERIFY(frame.width() > 0);
        QVERIFY(frame.x() >= 0);
        VERIFY_INVARIANTS(fixture);
    }

    void moveFloatingWindowByFixedPosition()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(moveFloating(fixture, QStringLiteral("100"), QStringLiteral("200")).ok);
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(100, 200));
        VERIFY_INVARIANTS(fixture);
    }

    void moveFloatingWindowByRelativePosition()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        moveFloating(fixture, QStringLiteral("100"), QStringLiteral("100"));
        QVERIFY(moveFloating(fixture, QStringLiteral("+50"), QStringLiteral("-25")).ok);
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(150, 75));
        VERIFY_INVARIANTS(fixture);
    }

    void directionalMovesLeaveFloatingWindowsWhereTheyAre()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        moveFloating(fixture, QStringLiteral("100"), QStringLiteral("100"));
        QVERIFY(fixture.perform(QStringLiteral("move-column-right")).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-window-down")).ok);
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(100, 100));
        VERIFY_INVARIANTS(fixture);
    }

    void directionalFocusGoesBackToTheColumns()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("first"));
        const auto second = fixture.add(QStringLiteral("second"));
        const auto floater = fixture.add(QStringLiteral("floater"), QSizeF(300, 200));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QCOMPARE(fixture.focused(), floater);
        QVERIFY(fixture.perform(QStringLiteral("focus-column-left")).ok);
        QVERIFY(fixture.focused() == first || fixture.focused() == second);
        for (int step = 0; step < 3; ++step) {
            QVERIFY(fixture.perform(QStringLiteral("focus-column-right")).ok);
            QVERIFY(fixture.focused() != floater);
            QVERIFY(fixture.perform(QStringLiteral("focus-window-down")).ok);
            QVERIFY(fixture.focused() != floater);
        }
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowsKeepTheGeometryTheyAreGiven()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        fixture.settle();
        const QRectF moved(300, 200, 900, 650);
        fixture.engine().setFloatingFrame(id, moved);
        fixture.settle();
        QCOMPARE(fixture.frame(id), moved);
        fixture.engine().setFloatingFrame(id, QRectF(10, 20, 500, 400));
        fixture.settle();
        QCOMPARE(fixture.frame(id), QRectF(10, 20, 500, 400));
        VERIFY_INVARIANTS(fixture);
    }

    void switchFocusBetweenFloatingAndTiling()
    {
        Fixture fixture;
        const auto tiled = fixture.add(QStringLiteral("tiled"));
        const auto floater = fixture.add(QStringLiteral("floater"));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QCOMPARE(fixture.focused(), floater);
        QVERIFY(fixture.perform(QStringLiteral("focus-tiling")).ok);
        QCOMPARE(fixture.focused(), tiled);
        QVERIFY(fixture.perform(QStringLiteral("focus-floating")).ok);
        QCOMPARE(fixture.focused(), floater);
        QVERIFY(fixture.perform(QStringLiteral("switch-focus-between-floating-and-tiling")).ok);
        QCOMPARE(fixture.focused(), tiled);
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowsStackAboveTiling()
    {
        Fixture fixture;
        const auto tiled = fixture.add(QStringLiteral("tiled"));
        const auto floater = fixture.add(QStringLiteral("floater"));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(floater).stackingIndex > fixture.state(tiled).stackingIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void childStacksAboveParent()
    {
        Fixture fixture;
        Layout::WindowProperties parentProperties = makeWindow(QStringLiteral("parent"), QStringLiteral("parent"), QSizeF(400, 300));
        parentProperties.minSize = QSizeF(400, 300);
        parentProperties.maxSize = QSizeF(400, 300);
        const auto parent = fixture.addWith(parentProperties);
        Layout::WindowProperties childProperties = makeWindow(QStringLiteral("child"), QStringLiteral("child"), QSizeF(200, 100));
        childProperties.parent = parent;
        const auto child = fixture.addWith(childProperties);
        QVERIFY(fixture.state(child).isFloating);
        QVERIFY(fixture.state(child).stackingIndex > fixture.state(parent).stackingIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void setWindowWidthOnFloatingWindow()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("600")}).ok);
        QCOMPARE(fixture.frame(id).width(), 600.0);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("500")}).ok);
        QCOMPARE(fixture.frame(id).height(), 500.0);
        VERIFY_INVARIANTS(fixture);
    }

    void centerFloatingWindow()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        moveFloating(fixture, QStringLiteral("0"), QStringLiteral("0"));
        QVERIFY(fixture.perform(QStringLiteral("center-window")).ok);
        const QRectF frame = fixture.frame(id);
        QCOMPARE(frame.x(), (1920.0 - frame.width()) / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void defaultFloatingPositionRule()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^corner$"));
        rule.matches.append(match);
        rule.openFloating = true;
        rule.defaultFloatingPosition = Config::FloatingPosition {10, 20, Config::FloatingRelativeTo::TopLeft};
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("corner"), QSizeF(300, 200));
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(10, 20));
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowsKeepPositionAcrossTilingRoundTrip()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        moveFloating(fixture, QStringLiteral("300"), QStringLiteral("400"));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(300, 400));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFloating)
#include "test_layout_floating.moc"
