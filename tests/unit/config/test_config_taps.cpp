#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigTaps : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsMapThreeAndFourFingerTaps()
    {
        const Config defaults = parsed(QStringLiteral(""));
        for (const MultiTouch &touch : {defaults.gestures.touchpad, defaults.gestures.touchscreen}) {
            QCOMPARE(touch.threeFingerTap, TapAction::CycleWidth);
            QCOMPARE(touch.fourFingerTap, TapAction::KontrolPanel);
            QCOMPARE(touch.fiveFingerTap, TapAction::Off);
            QCOMPARE(touch.tap(2), TapAction::Off);
        }
    }

    void parsesTapActions()
    {
        const Config config = parsed(QStringLiteral(R"(
            gestures {
                touchpad {
                    three-finger-tap "off"
                    four-finger-tap "cycle-width"
                    five-finger-tap "toggle-overview"
                }
                touchscreen {
                    three-finger-tap "kontrol-panel"
                }
            }
        )"));
        QCOMPARE(config.gestures.touchpad.tap(3), TapAction::Off);
        QCOMPARE(config.gestures.touchpad.tap(4), TapAction::CycleWidth);
        QCOMPARE(config.gestures.touchpad.tap(5), TapAction::ToggleOverview);
        QCOMPARE(config.gestures.touchscreen.tap(3), TapAction::KontrolPanel);
        QCOMPARE(config.gestures.touchscreen.tap(4), TapAction::KontrolPanel);
        QVERIFY(mustFail(QStringLiteral("gestures {\n touchpad {\n three-finger-tap \"wobble\"\n }\n}\n"))
                .message.contains(QStringLiteral("expected one of")));
    }
};

QTEST_MAIN(TestConfigTaps)
#include "test_config_taps.moc"
