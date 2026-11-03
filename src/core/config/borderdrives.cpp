#include "config/decode.h"
#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

LayerDrive defaultDrive(const char *setting, Signal signal, double amount, double smooth)
{
    return {QLatin1String(setting), signal, amount, smooth, DriveCurve::Linear};
}

double numberProperty(const Kdl::Property &property, Range range)
{
    return toNumber(property.value, range);
}

}

QStringList drivableSettings()
{
    return {QStringLiteral("speed"), QStringLiteral("width"), QStringLiteral("opacity"), QStringLiteral("angle"), QStringLiteral("count"),
        QStringLiteral("tail"), QStringLiteral("dash"), QStringLiteral("gap"), QStringLiteral("rate"), QStringLiteral("size"),
        QStringLiteral("lifetime"), QStringLiteral("burst"), QStringLiteral("thickness"), QStringLiteral("baseline"),
        QStringLiteral("position"), QStringLiteral("direction"), QStringLiteral("color-mix")};
}

QStringList driveCurveNames()
{
    return {QStringLiteral("linear"), QStringLiteral("square"), QStringLiteral("sqrt"), QStringLiteral("invert")};
}

void addToDrivenSetting(BorderLayer &layer, const QString &setting, double amount)
{
    static const QHash<QString, double BorderLayer::*> fields {
        {QStringLiteral("speed"), &BorderLayer::speed},
        {QStringLiteral("width"), &BorderLayer::width},
        {QStringLiteral("opacity"), &BorderLayer::opacity},
        {QStringLiteral("angle"), &BorderLayer::angle},
        {QStringLiteral("tail"), &BorderLayer::tail},
        {QStringLiteral("dash"), &BorderLayer::dash},
        {QStringLiteral("gap"), &BorderLayer::gap},
        {QStringLiteral("rate"), &BorderLayer::rate},
        {QStringLiteral("size"), &BorderLayer::size},
        {QStringLiteral("lifetime"), &BorderLayer::lifetime},
        {QStringLiteral("thickness"), &BorderLayer::thickness},
        {QStringLiteral("baseline"), &BorderLayer::baseline},
        {QStringLiteral("position"), &BorderLayer::position},
    };
    if (setting == QLatin1String("count")) {
        layer.count += static_cast<int>(amount);
    } else if (setting == QLatin1String("burst")) {
        layer.burst += static_cast<int>(amount);
    } else if (const auto found = fields.constFind(setting); found != fields.constEnd()) {
        layer.*(*found) += amount;
    }
}

void applyScrollKindDefaults(BorderLayer &layer)
{
    const LayerDrive motion = defaultDrive("speed", Signal::ScrollSpeed, 4.0, 0.1);
    const LayerDrive level = defaultDrive("baseline", Signal::ScrollSpeed, 1.0, 0.1);
    const LayerDrive direction = defaultDrive("direction", Signal::ScrollDirection, 1.0, 0.0);
    layer.speed = 0.0;
    layer.baseline = 0.0;
    layer.drives = {motion, level, direction};
    switch (layer.kind) {
    case BorderLayerKind::SpeedLines:
        layer.count = 10;
        layer.tail = 0.4;
        layer.speed = 0.2;
        layer.drives = {defaultDrive("speed", Signal::ScrollSpeed, 3.0, 0.1), level, direction};
        break;
    case BorderLayerKind::ScrollTrail:
        layer.tail = 0.2;
        layer.drives = {defaultDrive("position", Signal::ScrollDistance, 1.0, 0.05), level, direction};
        break;
    case BorderLayerKind::Bulge:
        layer.width = 22.0;
        layer.thickness = 3.0;
        layer.count = 3;
        layer.tail = 0.2;
        break;
    case BorderLayerKind::Wavy:
        layer.width = 12.0;
        layer.thickness = 3.0;
        layer.count = 12;
        break;
    case BorderLayerKind::Elastic:
        layer.width = 28.0;
        layer.thickness = 3.0;
        layer.drives = {defaultDrive("baseline", Signal::ScrollSpeed, 1.0, 0.03), direction};
        break;
    case BorderLayerKind::Chevrons:
        layer.width = 20.0;
        layer.thickness = 3.0;
        layer.count = 8;
        break;
    case BorderLayerKind::EdgeFlash:
        layer.width = 14.0;
        layer.tail = 0.2;
        layer.drives = {level, direction};
        break;
    case BorderLayerKind::Gauge:
        layer.width = 8.0;
        layer.tail = 0.12;
        layer.drives = {defaultDrive("position", Signal::ScrollDistance, 1.0, 0.08)};
        break;
    default:
        break;
    }
}

void applyKindDefaults(BorderLayer &layer)
{
    switch (layer.kind) {
    case BorderLayerKind::Particles:
        layer.count = 48;
        break;
    case BorderLayerKind::Equalizer:
        layer.count = 32;
        layer.gap = 2.0;
        layer.baseline = 0.3;
        break;
    case BorderLayerKind::Waveform:
        layer.baseline = 0.25;
        break;
    case BorderLayerKind::Spectrum:
        layer.baseline = 0.45;
        break;
    case BorderLayerKind::Shimmer:
        layer.baseline = 0.2;
        layer.drives = {defaultDrive("baseline", Signal::AudioTreble, 1.0, 0.08)};
        break;
    case BorderLayerKind::BassThump:
        layer.baseline = 0.35;
        layer.drives = {defaultDrive("baseline", Signal::AudioBass, 1.0, 0.05)};
        break;
    case BorderLayerKind::SpeedLines:
    case BorderLayerKind::ScrollTrail:
    case BorderLayerKind::Bulge:
    case BorderLayerKind::Wavy:
    case BorderLayerKind::Elastic:
    case BorderLayerKind::Chevrons:
    case BorderLayerKind::EdgeFlash:
    case BorderLayerKind::Gauge:
        applyScrollKindDefaults(layer);
        break;
    default:
        break;
    }
}

LayerDrive decodeLayerDrive(const Kdl::Node &node)
{
    expectNoChildren(node);
    expectArgumentLimit(node, 2);
    requiredArgument(node, QStringLiteral("setting"));
    if (node.arguments.size() < 2) {
        fail(node, QStringLiteral("drive needs a setting and a signal"));
    }
    const QStringList settings = drivableSettings();
    LayerDrive drive;
    drive.setting = settings.at(toKeyword(node.arguments.at(0), settings));
    drive.signal = static_cast<Signal>(toKeyword(node.arguments.at(1), signalNames()));
    for (const Kdl::Property &property : node.properties) {
        if (property.name == QLatin1String("amount")) {
            drive.amount = numberProperty(property, Range {-65535, 65535});
        } else if (property.name == QLatin1String("smooth")) {
            drive.smooth = numberProperty(property, Range {0, 60});
        } else if (property.name == QLatin1String("curve")) {
            drive.curve = static_cast<DriveCurve>(toKeyword(property.value, driveCurveNames()));
        } else {
            failAt(property.location, QStringLiteral("unexpected property ") + quoteName(property.name));
        }
    }
    return drive;
}

QList<Signal> decodeFireOn(const Kdl::Node &node)
{
    expectNoProperties(node);
    expectNoChildren(node);
    requiredArgument(node, QStringLiteral("signal"));
    QList<Signal> signalList;
    for (const Kdl::Value &value : node.arguments) {
        const auto signal = static_cast<Signal>(toKeyword(value, signalNames()));
        if (!isImpulse(signal)) {
            failAt(value.location, QStringLiteral("fire-on needs an impulse signal, not ") + signalName(signal));
        }
        signalList.append(signal);
    }
    return signalList;
}

}
