#include "config/color.h"
#include "config/loader.h"
#include "config/sections.h"

#include <algorithm>

namespace Konveyor::Config
{

namespace
{

QStringList layerKindNames()
{
    QStringList names;
    for (const auto &[name, kind] : borderLayerKinds()) {
        names.append(name);
    }
    return names;
}

const QStringList &blendModes()
{
    static const QStringList modes {QStringLiteral("normal"), QStringLiteral("add"), QStringLiteral("screen"), QStringLiteral("multiply")};
    return modes;
}

const QStringList &particleDirections()
{
    static const QStringList directions {
        QStringLiteral("out"), QStringLiteral("up"), QStringLiteral("down"), QStringLiteral("along"), QStringLiteral("random")};
    return directions;
}

void decodeLayerColors(const Kdl::Node &node, QList<Paint> &colors)
{
    expectNoProperties(node);
    expectNoChildren(node);
    requiredArgument(node, QStringLiteral("color"));
    expectArgumentLimit(node, kBorderLayerMaxColors);
    colors.clear();
    for (const Kdl::Value &value : node.arguments) {
        colors.append(decodePaintValue(value));
    }
}

void addShapeHandlers(NodeTable &table, BorderLayer &layer)
{
    table.insert(QStringLiteral("colors"), [&layer](const Kdl::Node &node) { decodeLayerColors(node, layer.colors); });
    table.insert(QStringLiteral("speed"), [&layer](const Kdl::Node &node) { layer.speed = numberArgument(node, Range {-1000, 1000}); });
    table.insert(QStringLiteral("width"), [&layer](const Kdl::Node &node) { layer.width = numberArgument(node, Range {0, 65535}); });
    table.insert(QStringLiteral("opacity"), [&layer](const Kdl::Node &node) { layer.opacity = numberArgument(node, Range {0, 1}); });
    table.insert(QStringLiteral("blend"),
        [&layer](const Kdl::Node &node) { layer.blend = static_cast<BorderBlend>(keywordArgument(node, blendModes())); });
    table.insert(QStringLiteral("angle"), [&layer](const Kdl::Node &node) { layer.angle = numberArgument(node, Range {-360, 360}); });
    table.insert(QStringLiteral("tail"), [&layer](const Kdl::Node &node) { layer.tail = numberArgument(node, Range {0, 1}); });
    table.insert(QStringLiteral("dash"), [&layer](const Kdl::Node &node) { layer.dash = positiveNumberArgument(node, 65535); });
    table.insert(QStringLiteral("gap"), [&layer](const Kdl::Node &node) { layer.gap = numberArgument(node, Range {0, 65535}); });
}

void addParticleHandlers(NodeTable &table, BorderLayer &layer)
{
    const bool particles = layer.kind == BorderLayerKind::Particles;
    const int maxCount = particles ? kBorderLayerMaxParticles : kBorderLayerMaxCount;
    table.insert(QStringLiteral("count"),
        [&layer, maxCount](const Kdl::Node &node) { layer.count = static_cast<int>(integerArgument(node, Range {1, maxCount})); });
    table.insert(QStringLiteral("rate"), [&layer](const Kdl::Node &node) { layer.rate = positiveNumberArgument(node, 10000); });
    table.insert(QStringLiteral("size"), [&layer](const Kdl::Node &node) { layer.size = positiveNumberArgument(node, 1000); });
    table.insert(QStringLiteral("lifetime"), [&layer](const Kdl::Node &node) { layer.lifetime = positiveNumberArgument(node, 600); });
    table.insert(QStringLiteral("burst"),
        [&layer](const Kdl::Node &node) { layer.burst = static_cast<int>(integerArgument(node, Range {0, kBorderLayerMaxParticles})); });
    table.insert(QStringLiteral("direction"),
        [&layer](const Kdl::Node &node) { layer.direction = static_cast<ParticleDirection>(keywordArgument(node, particleDirections())); });
}

void addReactiveHandlers(NodeTable &table, BorderLayer &layer)
{
    table.insert(QStringLiteral("thickness"), [&layer](const Kdl::Node &node) { layer.thickness = positiveNumberArgument(node, 1000); });
    table.insert(
        QStringLiteral("sensitivity"), [&layer](const Kdl::Node &node) { layer.sensitivity = numberArgument(node, Range {0, 20}); });
    table.insert(QStringLiteral("smoothing"), [&layer](const Kdl::Node &node) { layer.smoothing = numberArgument(node, Range {0, 5}); });
    table.insert(QStringLiteral("baseline"), [&layer](const Kdl::Node &node) { layer.baseline = numberArgument(node, Range {0, 1}); });
    table.insert(QStringLiteral("position"), [&layer](const Kdl::Node &node) { layer.position = numberArgument(node, Range {0, 1}); });
    table.insert(QStringLiteral("fire-on"), [&layer](const Kdl::Node &node) { layer.fireOn = decodeFireOn(node); });
}

BorderLayer decodeBorderLayer(const Kdl::Node &node)
{
    expectNoProperties(node);
    expectArgumentLimit(node, 1);
    BorderLayer layer;
    layer.kind = borderLayerKinds().at(toKeyword(requiredArgument(node, QStringLiteral("layer type")), layerKindNames())).second;
    applyKindDefaults(layer);
    NodeTable table;
    addShapeHandlers(table, layer);
    addParticleHandlers(table, layer);
    addReactiveHandlers(table, layer);
    QList<LayerDrive> written;
    table.insert(QStringLiteral("drive"), [&written](const Kdl::Node &node) { written.append(decodeLayerDrive(node)); });
    decodeChildren(node, table, {QStringLiteral("drive")});
    for (const LayerDrive &drive : std::as_const(written)) {
        layer.drives.removeIf([&drive](const LayerDrive &other) { return other.setting == drive.setting; });
    }
    layer.drives.append(written);
    return layer;
}

void warnIfUnknown(LoadContext &context, const QString &name)
{
    if (name.isEmpty() || name == kBorderStyleNone) {
        return;
    }
    const auto &styles = context.config.borderStyles;
    if (std::ranges::none_of(styles, [&name](const BorderStyle &style) { return style.name == name; })) {
        context.warnings.append(QStringLiteral("unknown border style: ") + name);
    }
}

void warnAboutRule(LoadContext &context, const BorderRule &rule)
{
    warnIfUnknown(context, rule.activeStyle);
    warnIfUnknown(context, rule.inactiveStyle);
    warnIfUnknown(context, rule.urgentStyle);
}

void warnAboutBorder(LoadContext &context, const Border &border)
{
    warnIfUnknown(context, border.activeStyle);
    warnIfUnknown(context, border.inactiveStyle);
    warnIfUnknown(context, border.urgentStyle);
}

void warnAboutEvents(LoadContext &context, const EventEffects &effects)
{
    for (const std::optional<EventEffect> &effect : effects.slots_) {
        if (effect) {
            warnIfUnknown(context, effect->style);
        }
    }
}

void warnAboutPart(LoadContext &context, const std::optional<LayoutPart> &part)
{
    if (!part) {
        return;
    }
    if (part->eventEffects) {
        warnAboutEvents(context, *part->eventEffects);
    }
    if (part->focusRing) {
        warnAboutRule(context, *part->focusRing);
    }
    if (part->border) {
        warnAboutRule(context, *part->border);
    }
}

}

const QList<std::pair<QString, BorderLayerKind>> &borderLayerKinds()
{
    static const QList<std::pair<QString, BorderLayerKind>> kinds {
        {QStringLiteral("gradient"), BorderLayerKind::Gradient},
        {QStringLiteral("sweep"), BorderLayerKind::Sweep},
        {QStringLiteral("comet"), BorderLayerKind::Comet},
        {QStringLiteral("glow"), BorderLayerKind::Glow},
        {QStringLiteral("ripple"), BorderLayerKind::Ripple},
        {QStringLiteral("dashes"), BorderLayerKind::Dashes},
        {QStringLiteral("pulse"), BorderLayerKind::Pulse},
        {QStringLiteral("particles"), BorderLayerKind::Particles},
        {QStringLiteral("equalizer"), BorderLayerKind::Equalizer},
        {QStringLiteral("waveform"), BorderLayerKind::Waveform},
        {QStringLiteral("spectrum"), BorderLayerKind::Spectrum},
        {QStringLiteral("shimmer"), BorderLayerKind::Shimmer},
        {QStringLiteral("bass-thump"), BorderLayerKind::BassThump},
        {QStringLiteral("speed-lines"), BorderLayerKind::SpeedLines},
        {QStringLiteral("scroll-trail"), BorderLayerKind::ScrollTrail},
        {QStringLiteral("bulge"), BorderLayerKind::Bulge},
        {QStringLiteral("wavy"), BorderLayerKind::Wavy},
        {QStringLiteral("elastic"), BorderLayerKind::Elastic},
        {QStringLiteral("chevrons"), BorderLayerKind::Chevrons},
        {QStringLiteral("edge-flash"), BorderLayerKind::EdgeFlash},
        {QStringLiteral("gauge"), BorderLayerKind::Gauge},
        {QStringLiteral("pulse-front"), BorderLayerKind::PulseFront},
        {QStringLiteral("wave"), BorderLayerKind::Wave},
        {QStringLiteral("draw-in"), BorderLayerKind::DrawIn},
        {QStringLiteral("fade-out"), BorderLayerKind::FadeOut},
    };
    return kinds;
}

void decodeBorderStyle(LoadContext &context, const Kdl::Node &node)
{
    expectNoProperties(node);
    expectArgumentLimit(node, 1);
    const Kdl::Value nameValue = requiredArgument(node, QStringLiteral("name"));
    BorderStyle style;
    style.name = toText(nameValue);
    if (style.name.isEmpty() || style.name == kBorderStyleNone) {
        failAt(nameValue.location, QStringLiteral("a border style needs a name other than `none`"));
    }
    if (std::ranges::any_of(context.config.borderStyles, [&style](const BorderStyle &other) { return other.name == style.name; })) {
        failAt(nameValue.location, QStringLiteral("duplicate border style: ") + style.name);
    }
    for (const Kdl::Node &child : node.children) {
        if (child.name != QLatin1String("layer")) {
            fail(child, QStringLiteral("unexpected node ") + quoteName(child.name));
        }
        style.layers.append(decodeBorderLayer(child));
    }
    context.config.borderStyles.append(style);
}

void warnAboutUnknownBorderStyles(LoadContext &context)
{
    warnAboutBorder(context, context.config.layout.focusRing);
    warnAboutBorder(context, context.config.layout.border);
    warnAboutEvents(context, context.config.layout.eventEffects);
    for (const OutputConfig &output : std::as_const(context.config.outputs)) {
        warnAboutPart(context, output.layout);
    }
    for (const MonitorProfile &profile : std::as_const(context.config.monitorProfiles)) {
        warnAboutPart(context, profile.layout);
    }
    for (const NamedWorkspace &workspace : std::as_const(context.config.workspaces)) {
        warnAboutPart(context, workspace.layout);
    }
    for (const WindowRule &rule : std::as_const(context.config.windowRules)) {
        warnAboutRule(context, rule.focusRing);
        warnAboutRule(context, rule.border);
    }
}

}
