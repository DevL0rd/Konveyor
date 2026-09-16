#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

const QStringList &curveNames()
{
    static const QStringList names {QStringLiteral("linear"), QStringLiteral("ease-out-quad"), QStringLiteral("ease-out-cubic"),
        QStringLiteral("ease-out-expo"), QStringLiteral("cubic-bezier")};
    return names;
}

struct AnimationSlot
{
    QString name;
    AnimationParams Animations::*field;
};

const QList<AnimationSlot> &animationSlots()
{
    static const QList<AnimationSlot> entries {
        {QStringLiteral("workspace-switch"), &Animations::workspaceSwitch},
        {QStringLiteral("window-open"), &Animations::windowOpen},
        {QStringLiteral("horizontal-view-movement"), &Animations::horizontalViewMovement},
        {QStringLiteral("window-movement"), &Animations::windowMovement},
        {QStringLiteral("window-resize"), &Animations::windowResize},
    };
    return entries;
}

SpringParams decodeSpring(const Kdl::Node &node)
{
    expectNoArguments(node);
    expectNoChildren(node);
    SpringParams spring;
    bool hasDamping = false;
    bool hasStiffness = false;
    bool hasEpsilon = false;
    ValueTable table;
    table.insert(QStringLiteral("damping-ratio"), [&](const Kdl::Value &value) {
        spring.dampingRatio = toNumber(value, AnyNumber);
        hasDamping = true;
    });
    table.insert(QStringLiteral("stiffness"), [&](const Kdl::Value &value) {
        spring.stiffness = static_cast<double>(toInteger(value, Range {1, 2147483647}));
        hasStiffness = true;
    });
    table.insert(QStringLiteral("epsilon"), [&](const Kdl::Value &value) {
        spring.epsilon = toNumber(value, AnyNumber);
        hasEpsilon = true;
    });
    decodeProperties(node, table);
    if (!hasDamping) {
        fail(node, QStringLiteral("property `damping-ratio` is required"));
    }
    if (!hasStiffness) {
        fail(node, QStringLiteral("property `stiffness` is required"));
    }
    if (!hasEpsilon) {
        fail(node, QStringLiteral("property `epsilon` is required"));
    }
    if (spring.dampingRatio < 0.1 || spring.dampingRatio > 10.0) {
        fail(node, QStringLiteral("damping-ratio must be between 0.1 and 10.0"));
    }
    if (spring.epsilon < 0.00001 || spring.epsilon > 0.1) {
        fail(node, QStringLiteral("epsilon must be between 0.00001 and 0.1"));
    }
    return spring;
}

void decodeBezier(const Kdl::Node &node, EasingParams &easing)
{
    if (node.arguments.size() != 5) {
        fail(node, QStringLiteral("cubic-bezier requires x1, y1, x2 and y2 control point coordinates"));
    }
    easing.x1 = toNumber(node.arguments.at(1), Range {0, 1});
    easing.y1 = toNumber(node.arguments.at(2), AnyNumber);
    easing.x2 = toNumber(node.arguments.at(3), Range {0, 1});
    easing.y2 = toNumber(node.arguments.at(4), AnyNumber);
}

void decodeCurve(const Kdl::Node &node, EasingParams &easing)
{
    expectLeafNode(node);
    easing.curve = static_cast<EasingCurve>(toKeyword(requiredArgument(node, QStringLiteral("curve")), curveNames()));
    if (easing.curve == EasingCurve::CubicBezier) {
        decodeBezier(node, easing);
        return;
    }
    expectArgumentLimit(node, 1);
}

struct AnimationDraft
{
    bool off = false;
    bool hasSpring = false;
    bool hasEasing = false;
    SpringParams spring;
    EasingParams easing {250, EasingCurve::EaseOutCubic, 0, 0, 1, 1};
};

void guardExclusive(const Kdl::Node &node, bool conflicting)
{
    if (conflicting) {
        fail(node, QStringLiteral("cannot set both spring and easing parameters at once"));
    }
}

AnimationParams finishAnimation(const AnimationDraft &draft, const AnimationParams &fallback)
{
    AnimationParams result = fallback;
    result.enabled = !draft.off;
    if (draft.hasSpring) {
        result.kind = draft.spring;
        return result;
    }
    if (draft.hasEasing) {
        result.kind = draft.easing;
    }
    return result;
}

AnimationParams decodeAnimation(const Kdl::Node &node, const AnimationParams &fallback)
{
    expectOnlyChildren(node);
    AnimationDraft draft;
    if (const auto *easing = std::get_if<EasingParams>(&fallback.kind)) {
        draft.easing = *easing;
    }
    NodeTable table;
    table.insert(QStringLiteral("off"), [&draft](const Kdl::Node &child) { draft.off = flagArgument(child); });
    table.insert(QStringLiteral("spring"), [&draft](const Kdl::Node &child) {
        guardExclusive(child, draft.hasEasing);
        draft.spring = decodeSpring(child);
        draft.hasSpring = true;
    });
    table.insert(QStringLiteral("duration-ms"), [&draft](const Kdl::Node &child) {
        guardExclusive(child, draft.hasSpring);
        draft.easing.durationMs = static_cast<double>(integerArgument(child, Range {0, 2147483647}));
        draft.hasEasing = true;
    });
    table.insert(QStringLiteral("curve"), [&draft](const Kdl::Node &child) {
        guardExclusive(child, draft.hasSpring);
        decodeCurve(child, draft.easing);
        draft.hasEasing = true;
    });
    decodeChildren(node, table);
    return finishAnimation(draft, fallback);
}

}

Animations defaultAnimations()
{
    Animations animations;
    animations.workspaceSwitch.kind = SpringParams {1.0, 1000, 0.0001};
    animations.windowOpen.kind = EasingParams {150, EasingCurve::EaseOutExpo, 0, 0, 1, 1};
    animations.horizontalViewMovement.kind = SpringParams {1.0, 800, 0.0001};
    animations.windowMovement.kind = SpringParams {1.0, 800, 0.0001};
    animations.windowResize.kind = SpringParams {1.0, 800, 0.0001};
    return animations;
}

void decodeAnimations(const Kdl::Node &node, Animations &animations)
{
    expectOnlyChildren(node);
    bool on = false;
    bool off = false;
    NodeTable table;
    table.insert(QStringLiteral("on"), [&on](const Kdl::Node &child) { on = flagArgument(child); });
    table.insert(QStringLiteral("off"), [&off](const Kdl::Node &child) { off = flagArgument(child); });
    table.insert(QStringLiteral("slowdown"),
        [&animations](const Kdl::Node &child) { animations.slowdown = numberArgument(child, Range {0, 2147483647}); });
    static const Animations defaults = defaultAnimations();
    for (const AnimationSlot &slot : animationSlots()) {
        table.insert(slot.name,
            [&animations, slot](const Kdl::Node &child) { animations.*slot.field = decodeAnimation(child, defaults.*slot.field); });
    }
    decodeChildren(node, table);
    if (off) {
        animations.enabled = false;
    }
    if (on) {
        animations.enabled = true;
    }
}

}
