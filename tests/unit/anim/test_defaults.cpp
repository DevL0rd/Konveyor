#include "anim/animation.h"
#include "anim/defaults.h"

#include <QTest>

using namespace std::chrono_literals;
using Konveyor::Config::AnimationParams;
using Konveyor::Config::Animations;
using Konveyor::Config::EasingCurve;
using Konveyor::Config::EasingParams;
using Konveyor::Config::SpringParams;

using Member = AnimationParams Animations::*;

namespace
{

AnimationParams fetchParams()
{
    QFETCH(QByteArray, member);
    const QHash<QByteArray, Member> members {
        {"workspace-switch", &Animations::workspaceSwitch},
        {"window-open", &Animations::windowOpen},
        {"window-close", &Animations::windowClose},
        {"horizontal-view-movement", &Animations::horizontalViewMovement},
        {"window-movement", &Animations::windowMovement},
        {"window-resize", &Animations::windowResize},
        {"overview-open-close", &Animations::overviewOpenClose},
    };
    return Konveyor::Anim::defaultAnimations().*members.value(member);
}

}

class TestDefaults : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void global()
    {
        const Animations animations = Konveyor::Anim::defaultAnimations();
        QVERIFY(animations.enabled);
        QCOMPARE(animations.slowdown, 1.0);
    }

    void springs_data()
    {
        QTest::addColumn<QByteArray>("member");
        QTest::addColumn<double>("stiffness");
        QTest::newRow("workspace-switch") << QByteArray("workspace-switch") << 1000.0;
        QTest::newRow("horizontal-view-movement") << QByteArray("horizontal-view-movement") << 800.0;
        QTest::newRow("window-movement") << QByteArray("window-movement") << 800.0;
        QTest::newRow("window-resize") << QByteArray("window-resize") << 800.0;
        QTest::newRow("overview-open-close") << QByteArray("overview-open-close") << 800.0;
    }

    void springs()
    {
        QFETCH(double, stiffness);
        const AnimationParams params = fetchParams();
        QVERIFY(params.enabled);
        QVERIFY(std::holds_alternative<SpringParams>(params.kind));
        const auto &spring = std::get<SpringParams>(params.kind);
        QCOMPARE(spring.dampingRatio, 1.0);
        QCOMPARE(spring.stiffness, stiffness);
        QCOMPARE(spring.epsilon, 0.0001);
    }

    void easings_data()
    {
        QTest::addColumn<QByteArray>("member");
        QTest::addColumn<EasingCurve>("curve");
        QTest::newRow("window-open") << QByteArray("window-open") << EasingCurve::EaseOutExpo;
        QTest::newRow("window-close") << QByteArray("window-close") << EasingCurve::EaseOutQuad;
    }

    void easings()
    {
        QFETCH(EasingCurve, curve);
        const AnimationParams params = fetchParams();
        QVERIFY(params.enabled);
        QVERIFY(std::holds_alternative<EasingParams>(params.kind));
        const auto &easing = std::get<EasingParams>(params.kind);
        QCOMPARE(easing.durationMs, 150.0);
        QCOMPARE(easing.curve, curve);

        const Konveyor::Anim::Animation animation(Konveyor::Anim::Clock::frozenAt(0ms), 0, 1, 0, params);
        QCOMPARE(animation.duration(), Konveyor::Anim::Duration(150ms));
    }
};

QTEST_GUILESS_MAIN(TestDefaults)
#include "test_defaults.moc"
