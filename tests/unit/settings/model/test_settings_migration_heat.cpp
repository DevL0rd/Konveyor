#include "config/loader.h"
#include "document/borderdefaults.h"

#include <QTest>

using namespace Konveyor::Settings;
using namespace Konveyor::Config;

namespace
{

const QString fileName = QStringLiteral("/tmp/konveyor-migration/config.kdl");

QString heat(const QString &text)
{
    const auto result = migrateHeatLayers(text, fileName, QString());
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

QString focus(const QString &text)
{
    const auto result = migrateFocusEffects(text, fileName, QString());
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

const QString hotStyle = QStringLiteral(R"(border-style "Hot" {
    layer "heat" {
        colors "#ff0000" "#0000ff"
        baseline 0.2
        position 0.5
        drive "position" "scroll.distance" amount=1
    }
}
)");

}

class TestSettingsMigrationHeat : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void heatLayersBecomeAStillGradientMixedByTheirOldSignal();
    void heatLayersWithoutAPositionDriveFallBackToTheAudioLevel();
    void heatLayersKeepOtherDrivesAndLayers();
    void everyHeatLayerOfEveryStyleIsRewritten();
    void aConfigWithoutHeatLayersIsReturnedAsIs();
    void focusEffectsRefuseABrokenConfig();
    void focusEffectsNeedAnActiveStyle();
    void focusEffectsOnlyTouchTheActiveStyle();
};

void TestSettingsMigrationHeat::heatLayersBecomeAStillGradientMixedByTheirOldSignal()
{
    const QString result = heat(hotStyle);
    QVERIFY(!result.contains(QStringLiteral("heat")));
    QVERIFY(!result.contains(QStringLiteral("baseline")));
    QVERIFY(!result.contains(QStringLiteral("position")));
    const auto loaded = loadString(result, fileName);
    QVERIFY2(loaded.has_value(), qPrintable(result));
    const BorderLayer layer = loaded->config.borderStyles.first().layers.first();
    QCOMPARE(layer.kind, BorderLayerKind::Gradient);
    QCOMPARE(layer.speed, 0.0);
    QCOMPARE(layer.colors.size(), 2);
    QCOMPARE(layer.drives.size(), 1);
    QCOMPARE(layer.drives.first().setting, QStringLiteral("color-mix"));
    QCOMPARE(layer.drives.first().signal, Signal::ScrollDistance);
    QCOMPARE(layer.drives.first().amount, 1.0);
    QCOMPARE(layer.drives.first().smooth, 0.15);
    QCOMPARE(heat(result), result);
}

void TestSettingsMigrationHeat::heatLayersWithoutAPositionDriveFallBackToTheAudioLevel()
{
    const QString result
        = heat(QStringLiteral("border-style \"Hot\" {\n    layer \"heat\" {\n        colors \"#ff0000\" \"#0000ff\"\n    }\n}\n"));
    const auto loaded = loadString(result, fileName);
    QVERIFY2(loaded.has_value(), qPrintable(result));
    const BorderLayer layer = loaded->config.borderStyles.first().layers.first();
    QCOMPARE(layer.kind, BorderLayerKind::Gradient);
    QCOMPARE(layer.drives.size(), 1);
    QCOMPARE(layer.drives.first().signal, Signal::AudioLevel);
}

void TestSettingsMigrationHeat::heatLayersKeepOtherDrivesAndLayers()
{
    const QString text = QStringLiteral(R"(border-style "Hot" {
    layer "glow" {
        colors "#00ff00"
    }
    layer "heat" {
        colors "#ff0000" "#0000ff"
        drive "opacity" "typing.rate"
        drive "position" "pointer.speed"
    }
}
)");
    const auto loaded = loadString(heat(text), fileName);
    QVERIFY(loaded.has_value());
    const QList<BorderLayer> layers = loaded->config.borderStyles.first().layers;
    QCOMPARE(layers.size(), 2);
    QCOMPARE(layers.at(0).kind, BorderLayerKind::Glow);
    QCOMPARE(layers.at(1).kind, BorderLayerKind::Gradient);
    QCOMPARE(layers.at(1).drives.size(), 2);
    QCOMPARE(layers.at(1).drives.at(0).setting, QStringLiteral("opacity"));
    QCOMPARE(layers.at(1).drives.at(1).setting, QStringLiteral("color-mix"));
    QCOMPARE(layers.at(1).drives.at(1).signal, Signal::PointerSpeed);
}

void TestSettingsMigrationHeat::everyHeatLayerOfEveryStyleIsRewritten()
{
    const QString text = hotStyle
        + QStringLiteral("border-style \"Warm\" {\n    layer \"heat\" {\n        colors \"#ffff00\"\n    }\n"
                         "    layer \"heat\" {\n        colors \"#ff00ff\"\n    }\n}\n");
    const auto loaded = loadString(heat(text), fileName);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->config.borderStyles.size(), 2);
    QCOMPARE(loaded->config.borderStyles.at(1).layers.size(), 2);
    for (const BorderStyle &style : loaded->config.borderStyles) {
        for (const BorderLayer &layer : style.layers) {
            QCOMPARE(layer.kind, BorderLayerKind::Gradient);
            QCOMPARE(layer.drives.size(), 1);
        }
    }
}

void TestSettingsMigrationHeat::aConfigWithoutHeatLayersIsReturnedAsIs()
{
    const QString text = QStringLiteral("// mine\nborder-style \"Plain\" {\n    layer \"glow\" {\n        colors \"#00ff00\"\n    }\n}\n");
    QCOMPARE(heat(text), text);
    QCOMPARE(heat(QString()), QString());
}

void TestSettingsMigrationHeat::focusEffectsRefuseABrokenConfig()
{
    const QString result = focus(QStringLiteral("border-style \"A\" {\n"));
    QVERIFY(result.startsWith(QStringLiteral("<error: ")));
}

void TestSettingsMigrationHeat::focusEffectsNeedAnActiveStyle()
{
    const QString noRing = QStringLiteral("border-style \"A\" {\n    layer \"ripple\" {\n    }\n}\n");
    QCOMPARE(focus(noRing), noRing);
    const QString plainRing = QStringLiteral("layout {\n    focus-ring {\n        width 2\n    }\n}\n") + noRing;
    QCOMPARE(focus(plainRing), plainRing);
}

void TestSettingsMigrationHeat::focusEffectsOnlyTouchTheActiveStyle()
{
    const QString text = QStringLiteral("layout {\n    focus-ring {\n        active-style \"A\"\n    }\n}\n"
                                        "border-style \"B\" {\n    layer \"ripple\" {\n    }\n}\n"
                                        "border-style \"A\" {\n    layer \"ripple\" {\n    }\n    layer \"glow\" {\n    }\n"
                                        "    layer \"ripple\" {\n        fire-on \"click\"\n    }\n}\n");
    const auto loaded = loadString(focus(text), fileName);
    QVERIFY(loaded.has_value());
    const QList<BorderStyle> styles = loaded->config.borderStyles;
    QVERIFY(styles.at(0).layers.at(0).fireOn.isEmpty());
    QCOMPARE(styles.at(1).layers.at(0).fireOn, QList<Signal> {Signal::Focus});
    QVERIFY(styles.at(1).layers.at(1).fireOn.isEmpty());
    QCOMPARE(styles.at(1).layers.at(2).fireOn, QList<Signal> {Signal::Click});
}

QTEST_APPLESS_MAIN(TestSettingsMigrationHeat)
#include "test_settings_migration_heat.moc"
