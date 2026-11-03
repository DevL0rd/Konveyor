#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

BorderLayer layerOf(const QString &kind)
{
    const Config config = parsed(QStringLiteral("border-style \"X\" { layer \"%1\" { }; }").arg(kind));
    return config.borderStyles.first().layers.first();
}

LayerDrive driveFor(const BorderLayer &layer, const QString &setting)
{
    for (const LayerDrive &drive : layer.drives) {
        if (drive.setting == setting) {
            return drive;
        }
    }
    return LayerDrive {QStringLiteral("<missing>"), Signal::Count, 0.0, 0.0, DriveCurve::Linear};
}

}

class TestConfigBorderDrives : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scrollKindsStartFromTheirOwnShape_data();
    void scrollKindsStartFromTheirOwnShape();
    void audioKindsCarryTheirOwnDrives();
    void plainKindsStayUndriven();
    void aWrittenDriveReplacesTheKindDefaultOfTheSameSetting();
    void addsToEveryDrivableSetting_data();
    void addsToEveryDrivableSetting();
    void unknownSettingsAddNothing();
    void everyDrivableSettingIsAccepted();
    void rejectsBadDrives_data();
    void rejectsBadDrives();
    void rejectsBadFireOn_data();
    void rejectsBadFireOn();
    void listsCurvesAndSettings();
};

void TestConfigBorderDrives::scrollKindsStartFromTheirOwnShape_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<int>("count");
    QTest::addColumn<double>("width");
    QTest::addColumn<int>("drives");

    QTest::newRow("speed-lines") << "speed-lines" << 10 << 0.0 << 3;
    QTest::newRow("scroll-trail") << "scroll-trail" << 1 << 0.0 << 3;
    QTest::newRow("bulge") << "bulge" << 3 << 22.0 << 3;
    QTest::newRow("wavy") << "wavy" << 12 << 12.0 << 3;
    QTest::newRow("elastic") << "elastic" << 1 << 28.0 << 2;
    QTest::newRow("chevrons") << "chevrons" << 8 << 20.0 << 3;
    QTest::newRow("edge-flash") << "edge-flash" << 1 << 14.0 << 2;
    QTest::newRow("gauge") << "gauge" << 1 << 8.0 << 1;
}

void TestConfigBorderDrives::scrollKindsStartFromTheirOwnShape()
{
    QFETCH(QString, kind);
    QFETCH(int, count);
    QFETCH(double, width);
    QFETCH(int, drives);
    const BorderLayer layer = layerOf(kind);
    QCOMPARE(layer.count, count);
    QCOMPARE(layer.width, width);
    QCOMPARE(layer.drives.size(), drives);
    QCOMPARE(layer.speed, kind == QLatin1String("speed-lines") ? 0.2 : 0.0);
}

void TestConfigBorderDrives::audioKindsCarryTheirOwnDrives()
{
    const BorderLayer lines = layerOf(QStringLiteral("speed-lines"));
    QCOMPARE(lines.tail, 0.4);
    QCOMPARE(driveFor(lines, QStringLiteral("speed")).amount, 3.0);
    QCOMPARE(driveFor(lines, QStringLiteral("speed")).signal, Signal::ScrollSpeed);
    QCOMPARE(driveFor(lines, QStringLiteral("direction")).signal, Signal::ScrollDirection);

    const BorderLayer trail = layerOf(QStringLiteral("scroll-trail"));
    QCOMPARE(driveFor(trail, QStringLiteral("position")).signal, Signal::ScrollDistance);
    QCOMPARE(driveFor(trail, QStringLiteral("position")).smooth, 0.05);

    const BorderLayer gauge = layerOf(QStringLiteral("gauge"));
    QCOMPARE(gauge.tail, 0.12);
    QCOMPARE(driveFor(gauge, QStringLiteral("position")).smooth, 0.08);

    const BorderLayer elastic = layerOf(QStringLiteral("elastic"));
    QCOMPARE(driveFor(elastic, QStringLiteral("baseline")).smooth, 0.03);
    QCOMPARE(driveFor(elastic, QStringLiteral("speed")).setting, QStringLiteral("<missing>"));

    const BorderLayer shimmer = layerOf(QStringLiteral("shimmer"));
    QCOMPARE(shimmer.baseline, 0.2);
    QCOMPARE(driveFor(shimmer, QStringLiteral("baseline")).signal, Signal::AudioTreble);

    const BorderLayer thump = layerOf(QStringLiteral("bass-thump"));
    QCOMPARE(thump.baseline, 0.35);
    QCOMPARE(driveFor(thump, QStringLiteral("baseline")).signal, Signal::AudioBass);

    const BorderLayer equalizer = layerOf(QStringLiteral("equalizer"));
    QCOMPARE(equalizer.count, 32);
    QCOMPARE(equalizer.gap, 2.0);
    QCOMPARE(equalizer.baseline, 0.3);
    QCOMPARE(layerOf(QStringLiteral("waveform")).baseline, 0.25);
    QCOMPARE(layerOf(QStringLiteral("spectrum")).baseline, 0.45);
    QCOMPARE(layerOf(QStringLiteral("particles")).count, 48);
}

void TestConfigBorderDrives::plainKindsStayUndriven()
{
    for (const QString &kind : {QStringLiteral("gradient"), QStringLiteral("glow"), QStringLiteral("ripple"), QStringLiteral("wave")}) {
        const BorderLayer layer = layerOf(kind);
        QVERIFY2(layer.drives.isEmpty(), qPrintable(kind));
        QCOMPARE(layer.count, 1);
        QCOMPARE(layer.baseline, BorderLayer {}.baseline);
    }
}

void TestConfigBorderDrives::aWrittenDriveReplacesTheKindDefaultOfTheSameSetting()
{
    const Config config = parsed(QStringLiteral(R"(
        border-style "X" {
            layer "speed-lines" {
                drive "speed" "pointer.speed" amount=2 smooth=0.5 curve="sqrt"
                drive "opacity" "typing.rate"
            }
        }
    )"));
    const BorderLayer layer = config.borderStyles.first().layers.first();
    QCOMPARE(layer.drives.size(), 4);
    int speeds = 0;
    for (const LayerDrive &drive : layer.drives) {
        speeds += drive.setting == QLatin1String("speed");
    }
    QCOMPARE(speeds, 1);
    const LayerDrive speed = driveFor(layer, QStringLiteral("speed"));
    QCOMPARE(speed.signal, Signal::PointerSpeed);
    QCOMPARE(speed.amount, 2.0);
    QCOMPARE(speed.smooth, 0.5);
    QCOMPARE(speed.curve, DriveCurve::Sqrt);
    QCOMPARE(layer.drives.last().setting, QStringLiteral("opacity"));
    QCOMPARE(layer.drives.last().curve, DriveCurve::Linear);
}

void TestConfigBorderDrives::addsToEveryDrivableSetting_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<double>("expected");

    const BorderLayer base;
    QTest::newRow("speed") << "speed" << base.speed + 2;
    QTest::newRow("width") << "width" << base.width + 2;
    QTest::newRow("opacity") << "opacity" << base.opacity + 2;
    QTest::newRow("angle") << "angle" << base.angle + 2;
    QTest::newRow("tail") << "tail" << base.tail + 2;
    QTest::newRow("dash") << "dash" << base.dash + 2;
    QTest::newRow("gap") << "gap" << base.gap + 2;
    QTest::newRow("rate") << "rate" << base.rate + 2;
    QTest::newRow("size") << "size" << base.size + 2;
    QTest::newRow("lifetime") << "lifetime" << base.lifetime + 2;
    QTest::newRow("thickness") << "thickness" << base.thickness + 2;
    QTest::newRow("baseline") << "baseline" << base.baseline + 2;
    QTest::newRow("position") << "position" << base.position + 2;
    QTest::newRow("count") << "count" << double(base.count + 2);
    QTest::newRow("burst") << "burst" << double(base.burst + 2);
}

void TestConfigBorderDrives::addsToEveryDrivableSetting()
{
    QFETCH(QString, setting);
    QFETCH(double, expected);
    BorderLayer layer;
    addToDrivenSetting(layer, setting, 2.0);
    const QHash<QString, double> now {{"speed", layer.speed}, {"width", layer.width}, {"opacity", layer.opacity}, {"angle", layer.angle},
        {"tail", layer.tail}, {"dash", layer.dash}, {"gap", layer.gap}, {"rate", layer.rate}, {"size", layer.size},
        {"lifetime", layer.lifetime}, {"thickness", layer.thickness}, {"baseline", layer.baseline}, {"position", layer.position},
        {"count", layer.count}, {"burst", layer.burst}};
    QCOMPARE(now.value(setting), expected);
    BorderLayer untouched;
    addToDrivenSetting(untouched, setting, 0.0);
    QCOMPARE(untouched, BorderLayer {});
}

void TestConfigBorderDrives::unknownSettingsAddNothing()
{
    BorderLayer layer;
    addToDrivenSetting(layer, QStringLiteral("direction"), 3.0);
    addToDrivenSetting(layer, QStringLiteral("color-mix"), 3.0);
    addToDrivenSetting(layer, QStringLiteral("nonsense"), 3.0);
    QCOMPARE(layer, BorderLayer {});
}

void TestConfigBorderDrives::everyDrivableSettingIsAccepted()
{
    for (const QString &setting : drivableSettings()) {
        const Config config
            = parsed(QStringLiteral("border-style \"X\" { layer \"glow\" { drive \"%1\" \"audio.level\"; }; }").arg(setting));
        QCOMPARE(config.borderStyles.first().layers.first().drives.first().setting, setting);
    }
}

void TestConfigBorderDrives::rejectsBadDrives_data()
{
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("message");

    QTest::newRow("no arguments") << "»drive" << "setting";
    QTest::newRow("no signal") << "»drive \"width\"" << "drive needs a setting and a signal";
    QTest::newRow("unknown setting") << "drive »\"colour\" \"audio.level\"" << "expected one of";
    QTest::newRow("unknown signal") << "drive \"width\" »\"audio.nope\"" << "expected one of";
    QTest::newRow("unknown property") << "drive \"width\" \"audio.level\" »bounce=2" << "unexpected property";
    QTest::newRow("amount range") << "drive \"width\" \"audio.level\" amount=»70000" << "between";
    QTest::newRow("smooth range") << "drive \"width\" \"audio.level\" smooth=»-1" << "between";
    QTest::newRow("three arguments") << "drive \"width\" \"audio.level\" »\"extra\"" << "unexpected argument";
    QTest::newRow("children") << "drive \"width\" \"audio.level\" {\n »x\n }" << "unexpected node";
}

void TestConfigBorderDrives::rejectsBadDrives()
{
    QFETCH(QString, body);
    QFETCH(QString, message);
    const QString text = QStringLiteral("border-style \"X\" {\n    layer \"glow\" {\n        %1\n    }\n}").arg(body);
    const LoadError error = mustFail(unmarked(text));
    QVERIFY2(error.message.contains(message, Qt::CaseInsensitive), qPrintable(error.message));
    QVERIFY(error.location.line > 0);
}

void TestConfigBorderDrives::rejectsBadFireOn_data()
{
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("message");

    QTest::newRow("no signal") << "fire-on" << "signal";
    QTest::newRow("property") << "fire-on \"click\" speed=2" << "unexpected property";
    QTest::newRow("children") << "fire-on \"click\" { x; }" << "unexpected node";
    QTest::newRow("level signal") << "fire-on \"click\" \"time\"" << "impulse signal, not time";
    QTest::newRow("unknown signal") << "fire-on \"tap\"" << "expected one of";
}

void TestConfigBorderDrives::rejectsBadFireOn()
{
    QFETCH(QString, body);
    QFETCH(QString, message);
    const LoadError error = mustFail(QStringLiteral("border-style \"X\" {\n    layer \"ripple\" {\n        %1\n    }\n}").arg(body));
    QVERIFY2(error.message.contains(message, Qt::CaseInsensitive), qPrintable(error.message));
}

void TestConfigBorderDrives::listsCurvesAndSettings()
{
    QCOMPARE(driveCurveNames(), (QStringList {"linear", "square", "sqrt", "invert"}));
    QVERIFY(drivableSettings().contains(QStringLiteral("color-mix")));
    QVERIFY(drivableSettings().contains(QStringLiteral("direction")));
    QCOMPARE(drivableSettings().size(), 17);
}

QTEST_MAIN(TestConfigBorderDrives)
#include "test_config_borderdrives.moc"
