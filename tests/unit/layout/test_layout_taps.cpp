#include "helpers.h"

#include "layout/gestures/gesturerouter.h"

using namespace LayoutTest;

namespace
{

const QString Output = QStringLiteral("DP-1");
const QString KontrolPanelCommand = QStringLiteral("\"$HOME/.local/bin/portal-launcher\" toggle");

struct Rig
{
    explicit Rig(const Config::Config &config = instantConfig())
        : fixture(config, QRectF(0, 0, 1920, 1080), hooks())
        , router(fixture.engine())
    {
        router.setConfig(config.gestures);
    }

    Layout::Hooks hooks()
    {
        Layout::Hooks result;
        result.spawn = [this](const QString &command) { spawned.append(command); };
        return result;
    }

    bool touchpadTap(int fingers, qint64 durationMs = 80, double travelMm = 0.0)
    {
        for (qint32 slot = 0; slot < fingers; ++slot) {
            router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 0);
        }
        for (qint32 slot = 0; slot < fingers; ++slot) {
            router.touchpadContactMotion(slot, QPointF(20.0 + slot * 15.0 + travelMm, 30.0));
        }
        bool performed = false;
        for (qint32 slot = 0; slot < fingers; ++slot) {
            performed = router.touchpadContactUp(slot, durationMs) || performed;
        }
        fixture.settle();
        return performed;
    }

    QList<bool> touchscreenTap(int fingers, QPointF at, qint64 durationMs = 80)
    {
        QList<bool> consumed;
        for (qint32 id = 0; id < fingers; ++id) {
            consumed.append(router.touchDown(id, at + QPointF(id * 40.0, 0.0), 0, Output));
        }
        for (qint32 id = 0; id < fingers; ++id) {
            router.touchUp(id, durationMs);
        }
        fixture.settle();
        return consumed;
    }

    QStringList spawned;
    Fixture fixture;
    Layout::GestureRouter router;
};

}

class TestLayoutTaps : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void touchpadThreeFingerTapCyclesTheFocusedColumnWidth()
    {
        Rig rig;
        const auto window = rig.fixture.add();
        const double before = rig.fixture.frame(window).width();
        QVERIFY(rig.touchpadTap(3));
        QVERIFY(rig.fixture.frame(window).width() != before);

        Rig reference;
        const auto same = reference.fixture.add();
        reference.fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(rig.fixture.frame(window).width(), reference.fixture.frame(same).width());
    }

    void touchpadFourFingerTapTogglesTheKontrolPanel()
    {
        Rig rig;
        QVERIFY(rig.touchpadTap(4));
        QVERIFY(rig.touchpadTap(4));
        QCOMPARE(rig.spawned, QStringList({KontrolPanelCommand, KontrolPanelCommand}));
    }

    void touchpadTapsNeedQuickStillFingers()
    {
        Rig rig;
        rig.fixture.add();
        QVERIFY(!rig.touchpadTap(4, 400));
        QVERIFY(!rig.touchpadTap(4, 80, 5.0));
        QVERIFY(!rig.touchpadTap(2));
        QVERIFY(!rig.touchpadTap(5));
        QVERIFY(rig.touchpadTap(4, 80, 1.0));
        QCOMPARE(rig.spawned.size(), 1);
    }

    void touchpadSwipesPinchesAndClicksAreNotTaps()
    {
        Rig rig;
        rig.fixture.add();
        for (qint32 slot = 0; slot < 4; ++slot) {
            rig.router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 0);
        }
        rig.router.touchpadPinchBegin(4);
        for (qint32 slot = 0; slot < 4; ++slot) {
            QVERIFY(!rig.router.touchpadContactUp(slot, 50));
        }
        for (qint32 slot = 0; slot < 3; ++slot) {
            rig.router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 100);
        }
        rig.router.touchpadSwipeBegin(3, Output);
        rig.router.touchpadSwipeEnd();
        for (qint32 slot = 0; slot < 3; ++slot) {
            QVERIFY(!rig.router.touchpadContactUp(slot, 150));
        }
        for (qint32 slot = 0; slot < 3; ++slot) {
            rig.router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 200);
        }
        rig.router.touchpadPhysicalClick();
        for (qint32 slot = 0; slot < 3; ++slot) {
            QVERIFY(!rig.router.touchpadContactUp(slot, 250));
        }
        QVERIFY(rig.spawned.isEmpty());
    }

    void touchpadTapButtonIsTakenOnlyForThreeFingerTaps()
    {
        Rig rig;
        rig.touchpadTap(2);
        QVERIFY(!rig.router.takesTouchpadTapButton());
        rig.touchpadTap(3);
        QVERIFY(rig.router.takesTouchpadTapButton());
        for (qint32 slot = 0; slot < 3; ++slot) {
            rig.router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 500);
        }
        rig.router.touchpadPhysicalClick();
        QVERIFY(!rig.router.takesTouchpadTapButton());

        Config::Config config = instantConfig();
        config.gestures.touchpad.threeFingerTap = Config::TapAction::Off;
        Rig off(config);
        off.touchpadTap(3);
        QVERIFY(!off.router.takesTouchpadTapButton());
    }

    void touchscreenThreeFingerTapCyclesTheColumnUnderTheFingers()
    {
        Rig rig;
        const auto left = rig.fixture.add(QStringLiteral("left"));
        const auto right = rig.fixture.add(QStringLiteral("right"));
        rig.fixture.perform(QStringLiteral("focus-column-first"));
        const QRectF leftBefore = rig.fixture.frame(left);
        const QRectF rightBefore = rig.fixture.frame(right);
        const QList<bool> consumed = rig.touchscreenTap(3, rightBefore.center() - QPointF(40.0, 0.0));
        QCOMPARE(consumed, QList<bool>({false, false, true}));
        QCOMPARE(rig.fixture.focused(), std::optional(right));
        QVERIFY(rig.fixture.frame(right).width() != rightBefore.width());
        QCOMPARE(rig.fixture.frame(left).width(), leftBefore.width());
    }

    void touchscreenFourFingerTapTogglesTheKontrolPanel()
    {
        Rig rig;
        rig.fixture.add();
        QCOMPARE(rig.touchscreenTap(4, QPointF(600, 500)), QList<bool>({false, false, true, true}));
        QCOMPARE(rig.spawned, QStringList({KontrolPanelCommand}));
        rig.touchscreenTap(4, QPointF(600, 500), 600);
        QCOMPARE(rig.spawned.size(), 1);
    }

    void touchscreenSwipesAreNotTaps()
    {
        Rig rig;
        rig.fixture.add();
        for (qint32 id = 0; id < 4; ++id) {
            rig.router.touchDown(id, QPointF(600.0 + id * 40.0, 500.0), 0, Output);
        }
        for (int step = 1; step <= 4; ++step) {
            for (qint32 id = 0; id < 4; ++id) {
                rig.router.touchMotion(id, QPointF(600.0 + id * 40.0, 500.0 + step * 20.0), step * 10);
            }
        }
        for (qint32 id = 0; id < 4; ++id) {
            rig.router.touchUp(id, 60);
        }
        QVERIFY(rig.spawned.isEmpty());
    }

    void fiveFingerTapCanToggleTheOverview()
    {
        Config::Config config = instantConfig();
        config.gestures.touchpad.fiveFingerTap = Config::TapAction::ToggleOverview;
        config.gestures.touchscreen.enabled = false;
        Rig rig(config);
        rig.fixture.add();
        QVERIFY(rig.touchpadTap(5));
        QVERIFY(rig.fixture.engine().isOverviewOpen());
        QCOMPARE(rig.touchscreenTap(5, QPointF(600, 500)), QList<bool>({false, false, false, false, false}));
        QVERIFY(rig.fixture.engine().isOverviewOpen());
    }
};

QTEST_GUILESS_MAIN(TestLayoutTaps)
#include "test_layout_taps.moc"
