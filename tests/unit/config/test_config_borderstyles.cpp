#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

void verifyRejectedAtMarker(const QString &text, const QString &message)
{
    const LoadError error = mustFail(unmarked(text));
    QVERIFY2(error.message.contains(message, Qt::CaseInsensitive), qPrintable(error.message));
    const qsizetype marker = text.indexOf(u'»');
    const qsizetype lineStart = text.lastIndexOf(u'\n', marker) + 1;
    QCOMPARE(error.location.line, text.left(marker).count(u'\n') + 1);
    QCOMPARE(error.location.column, marker - lineStart + 1);
}

}

class TestConfigBorderStyles : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesEveryLayerProperty();
    void parsesDrivesAndFireOn();
    void driveDefaultsAndReplacement();
    void parsesFocusRingStyleSlots();
    void parsesEventEffects();
    void eventModeDefaults();
    void parsesReduceMotionAndBeatThreshold();
    void warnsAboutUnknownStyles();
    void warnsAboutUnknownStylesInPerOutputEventEffects();
    void rejectsBadStyleNames_data();
    void rejectsBadStyleNames();
    void rejectsBadLayerValues_data();
    void rejectsBadLayerValues();
    void rejectsBadEventEffects_data();
    void rejectsBadEventEffects();
};

void TestConfigBorderStyles::parsesEveryLayerProperty()
{
    const Config config = parsed(QStringLiteral(R"(
        border-style "Everything" {
            layer "particles" {
                colors "#ff0000" "#00ff00"
                speed 2
                width 6
                opacity 0.5
                blend "add"
                angle 45
                count 100
                tail 0.4
                dash 10
                gap 5
                rate 30
                size 4
                lifetime 2
                direction "up"
                burst 12
                thickness 3
                sensitivity 2
                smoothing 0.5
                baseline 0.3
                position 0.7
            }
        }
    )"));
    QCOMPARE(config.borderStyles.size(), 1);
    QCOMPARE(config.borderStyles.first().name, QStringLiteral("Everything"));
    const BorderLayer layer = config.borderStyles.first().layers.first();
    QCOMPARE(layer.kind, BorderLayerKind::Particles);
    QCOMPARE(layer.colors.size(), 2);
    QCOMPARE(layer.colors.at(0).color, QColor(255, 0, 0));
    QCOMPARE(layer.colors.at(1).color, QColor(0, 255, 0));
    QCOMPARE(layer.speed, 2.0);
    QCOMPARE(layer.width, 6.0);
    QCOMPARE(layer.opacity, 0.5);
    QCOMPARE(layer.blend, BorderBlend::Add);
    QCOMPARE(layer.angle, 45.0);
    QCOMPARE(layer.count, 100);
    QCOMPARE(layer.tail, 0.4);
    QCOMPARE(layer.dash, 10.0);
    QCOMPARE(layer.gap, 5.0);
    QCOMPARE(layer.rate, 30.0);
    QCOMPARE(layer.size, 4.0);
    QCOMPARE(layer.lifetime, 2.0);
    QCOMPARE(layer.direction, ParticleDirection::Up);
    QCOMPARE(layer.burst, 12);
    QCOMPARE(layer.thickness, 3.0);
    QCOMPARE(layer.sensitivity, 2.0);
    QCOMPARE(layer.smoothing, 0.5);
    QCOMPARE(layer.baseline, 0.3);
    QCOMPARE(layer.position, 0.7);
}

void TestConfigBorderStyles::parsesDrivesAndFireOn()
{
    const Config config = parsed(QStringLiteral(R"(
        border-style "Club" {
            layer "particles" {
                colors "#ffffff"
                drive "width" "audio.bass" amount=18 smooth=0.05 curve="square"
                drive "opacity" "audio.level"
                fire-on "audio.beat" "click"
            }
        }
    )"));
    const BorderLayer layer = config.borderStyles.first().layers.first();
    QCOMPARE(layer.fireOn, (QList<Signal> {Signal::AudioBeat, Signal::Click}));
    const auto width = std::ranges::find_if(layer.drives, [](const LayerDrive &drive) { return drive.setting == QLatin1String("width"); });
    QVERIFY(width != layer.drives.end());
    QCOMPARE(width->signal, Signal::AudioBass);
    QCOMPARE(width->amount, 18.0);
    QCOMPARE(width->smooth, 0.05);
    QCOMPARE(width->curve, DriveCurve::Square);
    const auto opacity
        = std::ranges::find_if(layer.drives, [](const LayerDrive &drive) { return drive.setting == QLatin1String("opacity"); });
    QVERIFY(opacity != layer.drives.end());
    QCOMPARE(opacity->amount, 1.0);
    QCOMPARE(opacity->smooth, 0.0);
    QCOMPARE(opacity->curve, DriveCurve::Linear);
}

void TestConfigBorderStyles::driveDefaultsAndReplacement()
{
    const Config config = parsed(QStringLiteral(R"(
        border-style "Twice" {
            layer "gradient" {
                drive "speed" "pointer.speed"
                drive "speed" "typing.rate" amount=3
            }
        }
    )"));
    const BorderLayer layer = config.borderStyles.first().layers.first();
    int speedDrives = 0;
    for (const LayerDrive &drive : layer.drives) {
        if (drive.setting == QLatin1String("speed")) {
            ++speedDrives;
        }
    }
    QCOMPARE(speedDrives, 2);
}

void TestConfigBorderStyles::parsesFocusRingStyleSlots()
{
    const Config config = parsed(QStringLiteral(R"(
        border-style "A" { layer "gradient" { colors "#ff0000"; }; }
        layout {
            focus-ring {
                active-style "A"
                inactive-style "none"
                urgent-style "A"
            }
            border {
                active-style "A"
            }
        }
    )"));
    QCOMPARE(config.layout.focusRing.activeStyle, QStringLiteral("A"));
    QCOMPARE(config.layout.focusRing.inactiveStyle, QStringLiteral("none"));
    QCOMPARE(config.layout.focusRing.urgentStyle, QStringLiteral("A"));
    QCOMPARE(config.layout.border.activeStyle, QStringLiteral("A"));
    QVERIFY(config.layout.border.inactiveStyle.isEmpty());
}

void TestConfigBorderStyles::parsesEventEffects()
{
    const Config config = parsed(QStringLiteral(R"(
        border-style "Pulse-Out" { layer "pulse-front" { colors "#ffffff"; }; }
        layout {
            event-effects {
                on "click" "Pulse-Out" reach="neighbors" mode="while" duration=2.5
                on "unfocus" "none"
                on "view" "Pulse-Out" reach="all"
            }
        }
    )"));
    const EventEffects &effects = config.layout.eventEffects;
    const auto click = effects.of(EventTrigger::Click);
    QVERIFY(click.has_value());
    QCOMPARE(click->style, QStringLiteral("Pulse-Out"));
    QCOMPARE(click->reach, EventReach::Neighbors);
    QCOMPARE(click->mode, EventMode::While);
    QCOMPARE(click->duration, std::optional<double>(2.5));
    const auto unfocus = effects.of(EventTrigger::Unfocus);
    QVERIFY(unfocus.has_value());
    QCOMPARE(unfocus->style, QStringLiteral("none"));
    QCOMPARE(unfocus->reach, EventReach::Self);
    QVERIFY(!unfocus->duration.has_value());
    QCOMPARE(effects.of(EventTrigger::View)->reach, EventReach::All);
    QVERIFY(!effects.of(EventTrigger::Focus).has_value());
}

void TestConfigBorderStyles::eventModeDefaults()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            event-effects {
                on "scroll" "none"
                on "view" "none"
                on "click" "none"
                on "keypress" "none"
            }
        }
    )"));
    const EventEffects &effects = config.layout.eventEffects;
    QCOMPARE(effects.of(EventTrigger::Scroll)->mode, EventMode::While);
    QCOMPARE(effects.of(EventTrigger::View)->mode, EventMode::While);
    QCOMPARE(effects.of(EventTrigger::Click)->mode, EventMode::Once);
    QCOMPARE(effects.of(EventTrigger::Keypress)->mode, EventMode::Once);
}

void TestConfigBorderStyles::parsesReduceMotionAndBeatThreshold()
{
    QVERIFY(!parsed(QString()).animations.reduceMotion);
    QCOMPARE(parsed(QString()).audioBeatThreshold, 1.4);
    const Config config = parsed(QStringLiteral("animations { reduce-motion; }\naudio-beat-threshold 2.5"));
    QVERIFY(config.animations.reduceMotion);
    QCOMPARE(config.audioBeatThreshold, 2.5);
}

void TestConfigBorderStyles::warnsAboutUnknownStyles()
{
    const LoadResult result = mustLoad(QStringLiteral("layout { focus-ring { active-style \"Missing\"; }; }"));
    QVERIFY(result.warnings.contains(QStringLiteral("unknown border style: Missing")));
}

void TestConfigBorderStyles::warnsAboutUnknownStylesInPerOutputEventEffects()
{
    const LoadResult result = mustLoad(QStringLiteral(R"(
        output "eDP-1" {
            layout {
                event-effects {
                    on "click" "Ghost"
                }
            }
        }
    )"));
    QVERIFY(result.warnings.contains(QStringLiteral("unknown border style: Ghost")));
}

void TestConfigBorderStyles::rejectsBadStyleNames_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("message");

    QTest::newRow("empty") << QStringLiteral("border-style »\"\" { }") << QStringLiteral("other than `none`");
    QTest::newRow("none") << QStringLiteral("border-style »\"none\" { }") << QStringLiteral("other than `none`");
    QTest::newRow("duplicate") << QStringLiteral("border-style \"A\" { }\nborder-style »\"A\" { }")
                               << QStringLiteral("duplicate border style: A");
}

void TestConfigBorderStyles::rejectsBadStyleNames()
{
    QFETCH(QString, text);
    QFETCH(QString, message);
    verifyRejectedAtMarker(text, message);
}

void TestConfigBorderStyles::rejectsBadLayerValues_data()
{
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("message");

    QTest::newRow("unknown kind") << QStringLiteral("layer »\"nope\" { }") << QStringLiteral("expected one of");
    QTest::newRow("blend") << QStringLiteral("layer \"glow\" { blend »\"nope\"; }") << QStringLiteral("expected one of");
    QTest::newRow("direction") << QStringLiteral("layer \"particles\" { direction »\"sideways\"; }") << QStringLiteral("expected one of");
    QTest::newRow("opacity") << QStringLiteral("layer \"glow\" { opacity »2; }") << QStringLiteral("between 0 and 1");
    QTest::newRow("drive curve") << QStringLiteral("layer \"glow\" { drive \"width\" \"audio.bass\" curve=»\"cubic\"; }")
                                 << QStringLiteral("expected one of");
    QTest::newRow("fire-on value signal") << QStringLiteral("layer \"glow\" { fire-on »\"audio.level\"; }") << QStringLiteral("impulse");
}

void TestConfigBorderStyles::rejectsBadLayerValues()
{
    QFETCH(QString, body);
    QFETCH(QString, message);
    const QString text = QStringLiteral("border-style \"X\" {\n    %1\n}").arg(body);
    verifyRejectedAtMarker(text, message);
}

void TestConfigBorderStyles::rejectsBadEventEffects_data()
{
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("message");

    QTest::newRow("event") << QStringLiteral("on »\"hover\" \"none\"") << QStringLiteral("expected one of");
    QTest::newRow("reach") << QStringLiteral("on \"click\" \"none\" reach=»\"far\"") << QStringLiteral("expected one of");
    QTest::newRow("mode") << QStringLiteral("on \"click\" \"none\" mode=»\"forever\"") << QStringLiteral("expected one of");
    QTest::newRow("no style") << QStringLiteral("»on \"click\"") << QStringLiteral("border style");
}

void TestConfigBorderStyles::rejectsBadEventEffects()
{
    QFETCH(QString, body);
    QFETCH(QString, message);
    const QString text = QStringLiteral("layout {\n    event-effects {\n        %1\n    }\n}").arg(body);
    verifyRejectedAtMarker(text, message);
}

QTEST_MAIN(TestConfigBorderStyles)
#include "test_config_borderstyles.moc"
