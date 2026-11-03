#include "striphelpers.h"

using namespace LayoutTest;

class TestLayoutWindow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fixedWindowsRememberTheirNativeSize()
    {
        Layout::WindowProperties properties = makeWindow(QStringLiteral("a"), {}, QSizeF(640, 480));
        properties.isResizable = false;
        const Layout::LayoutWindow window(1, properties);
        QCOMPARE(window.nativeSize(), std::optional(QSize(640, 480)));
    }

    void fixedWindowsWithoutAFrameHaveNoNativeSize()
    {
        Layout::WindowProperties properties = makeWindow(QStringLiteral("a"), {}, QSizeF());
        properties.isResizable = false;
        const Layout::LayoutWindow window(1, properties);
        QVERIFY(!window.nativeSize());
    }

    void updatedPropertiesNeverChangeTheNativeSize()
    {
        Layout::WindowProperties fixed = makeWindow(QStringLiteral("a"), {}, QSizeF(640, 480));
        fixed.isResizable = false;
        Layout::LayoutWindow window(1, fixed);
        Layout::WindowProperties larger = fixed;
        larger.frameSize = QSizeF(800, 600);
        window.setProperties(larger);
        QCOMPARE(window.nativeSize(), std::optional(QSize(640, 480)));
        Layout::WindowProperties resizable = makeWindow(QStringLiteral("a"), {}, QSizeF(900, 700));
        window.setProperties(resizable);
        QCOMPARE(window.nativeSize(), std::optional(QSize(640, 480)));
    }

    void theClientsOwnChoiceBecomesTheNativeSize()
    {
        Layout::WindowProperties fixed = makeWindow(QStringLiteral("a"), {}, QSizeF(640, 480));
        fixed.isResizable = false;
        Layout::LayoutWindow window(1, fixed);
        window.setNativeSize(QSize(800, 600));
        QCOMPARE(window.nativeSize(), std::optional(QSize(800, 600)));
        window.setNativeSize(QSize());
        QCOMPARE(window.nativeSize(), std::optional(QSize(800, 600)));
    }

    void declaredLimitsOfAForcedSizeDoNotReplaceTheNativeSize()
    {
        Layout::WindowProperties fixed = makeWindow(QStringLiteral("a"), {}, QSizeF(640, 480));
        fixed.isResizable = false;
        Layout::LayoutWindow window(1, fixed);
        Layout::EffectiveWindowRules forced;
        forced.forceResizable = true;
        window.setRules(forced);
        QCOMPARE(window.minSize(), QSize(0, 0));
        Layout::WindowProperties redeclared = fixed;
        redeclared.minSize = redeclared.maxSize = QSizeF(1888, 1048);
        redeclared.frameSize = QSizeF(1888, 1048);
        window.setProperties(redeclared);
        window.setRules(Layout::EffectiveWindowRules());
        QCOMPARE(window.minSize(), QSize(640, 480));
        QCOMPARE(window.maxSize(), QSize(640, 480));
    }

    void opacityOverrideEndsWhenTheRuleStopsDimming()
    {
        Layout::LayoutWindow window(1, makeWindow());
        Layout::EffectiveWindowRules dim;
        dim.opacity = 0.5;
        QVERIFY(window.setRules(dim));
        window.toggleIgnoreOpacityRule();
        QVERIFY(window.isIgnoringOpacityRule());
        Layout::EffectiveWindowRules dimmer;
        dimmer.opacity = 0.4;
        QVERIFY(window.setRules(dimmer));
        QVERIFY(window.isIgnoringOpacityRule());
        Layout::EffectiveWindowRules opaque;
        opaque.opacity = 1.0;
        QVERIFY(window.setRules(opaque));
        QVERIFY(!window.isIgnoringOpacityRule());
        window.toggleIgnoreOpacityRule();
        QVERIFY(window.setRules(Layout::EffectiveWindowRules()));
        QVERIFY(!window.isIgnoringOpacityRule());
        QVERIFY(!window.setRules(Layout::EffectiveWindowRules()));
    }

    void zeroRequestedSidesKeepTheCurrentSize()
    {
        Layout::LayoutWindow window(1, makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        window.requestSize(QSize(0, 200), Layout::WindowMode::Normal, false);
        QCOMPARE(window.pendingSize(), std::optional(QSize(400, 200)));
        window.requestSize(QSize(250, 0), Layout::WindowMode::Normal, false);
        QCOMPARE(window.pendingSize(), std::optional(QSize(250, 300)));
    }

    void zeroRequestedSidesWithoutACurrentSizeHaveNoPendingSize()
    {
        Layout::LayoutWindow window(1, makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        window.requestSize(QSize(500, 500), Layout::WindowMode::Maximized, false);
        window.commit(QSizeF(1920, 1080));
        QVERIFY(!window.pendingSize());
        window.requestSize(QSize(0, 300), Layout::WindowMode::Normal, false);
        QVERIFY(!window.pendingSize());
        window.requestSize(QSize(300, 0), Layout::WindowMode::Normal, false);
        QVERIFY(!window.pendingSize());
        window.requestSize(QSize(0, 0), Layout::WindowMode::Normal, false);
        QVERIFY(!window.pendingSize());
        window.requestSize(QSize(300, 200), Layout::WindowMode::Normal, false);
        QCOMPARE(window.pendingSize(), std::optional(QSize(300, 200)));
    }

    void pendingSizeFollowsTheWindowAfterAOneShotRequest()
    {
        Layout::LayoutWindow window(1, makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        window.requestSizeUntilCommit(QSize(500, 400), false);
        QCOMPARE(window.pendingSize(), std::optional(QSize(500, 400)));
        window.commit(QSizeF(500, 400));
        window.requestSizeUntilCommit(QSize(500, 400), false);
        QCOMPARE(window.pendingSize(), std::optional(QSize(500, 400)));
    }

    void windowedFullscreenRequestsDoNotChangeThePendingSize()
    {
        Layout::LayoutWindow window(1, makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        window.requestWindowedFullscreen(true);
        QCOMPARE(window.pendingSize(), std::optional(QSize(400, 300)));
    }

    void tilesWithoutARequestReportTheirPendingSize()
    {
        FloatKit kit;
        Layout::Tile tile = kit.makeTile(makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        QCOMPARE(tile.targetWindowSize(), tile.pendingWindowSize());
        tile.requestOuterSize(QSizeF(500, 0), false);
        QCOMPARE(tile.targetWindowSize().width(), 500.0);
    }

    void fadesCanOnlyBeHeldOnceStarted()
    {
        FloatKit kit;
        Layout::Tile tile = kit.makeTile();
        tile.keepFadeAfterFinish();
        tile.ensureFadesToOpaque();
        QCOMPARE(tile.alpha(), 1.0);
        Config::AnimationParams params;
        Config::EasingParams easing;
        easing.durationMs = 100;
        easing.curve = Config::EasingCurve::Linear;
        params.enabled = true;
        params.kind = easing;
        tile.fadeOpacity(1.0, 0.5, params);
        tile.keepFadeAfterFinish();
        tile.ensureFadesToOpaque();
        QCOMPARE(tile.alpha(), 1.0);
        tile.fadeOpacity(1.0, 1.0, params);
        tile.ensureFadesToOpaque();
        QCOMPARE(tile.alpha(), 1.0);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWindow)
#include "test_layout_window.moc"
