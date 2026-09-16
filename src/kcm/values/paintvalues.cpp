#include "values/configvalues.h"

namespace Konveyor::Settings
{

namespace
{

QString sourceName(Config::ColorSource source)
{
    static const QHash<Config::ColorSource, QString> names {
        {Config::ColorSource::Explicit, QStringLiteral("color")},
        {Config::ColorSource::SystemAccent, QStringLiteral("accent")},
        {Config::ColorSource::SystemFocus, QStringLiteral("focus")},
        {Config::ColorSource::SystemHover, QStringLiteral("hover")},
        {Config::ColorSource::SystemWindow, QStringLiteral("window")},
        {Config::ColorSource::SystemWindowText, QStringLiteral("window-text")},
        {Config::ColorSource::SystemInactiveText, QStringLiteral("inactive-text")},
    };
    return names.value(source);
}

QString interpolationName(const Config::Gradient &gradient)
{
    static const QStringList spaces {
        QStringLiteral("srgb"), QStringLiteral("srgb-linear"), QStringLiteral("oklab"), QStringLiteral("oklch")};
    static const QStringList hues {
        QStringLiteral("shorter"), QStringLiteral("longer"), QStringLiteral("increasing"), QStringLiteral("decreasing")};
    const QString &space = spaces.at(static_cast<qsizetype>(gradient.interpolation));
    if (gradient.interpolation != Config::GradientInterpolation::Oklch) {
        return space;
    }
    return space + QLatin1Char(' ') + hues.at(static_cast<qsizetype>(gradient.hue)) + QStringLiteral(" hue");
}

}

QVariant colorValue(const QColor &color)
{
    return color.isValid() ? QVariant(color.name(QColor::HexArgb)) : QVariant();
}

QVariant paintValue(const std::optional<Config::Paint> &paint)
{
    if (!paint) {
        return QVariant();
    }
    QVariantMap value {
        {QStringLiteral("source"), sourceName(paint->source)},
        {QStringLiteral("color"), colorValue(paint->color)},
    };
    if (paint->gradient) {
        const Config::Gradient &gradient = *paint->gradient;
        value.insert(QStringLiteral("gradient"),
            QVariantMap {
                {QStringLiteral("from"), colorValue(gradient.from)},
                {QStringLiteral("to"), colorValue(gradient.to)},
                {QStringLiteral("angle"), gradient.angle},
                {QStringLiteral("relative-to"),
                    gradient.relativeTo == Config::GradientRelativeTo::Window ? QStringLiteral("window")
                                                                              : QStringLiteral("workspace-view")},
                {QStringLiteral("in"), interpolationName(gradient)},
            });
    }
    return value;
}

QVariant sizeValue(const std::optional<Config::PresetSize> &size)
{
    if (!size) {
        return QVariant();
    }
    if (const auto *proportion = std::get_if<Config::Proportion>(&*size)) {
        return QVariantMap {{QStringLiteral("kind"), QStringLiteral("proportion")}, {QStringLiteral("value"), proportion->value}};
    }
    return QVariantMap {{QStringLiteral("kind"), QStringLiteral("fixed")}, {QStringLiteral("value"), std::get<Config::Fixed>(*size).value}};
}

}
