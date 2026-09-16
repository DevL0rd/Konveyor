#include "values/configvalues.h"

namespace Konveyor::Settings
{

namespace
{

QVariantMap edgeScrollValue(const Config::DndEdgeScroll &scroll)
{
    return {
        {QStringLiteral("trigger"), scroll.triggerSize},
        {QStringLiteral("delay-ms"), scroll.delayMs},
        {QStringLiteral("max-speed"), scroll.maxSpeed},
    };
}

QVariantMap animationValue(const Config::AnimationParams &params)
{
    static const QStringList curves {QStringLiteral("linear"), QStringLiteral("ease-out-quad"), QStringLiteral("ease-out-cubic"),
        QStringLiteral("ease-out-expo"), QStringLiteral("cubic-bezier")};
    QVariantMap value {{QStringLiteral("enabled"), params.enabled}};
    if (const auto *spring = std::get_if<Config::SpringParams>(&params.kind)) {
        value.insert(QStringLiteral("kind"), QStringLiteral("spring"));
        value.insert(QStringLiteral("damping-ratio"), spring->dampingRatio);
        value.insert(QStringLiteral("stiffness"), spring->stiffness);
        value.insert(QStringLiteral("epsilon"), spring->epsilon);
        return value;
    }
    const auto &easing = std::get<Config::EasingParams>(params.kind);
    value.insert(QStringLiteral("kind"), QStringLiteral("easing"));
    value.insert(QStringLiteral("duration-ms"), easing.durationMs);
    value.insert(QStringLiteral("curve"), curves.at(static_cast<qsizetype>(easing.curve)));
    value.insert(QStringLiteral("bezier"), QVariantList {easing.x1, easing.y1, easing.x2, easing.y2});
    return value;
}

void insertAnimations(QVariantMap &values, const Config::Animations &animations)
{
    values.insert(QStringLiteral("animations/enabled"), animations.enabled);
    values.insert(QStringLiteral("animations/slowdown"), animations.slowdown);
    const QList<std::pair<QString, const Config::AnimationParams *>> entries {
        {QStringLiteral("workspace-switch"), &animations.workspaceSwitch},
        {QStringLiteral("window-open"), &animations.windowOpen},
        {QStringLiteral("horizontal-view-movement"), &animations.horizontalViewMovement},
        {QStringLiteral("window-movement"), &animations.windowMovement},
        {QStringLiteral("window-resize"), &animations.windowResize},
    };
    for (const auto &[name, params] : entries) {
        values.insert(QStringLiteral("animations/") + name, animationValue(*params));
    }
}

}

QVariantMap globalValues(const Config::Config &config)
{
    static const QStringList warpModes {QString(), QStringLiteral("center-xy"), QStringLiteral("center-xy-always")};
    const Config::Input &input = config.input;
    const Config::Gestures &gestures = config.gestures;
    QVariantMap values {
        {QStringLiteral("hide-desktop-widgets"), config.hideDesktopWidgets},
        {QStringLiteral("fill-panels-on-maximize"), config.fillPanelsOnMaximize},
        {QStringLiteral("disable-minimize"), config.disableMinimize},
        {QStringLiteral("config-notification/disable-failed"), config.configNotificationDisableFailed},
        {QStringLiteral("input/focus-follows-mouse"), input.focusFollowsMouse},
        {QStringLiteral("input/warp-mouse-to-focus"), input.warpMouseToFocus},
        {QStringLiteral("input/warp-mouse-to-focus/mode"), warpModes.at(static_cast<qsizetype>(input.warpMouseMode))},
        {QStringLiteral("input/workspace-auto-back-and-forth"), input.workspaceAutoBackAndForth},
        {QStringLiteral("input/mod-key"), input.modKey},
        {QStringLiteral("gestures/dnd-edge-view-scroll"), edgeScrollValue(gestures.dndEdgeViewScroll)},
        {QStringLiteral("gestures/dnd-edge-workspace-switch"), edgeScrollValue(gestures.dndEdgeWorkspaceSwitch)},
        {QStringLiteral("gestures/hot-corners"),
            QVariantMap {
                {QStringLiteral("enabled"), gestures.hotCorners.enabled},
                {QStringLiteral("top-left"), gestures.hotCorners.topLeft},
                {QStringLiteral("top-right"), gestures.hotCorners.topRight},
                {QStringLiteral("bottom-left"), gestures.hotCorners.bottomLeft},
                {QStringLiteral("bottom-right"), gestures.hotCorners.bottomRight},
            }},
        {QStringLiteral("gestures/resize-tiled-windows"), gestures.resizeTiledWindows},
        {QStringLiteral("gestures/titlebar-drag"),
            gestures.titlebarDrag == Config::TitlebarDrag::MoveWindow ? QStringLiteral("move-window") : QStringLiteral("scroll-view")},
    };
    insertAnimations(values, config.animations);
    return values;
}

}
