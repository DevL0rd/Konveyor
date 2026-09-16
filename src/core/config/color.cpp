#include "config/color.h"

#include "config/decode.h"

#include <QStringList>

#include <algorithm>
#include <cmath>

namespace Konveyor::Config
{

namespace
{

const QStringList &gradientSpaces()
{
    static const QStringList spaces {
        QStringLiteral("srgb"), QStringLiteral("srgb-linear"), QStringLiteral("oklab"), QStringLiteral("oklch")};
    return spaces;
}

const QStringList &hueModes()
{
    static const QStringList modes {
        QStringLiteral("shorter"), QStringLiteral("longer"), QStringLiteral("increasing"), QStringLiteral("decreasing")};
    return modes;
}

int hexDigits(QStringView text, int start, int count)
{
    bool ok = false;
    const int value = text.mid(start, count).toInt(&ok, 16);
    return ok ? value : -1;
}

std::optional<QColor> parseHexColor(QStringView text)
{
    const int size = static_cast<int>(text.size());
    const bool shortForm = size == 3 || size == 4;
    if (!shortForm && size != 6 && size != 8) {
        return std::nullopt;
    }
    const int step = shortForm ? 1 : 2;
    const int count = size / step;
    int channels[4] = {0, 0, 0, 255};
    for (int index = 0; index < count; ++index) {
        const int digit = hexDigits(text, index * step, step);
        if (digit < 0) {
            return std::nullopt;
        }
        channels[index] = shortForm ? digit * 17 : digit;
    }
    return QColor(channels[0], channels[1], channels[2], channels[3]);
}

double clampUnit(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

std::optional<double> parseScaled(const QString &token, double fullScale)
{
    bool ok = false;
    if (token.endsWith(QLatin1Char('%'))) {
        const double value = QStringView {token}.chopped(1).toDouble(&ok);
        return ok ? std::optional {value / 100.0} : std::nullopt;
    }
    const double value = token.toDouble(&ok);
    return ok ? std::optional {value / fullScale} : std::nullopt;
}

std::optional<double> parseHue(const QString &token)
{
    static const QList<std::pair<QString, double>> units {{QStringLiteral("deg"), 1.0}, {QStringLiteral("grad"), 0.9},
        {QStringLiteral("rad"), 180.0 / M_PI}, {QStringLiteral("turn"), 360.0}};
    for (const auto &[suffix, factor] : units) {
        if (token.endsWith(suffix)) {
            bool ok = false;
            const double value = QStringView {token}.chopped(suffix.size()).toDouble(&ok);
            return ok ? std::optional {value * factor} : std::nullopt;
        }
    }
    bool ok = false;
    const double value = token.toDouble(&ok);
    return ok ? std::optional {value} : std::nullopt;
}

QStringList functionTokens(const QString &body)
{
    QString normalized = body;
    normalized.replace(QLatin1Char(','), QLatin1Char(' '));
    normalized.replace(QLatin1Char('/'), QLatin1Char(' '));
    return normalized.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

std::optional<QColor> buildRgb(const QStringList &tokens, double alpha)
{
    double channels[3] = {0, 0, 0};
    for (int index = 0; index < 3; ++index) {
        const std::optional<double> value = parseScaled(tokens.at(index), 255.0);
        if (!value) {
            return std::nullopt;
        }
        channels[index] = clampUnit(*value);
    }
    return QColor::fromRgbF(channels[0], channels[1], channels[2], alpha);
}

std::optional<QColor> buildHsl(const QStringList &tokens, double alpha)
{
    const std::optional<double> hue = parseHue(tokens.at(0));
    const std::optional<double> saturation = parseScaled(tokens.at(1), 1.0);
    const std::optional<double> lightness = parseScaled(tokens.at(2), 1.0);
    if (!hue || !saturation || !lightness) {
        return std::nullopt;
    }
    double wrapped = std::fmod(*hue, 360.0);
    if (wrapped < 0) {
        wrapped += 360.0;
    }
    return QColor::fromHslF(wrapped / 360.0, clampUnit(*saturation), clampUnit(*lightness), alpha);
}

std::optional<QColor> parseFunctionColor(const QString &text)
{
    const qsizetype open = text.indexOf(QLatin1Char('('));
    if (open < 0 || !text.endsWith(QLatin1Char(')'))) {
        return std::nullopt;
    }
    const QString name = text.left(open).trimmed();
    const QStringList tokens = functionTokens(text.mid(open + 1, text.size() - open - 2));
    if (tokens.size() < 3 || tokens.size() > 4) {
        return std::nullopt;
    }
    double alpha = 1.0;
    if (tokens.size() == 4) {
        const std::optional<double> parsed = parseScaled(tokens.at(3), 1.0);
        if (!parsed) {
            return std::nullopt;
        }
        alpha = clampUnit(*parsed);
    }
    if (name == QLatin1String("rgb") || name == QLatin1String("rgba")) {
        return buildRgb(tokens, alpha);
    }
    if (name == QLatin1String("hsl") || name == QLatin1String("hsla")) {
        return buildHsl(tokens, alpha);
    }
    return std::nullopt;
}

const QList<std::pair<QLatin1String, ColorSource>> &themeColorKeywords()
{
    static const QList<std::pair<QLatin1String, ColorSource>> keywords {
        {QLatin1String("accent"), ColorSource::SystemAccent},
        {QLatin1String("focus"), ColorSource::SystemFocus},
        {QLatin1String("hover"), ColorSource::SystemHover},
        {QLatin1String("window"), ColorSource::SystemWindow},
        {QLatin1String("window-text"), ColorSource::SystemWindowText},
        {QLatin1String("inactive-text"), ColorSource::SystemInactiveText},
    };
    return keywords;
}

Paint paintFromText(const Kdl::Value &value)
{
    const QString text = toText(value).trimmed();
    for (const auto &[keyword, source] : themeColorKeywords()) {
        if (text.compare(keyword, Qt::CaseInsensitive) == 0) {
            return Paint {source, QColor(), std::nullopt};
        }
    }
    const std::optional<QColor> color = parseCssColor(text);
    if (!color) {
        failAt(value.location, QStringLiteral("invalid color: ") + text);
    }
    return Paint {ColorSource::Explicit, *color, std::nullopt};
}

QColor colorFromChannels(const Kdl::Node &node)
{
    if (node.arguments.size() != 4) {
        fail(node, QStringLiteral("expected a color string or 4 color channels"));
    }
    int channels[4] = {0, 0, 0, 0};
    for (int index = 0; index < 4; ++index) {
        channels[index] = static_cast<int>(toInteger(node.arguments.at(index), Range {0, 255}));
    }
    return QColor(channels[0], channels[1], channels[2], channels[3]);
}

GradientInterpolation gradientSpace(const Kdl::Value &value, HueInterpolation &hue)
{
    const QStringList parts = toText(value).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        failAt(value.location, QStringLiteral("missing color space"));
    }
    const qsizetype space = gradientSpaces().indexOf(parts.first());
    if (space < 0) {
        failAt(value.location,
            QStringLiteral("invalid color space ") + parts.first() + QStringLiteral("; can be srgb, srgb-linear, oklab or oklch"));
    }
    if (parts.size() == 1) {
        return static_cast<GradientInterpolation>(space);
    }
    if (space != gradientSpaces().indexOf(QStringLiteral("oklch"))) {
        failAt(value.location, QStringLiteral("only oklch color space can have hue interpolation"));
    }
    if (parts.size() != 3 || parts.at(2) != QLatin1String("hue")) {
        failAt(value.location, QStringLiteral("interpolation must end with \"hue\", like \"oklch shorter hue\""));
    }
    const qsizetype mode = hueModes().indexOf(parts.at(1));
    if (mode < 0) {
        failAt(value.location,
            QStringLiteral("invalid hue interpolation ") + parts.at(1)
                + QStringLiteral("; can be shorter, longer, increasing, decreasing"));
    }
    hue = static_cast<HueInterpolation>(mode);
    return static_cast<GradientInterpolation>(space);
}

}

std::optional<QColor> parseCssColor(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return std::nullopt;
    }
    if (trimmed.startsWith(QLatin1Char('#'))) {
        return parseHexColor(QStringView {trimmed}.mid(1));
    }
    if (trimmed.contains(QLatin1Char('('))) {
        return parseFunctionColor(trimmed.toLower());
    }
    const QColor named = QColor::fromString(trimmed.toLower());
    return named.isValid() ? std::optional {named} : std::nullopt;
}

QColor decodeColorNode(const Kdl::Node &node)
{
    const Paint paint = decodePaintNode(node);
    if (paint.source != ColorSource::Explicit) {
        fail(node, QStringLiteral("`accent` is only supported for focus ring, border, tab indicator and insert hint colors"));
    }
    return paint.color;
}

Paint decodePaintNode(const Kdl::Node &node)
{
    expectLeafNode(node);
    const Kdl::Value first = requiredArgument(node, node.name);
    if (!first.isString()) {
        return Paint {ColorSource::Explicit, colorFromChannels(node), std::nullopt};
    }
    expectArgumentLimit(node, 1);
    return paintFromText(first);
}

Gradient decodeGradientNode(const Kdl::Node &node)
{
    expectNoArguments(node);
    expectNoChildren(node);
    Gradient gradient;
    bool hasFrom = false;
    bool hasTo = false;
    ValueTable table;
    table.insert(QStringLiteral("from"), [&](const Kdl::Value &value) {
        gradient.from = paintFromText(value).color;
        hasFrom = true;
    });
    table.insert(QStringLiteral("to"), [&](const Kdl::Value &value) {
        gradient.to = paintFromText(value).color;
        hasTo = true;
    });
    table.insert(QStringLiteral("angle"), [&](const Kdl::Value &value) { gradient.angle = toNumber(value, Range {-32768, 32767}); });
    table.insert(QStringLiteral("relative-to"), [&](const Kdl::Value &value) {
        gradient.relativeTo
            = static_cast<GradientRelativeTo>(toKeyword(value, {QStringLiteral("window"), QStringLiteral("workspace-view")}));
    });
    table.insert(QStringLiteral("in"), [&](const Kdl::Value &value) { gradient.interpolation = gradientSpace(value, gradient.hue); });
    decodeProperties(node, table);
    if (!hasFrom) {
        fail(node, QStringLiteral("property `from` is required"));
    }
    if (!hasTo) {
        fail(node, QStringLiteral("property `to` is required"));
    }
    return gradient;
}

void applyPaintColor(std::optional<Paint> &paint, const Paint &value)
{
    paint = value;
}

void applyPaintGradient(std::optional<Paint> &paint, const Gradient &gradient)
{
    if (!paint) {
        paint = Paint {};
    }
    paint->gradient = gradient;
}

void mergePaint(Paint &base, const std::optional<Paint> &part)
{
    if (!part) {
        return;
    }
    if (!part->gradient) {
        base = *part;
        return;
    }
    base.gradient = part->gradient;
    if (part->source != ColorSource::Explicit || part->color.isValid()) {
        base.source = part->source;
        base.color = part->color;
    }
}

void mergeOptionalPaint(std::optional<Paint> &base, const std::optional<Paint> &part)
{
    if (!part) {
        return;
    }
    if (!base) {
        base = *part;
        return;
    }
    mergePaint(*base, part);
}

}
