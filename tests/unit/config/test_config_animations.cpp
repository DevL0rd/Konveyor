#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigAnimations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesAnimationSpring();
    void parsesAnimationEasing();
    void parsesCubicBezier();
    void rejectsMixedAnimationKinds();
    void rejectsBadCurveAndSpring();
    void animationsOffAndSlowdown();
    void ignoresUnrenderedAnimations();
};

void TestConfigAnimations::parsesAnimationSpring()
{
    const Config config = parsed(QStringLiteral(R"(
        animations {
            workspace-switch {
                spring damping-ratio=0.8 stiffness=500 epsilon=0.001
            }
        }
    )"));
    QCOMPARE(std::get<SpringParams>(config.animations.workspaceSwitch.kind), (SpringParams {0.8, 500, 0.001}));
    QCOMPARE(config.animations.workspaceSwitch.enabled, true);
}

void TestConfigAnimations::parsesAnimationEasing()
{
    const Config config = parsed(QStringLiteral(R"(
        animations {
            horizontal-view-movement {
                duration-ms 100
                curve "ease-out-expo"
            }
            window-open {
                off
            }
            window-resize {
                duration-ms 50
            }
        }
    )"));
    const auto movement = std::get<EasingParams>(config.animations.horizontalViewMovement.kind);
    QCOMPARE(movement.durationMs, 100.0);
    QCOMPARE(movement.curve, EasingCurve::EaseOutExpo);
    QCOMPARE(config.animations.windowOpen.enabled, false);
    const auto resize = std::get<EasingParams>(config.animations.windowResize.kind);
    QCOMPARE(resize.durationMs, 50.0);
    QCOMPARE(resize.curve, EasingCurve::EaseOutCubic);
}

void TestConfigAnimations::parsesCubicBezier()
{
    const Config config = parsed(QStringLiteral(R"(
        animations {
            window-close {
                curve "cubic-bezier" 0.05 0.7 0.1 1
            }
        }
    )"));
    const auto easing = std::get<EasingParams>(config.animations.windowClose.kind);
    QCOMPARE(easing.curve, EasingCurve::CubicBezier);
    QCOMPARE(easing.x1, 0.05);
    QCOMPARE(easing.y1, 0.7);
    QCOMPARE(easing.x2, 0.1);
    QCOMPARE(easing.y2, 1.0);
    QCOMPARE(easing.durationMs, 150.0);
}

void TestConfigAnimations::rejectsMixedAnimationKinds()
{
    const QString both
        = QStringLiteral("animations {\n  window-movement {\n    duration-ms 100\n    spring damping-ratio=1 stiffness=800 epsilon=0.001\n"
                         "  }\n}\n");
    QVERIFY(mustFail(both).message.contains(QStringLiteral("cannot set both spring and easing parameters at once")));

    const QString reversed
        = QStringLiteral("animations {\n  window-movement {\n    spring damping-ratio=1 stiffness=800 epsilon=0.001\n    duration-ms 100\n"
                         "  }\n}\n");
    QVERIFY(mustFail(reversed).message.contains(QStringLiteral("cannot set both spring and easing parameters at once")));
}

void TestConfigAnimations::rejectsBadCurveAndSpring()
{
    QVERIFY(mustFail(QStringLiteral("animations {\n window-open { curve \"bouncy\"; }\n}\n"))
            .message.contains(QStringLiteral("expected one of")));
    QVERIFY(mustFail(QStringLiteral("animations {\n window-open { spring stiffness=800 epsilon=0.001; }\n}\n"))
            .message.contains(QStringLiteral("`damping-ratio` is required")));
    QVERIFY(mustFail(QStringLiteral("animations {\n window-open { spring damping-ratio=0.01 stiffness=800 epsilon=0.001; }\n}\n"))
            .message.contains(QStringLiteral("damping-ratio must be between")));
    QVERIFY(mustFail(QStringLiteral("animations {\n window-open { spring damping-ratio=1 stiffness=800 epsilon=5; }\n}\n"))
            .message.contains(QStringLiteral("epsilon must be between")));
    QVERIFY(mustFail(QStringLiteral("animations {\n window-close { curve \"cubic-bezier\" 0.1 0.2; }\n}\n"))
            .message.contains(QStringLiteral("cubic-bezier requires")));
}

void TestConfigAnimations::animationsOffAndSlowdown()
{
    const Config off = parsed(QStringLiteral("animations {\n off\n slowdown 2.5\n}\n"));
    QCOMPARE(off.animations.enabled, false);
    QCOMPARE(off.animations.slowdown, 2.5);
    QCOMPARE(parsed(QStringLiteral("animations {\n off\n on\n}\n")).animations.enabled, true);
}

void TestConfigAnimations::ignoresUnrenderedAnimations()
{
    const LoadResult result = mustLoad(QStringLiteral(R"(
        animations {
            screenshot-ui-open { duration-ms 1; }
            config-notification-open-close { off; }
            exit-confirmation-open-close { off; }
            recent-windows-close { off; }
        }
    )"));
    QCOMPARE(result.warnings.size(), 4);
    QVERIFY(result.warnings.first().contains(QStringLiteral("screenshot-ui-open")));
}

QTEST_MAIN(TestConfigAnimations)
#include "test_config_animations.moc"
