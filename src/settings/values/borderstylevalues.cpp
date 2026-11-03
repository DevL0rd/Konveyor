#include "values/configvalues.h"

#include "config/loader.h"
#include "document/nodecodec.h"

#include <QDebug>

namespace Konveyor::Settings
{

namespace
{

QString kindName(Config::BorderLayerKind kind)
{
    for (const auto &[name, value] : Config::borderLayerKinds()) {
        if (value == kind) {
            return name;
        }
    }
    return {};
}

Config::BorderLayerKind kindNamed(const QVariant &value, Config::BorderLayerKind fallback)
{
    for (const auto &[name, kind] : Config::borderLayerKinds()) {
        if (name == value.toString()) {
            return kind;
        }
    }
    return fallback;
}

const QStringList &blendNames()
{
    static const QStringList names {QStringLiteral("normal"), QStringLiteral("add"), QStringLiteral("screen"), QStringLiteral("multiply")};
    return names;
}

const QStringList &directionNames()
{
    static const QStringList names {
        QStringLiteral("out"), QStringLiteral("up"), QStringLiteral("down"), QStringLiteral("along"), QStringLiteral("random")};
    return names;
}

template<typename Enum> Enum named(const QVariant &value, const QStringList &names, Enum fallback)
{
    const qsizetype index = names.indexOf(value.toString());
    return index < 0 ? fallback : static_cast<Enum>(index);
}

QVariantList driveValues(const Config::BorderLayer &layer)
{
    QVariantList drives;
    for (const Config::LayerDrive &drive : layer.drives) {
        drives.append(QVariantMap {{QStringLiteral("setting"), drive.setting}, {QStringLiteral("signal"), Config::signalName(drive.signal)},
            {QStringLiteral("amount"), drive.amount}, {QStringLiteral("smooth"), drive.smooth},
            {QStringLiteral("curve"), Config::driveCurveNames().at(static_cast<qsizetype>(drive.curve))}});
    }
    return drives;
}

QVariantList fireValues(const Config::BorderLayer &layer)
{
    QVariantList names;
    for (const Config::Signal signal : layer.fireOn) {
        names.append(Config::signalName(signal));
    }
    return names;
}

QList<Config::LayerDrive> drivesFrom(const QVariantList &values)
{
    QList<Config::LayerDrive> drives;
    for (const QVariant &value : values) {
        const QVariantMap map = value.toMap();
        const auto signal = Config::signalNamed(map.value(QStringLiteral("signal")).toString());
        if (signal) {
            drives.append({map.value(QStringLiteral("setting")).toString(), *signal, map.value(QStringLiteral("amount"), 1.0).toDouble(),
                map.value(QStringLiteral("smooth")).toDouble(),
                named(map.value(QStringLiteral("curve")), Config::driveCurveNames(), Config::DriveCurve::Linear)});
        }
    }
    return drives;
}

QList<Config::Signal> firesFrom(const QVariantList &values)
{
    QList<Config::Signal> fires;
    for (const QVariant &value : values) {
        if (const auto signal = Config::signalNamed(value.toString())) {
            fires.append(*signal);
        }
    }
    return fires;
}

QVariantMap layerValue(const Config::BorderLayer &layer)
{
    QVariantList colors;
    for (const Config::Paint &paint : layer.colors) {
        colors.append(paintValue(paint));
    }
    return {
        {QStringLiteral("kind"), kindName(layer.kind)},
        {QStringLiteral("colors"), colors},
        {QStringLiteral("speed"), layer.speed},
        {QStringLiteral("width"), layer.width},
        {QStringLiteral("opacity"), layer.opacity},
        {QStringLiteral("blend"), blendNames().at(static_cast<qsizetype>(layer.blend))},
        {QStringLiteral("angle"), layer.angle},
        {QStringLiteral("count"), layer.count},
        {QStringLiteral("tail"), layer.tail},
        {QStringLiteral("dash"), layer.dash},
        {QStringLiteral("gap"), layer.gap},
        {QStringLiteral("rate"), layer.rate},
        {QStringLiteral("size"), layer.size},
        {QStringLiteral("lifetime"), layer.lifetime},
        {QStringLiteral("burst"), layer.burst},
        {QStringLiteral("thickness"), layer.thickness},
        {QStringLiteral("sensitivity"), layer.sensitivity},
        {QStringLiteral("smoothing"), layer.smoothing},
        {QStringLiteral("baseline"), layer.baseline},
        {QStringLiteral("position"), layer.position},
        {QStringLiteral("drives"), driveValues(layer)},
        {QStringLiteral("fireOn"), fireValues(layer)},
        {QStringLiteral("direction"), directionNames().at(static_cast<qsizetype>(layer.direction))},
    };
}

}

Config::BorderLayer borderLayerWithoutColors(const QVariantMap &value)
{
    Config::BorderLayer layer;
    const auto number = [&](const char *key, double fallback) { return value.value(QLatin1String(key), fallback).toDouble(); };
    layer.kind = kindNamed(value.value(QStringLiteral("kind")), layer.kind);
    layer.blend = named(value.value(QStringLiteral("blend")), blendNames(), layer.blend);
    layer.direction = named(value.value(QStringLiteral("direction")), directionNames(), layer.direction);
    layer.speed = number("speed", layer.speed);
    layer.width = number("width", layer.width);
    layer.opacity = number("opacity", layer.opacity);
    layer.angle = number("angle", layer.angle);
    layer.count = static_cast<int>(number("count", layer.count));
    layer.tail = number("tail", layer.tail);
    layer.dash = number("dash", layer.dash);
    layer.gap = number("gap", layer.gap);
    layer.rate = number("rate", layer.rate);
    layer.size = number("size", layer.size);
    layer.lifetime = number("lifetime", layer.lifetime);
    layer.burst = static_cast<int>(number("burst", layer.burst));
    layer.thickness = number("thickness", layer.thickness);
    layer.sensitivity = number("sensitivity", layer.sensitivity);
    layer.smoothing = number("smoothing", layer.smoothing);
    layer.baseline = number("baseline", layer.baseline);
    layer.position = number("position", layer.position);
    layer.drives = drivesFrom(value.value(QStringLiteral("drives")).toList());
    layer.fireOn = firesFrom(value.value(QStringLiteral("fireOn")).toList());
    return layer;
}

QVariantList borderStyleValues(const QList<Config::BorderStyle> &styles)
{
    QVariantList values;
    for (const Config::BorderStyle &style : styles) {
        QVariantList layers;
        for (const Config::BorderLayer &layer : style.layers) {
            layers.append(layerValue(layer));
        }
        values.append(QVariantMap {{QStringLiteral("name"), style.name}, {QStringLiteral("layers"), layers}});
    }
    return values;
}

QVariantList parsedBorderStyleLayers(const QVariantMap &styleNode)
{
    const auto loaded = Config::loadString(writeNode(styleNode, QString()), QStringLiteral("border-style.kdl"));
    if (!loaded || loaded->config.borderStyles.isEmpty()) {
        qWarning() << "Konveyor: border style did not parse:" << (loaded ? QStringLiteral("no style in it") : loaded.error().message);
        return {};
    }
    return borderStyleValues(loaded->config.borderStyles).constFirst().toMap().value(QStringLiteral("layers")).toList();
}

}
