#include "config/loader.h"
#include "documenttesthelpers.h"
#include "values/configvalues.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

namespace
{

Config::Config load(const QString &text)
{
    const auto loaded = Config::loadString(text, QStringLiteral("config.kdl"));
    return loaded ? loaded->config : Config::Config();
}

QVariantMap layerNode(const QString &kind, const QVariantList &children = {})
{
    return block(QStringLiteral("layer"), children, {kind});
}

QVariantMap styleNode(const QVariantList &layers)
{
    return block(QStringLiteral("border-style"), layers, {QStringLiteral("Mine")});
}

}

class TestSettingsBorderStyleValues : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void exposesEveryLayerSetting();
    void exposesDrivesAndFires();
    void roundTripsALayerThroughItsValues();
    void unknownNamesFallBackToTheLayerDefaults();
    void dropsDrivesWithoutAKnownSignal();
    void parsesAStyleNodeIntoLayerValues();
    void reportsAStyleNodeThatDoesNotParse();
    void exposesEventEffects();
    void leavesUnsetEventEffectsOut();
};

void TestSettingsBorderStyleValues::exposesEveryLayerSetting()
{
    const Config::Config config = load(QStringLiteral(R"(
        border-style "A" {
            layer "particles" {
                colors "#ff0000" "#00ff00"
                blend "screen"
                direction "along"
                speed 2
                width 6
                opacity 0.5
                angle 45
                count 100
                tail 0.4
                dash 10
                gap 5
                rate 30
                size 4
                lifetime 2
                burst 12
                thickness 3
                sensitivity 2
                smoothing 0.5
                baseline 0.3
                position 0.7
            }
        }
        border-style "B" { }
    )"));
    const QVariantList styles = borderStyleValues(config.borderStyles);
    QCOMPARE(styles.size(), 2);
    QCOMPARE(styles.at(0).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("A"));
    QVERIFY(styles.at(1).toMap().value(QStringLiteral("layers")).toList().isEmpty());
    const QVariantMap layer = styles.at(0).toMap().value(QStringLiteral("layers")).toList().first().toMap();
    QCOMPARE(layer.value(QStringLiteral("kind")).toString(), QStringLiteral("particles"));
    QCOMPARE(layer.value(QStringLiteral("blend")).toString(), QStringLiteral("screen"));
    QCOMPARE(layer.value(QStringLiteral("direction")).toString(), QStringLiteral("along"));
    QCOMPARE(layer.value(QStringLiteral("colors")).toList().size(), 2);
    const QHash<QString, double> numbers {{"speed", 2}, {"width", 6}, {"opacity", 0.5}, {"angle", 45}, {"count", 100}, {"tail", 0.4},
        {"dash", 10}, {"gap", 5}, {"rate", 30}, {"size", 4}, {"lifetime", 2}, {"burst", 12}, {"thickness", 3}, {"sensitivity", 2},
        {"smoothing", 0.5}, {"baseline", 0.3}, {"position", 0.7}};
    for (auto number = numbers.constBegin(); number != numbers.constEnd(); ++number) {
        QCOMPARE(layer.value(number.key()).toDouble(), number.value());
    }
}

void TestSettingsBorderStyleValues::exposesDrivesAndFires()
{
    const Config::Config config = load(QStringLiteral(R"(
        border-style "A" {
            layer "ripple" {
                drive "width" "audio.bass" amount=2 smooth=0.4 curve="invert"
                fire-on "click" "focus"
            }
        }
    )"));
    const QVariantMap layer
        = borderStyleValues(config.borderStyles).first().toMap().value(QStringLiteral("layers")).toList().first().toMap();
    const QVariantList drives = layer.value(QStringLiteral("drives")).toList();
    QCOMPARE(drives.size(), 1);
    const QVariantMap drive = drives.first().toMap();
    QCOMPARE(drive.value(QStringLiteral("setting")).toString(), QStringLiteral("width"));
    QCOMPARE(drive.value(QStringLiteral("signal")).toString(), QStringLiteral("audio.bass"));
    QCOMPARE(drive.value(QStringLiteral("amount")).toDouble(), 2.0);
    QCOMPARE(drive.value(QStringLiteral("smooth")).toDouble(), 0.4);
    QCOMPARE(drive.value(QStringLiteral("curve")).toString(), QStringLiteral("invert"));
    QCOMPARE(layer.value(QStringLiteral("fireOn")).toStringList(), (QStringList {"click", "focus"}));
}

void TestSettingsBorderStyleValues::roundTripsALayerThroughItsValues()
{
    const Config::Config config = load(QStringLiteral(R"(
        border-style "A" {
            layer "bulge" {
                blend "multiply"
                direction "random"
                count 5
                burst 3
                drive "tail" "scroll.speed" amount=3 smooth=0.2 curve="square"
                fire-on "keypress"
            }
        }
    )"));
    const Config::BorderLayer original = config.borderStyles.first().layers.first();
    const QVariantMap values
        = borderStyleValues(config.borderStyles).first().toMap().value(QStringLiteral("layers")).toList().first().toMap();
    Config::BorderLayer restored = borderLayerWithoutColors(values);
    QVERIFY(restored.colors.isEmpty());
    restored.colors = original.colors;
    QCOMPARE(restored, original);
}

void TestSettingsBorderStyleValues::unknownNamesFallBackToTheLayerDefaults()
{
    const Config::BorderLayer layer = borderLayerWithoutColors({{"kind", "nonsense"}, {"blend", "nonsense"}, {"direction", "sideways"}});
    QCOMPARE(layer, Config::BorderLayer {});
    QCOMPARE(borderLayerWithoutColors({}), Config::BorderLayer {});
    QCOMPARE(borderLayerWithoutColors({{"kind", "wavy"}}).kind, Config::BorderLayerKind::Wavy);
}

void TestSettingsBorderStyleValues::dropsDrivesWithoutAKnownSignal()
{
    const QVariantList drives {
        QVariantMap {{"setting", "width"}, {"signal", "audio.nope"}},
        QVariantMap {{"setting", "tail"}, {"signal", "time"}, {"curve", "unknown"}},
        QVariantMap {{"setting", "gap"}},
    };
    const Config::BorderLayer layer = borderLayerWithoutColors({{"drives", drives}, {"fireOn", QStringList {"audio.nope", "urgent"}}});
    QCOMPARE(layer.drives.size(), 1);
    QCOMPARE(layer.drives.first().setting, QStringLiteral("tail"));
    QCOMPARE(layer.drives.first().signal, Config::Signal::Time);
    QCOMPARE(layer.drives.first().amount, 1.0);
    QCOMPARE(layer.drives.first().smooth, 0.0);
    QCOMPARE(layer.drives.first().curve, Config::DriveCurve::Linear);
    QCOMPARE(layer.fireOn, QList<Config::Signal> {Config::Signal::Urgent});
}

void TestSettingsBorderStyleValues::parsesAStyleNodeIntoLayerValues()
{
    const QVariantMap node
        = styleNode({layerNode(QStringLiteral("glow"), {leaf(QStringLiteral("width"), {9})}), layerNode(QStringLiteral("speed-lines"))});
    const QVariantList layers = parsedBorderStyleLayers(node);
    QCOMPARE(layers.size(), 2);
    QCOMPARE(layers.at(0).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("glow"));
    QCOMPARE(layers.at(0).toMap().value(QStringLiteral("width")).toDouble(), 9.0);
    QCOMPARE(layers.at(1).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("speed-lines"));
    QCOMPARE(layers.at(1).toMap().value(QStringLiteral("drives")).toList().size(), 3);
}

void TestSettingsBorderStyleValues::reportsAStyleNodeThatDoesNotParse()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("border style did not parse: .*")));
    QVERIFY(parsedBorderStyleLayers(styleNode({layerNode(QStringLiteral("nonsense"))})).isEmpty());
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("border style did not parse: .no style in it.")));
    QVERIFY(parsedBorderStyleLayers(leaf(QStringLiteral("layout"))).isEmpty());
}

void TestSettingsBorderStyleValues::exposesEventEffects()
{
    const Config::Config config = load(QStringLiteral(R"(
        border-style "Pulse" { layer "pulse-front" { colors "#ffffff"; }; }
        layout {
            event-effects {
                on "click" "Pulse" reach="neighbors" mode="while" duration=2.5
                on "unfocus" "none"
                on "view" "Pulse" reach="all"
            }
        }
    )"));
    const QVariantMap effects = layoutValues(config.layout).value(QStringLiteral("event-effects")).toMap();
    QCOMPARE(effects.size(), 3);
    const QVariantMap click = effects.value(QStringLiteral("click")).toMap();
    QCOMPARE(click.value(QStringLiteral("style")).toString(), QStringLiteral("Pulse"));
    QCOMPARE(click.value(QStringLiteral("reach")).toString(), QStringLiteral("neighbors"));
    QCOMPARE(click.value(QStringLiteral("mode")).toString(), QStringLiteral("while"));
    QCOMPARE(click.value(QStringLiteral("duration")).toDouble(), 2.5);
    const QVariantMap unfocus = effects.value(QStringLiteral("unfocus")).toMap();
    QCOMPARE(unfocus.value(QStringLiteral("reach")).toString(), QStringLiteral("self"));
    QCOMPARE(unfocus.value(QStringLiteral("mode")).toString(), QStringLiteral("once"));
    QCOMPARE(unfocus.value(QStringLiteral("duration")).toDouble(), 0.0);
    QCOMPARE(effects.value(QStringLiteral("view")).toMap().value(QStringLiteral("reach")).toString(), QStringLiteral("all"));
}

void TestSettingsBorderStyleValues::leavesUnsetEventEffectsOut()
{
    QVERIFY(layoutValues(Config::Config().layout).value(QStringLiteral("event-effects")).toMap().isEmpty());
}

QTEST_APPLESS_MAIN(TestSettingsBorderStyleValues)
#include "test_settings_border_style_values.moc"
