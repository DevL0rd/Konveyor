#include "plugin/konveyoreffect_p.h"

#include <screenedge.h>

namespace Konveyor
{

namespace
{

std::optional<Layout::EdgeHoverTarget> edgeTargetAt(
    const QPointF &position, const QList<Layout::OutputInfo> &outputs, const Config::EdgeSwitchColumns &options)
{
    const QString name = outputNameAt(position);
    const auto output = std::ranges::find(outputs, name, &Layout::OutputInfo::name);
    if (output == outputs.end()) {
        return std::nullopt;
    }
    QList<QRectF> geometries;
    for (const Layout::OutputInfo &each : outputs) {
        geometries.append(each.geometry);
    }
    const std::optional<Layout::HoverSide> side = Layout::hoverSideAt(
        position, {output->geometry, output->workArea}, geometries, options.monitorEdges, KWin::workspace()->screenEdges()->cornerOffset());
    return side ? std::optional(Layout::EdgeHoverTarget {name, *side}) : std::nullopt;
}

}

bool KonveyorEffect::edgeSwitchBlocked() const
{
    const KWin::Window *active = KWin::workspace()->activeWindow();
    return !d->config.config().gestures.edgeSwitchColumns.enabled || d->titlebarDrag || readEngine().movingWindow()
        || readEngine().isOverviewOpen() || KWin::effects->hasActiveFullScreenEffect() || KWin::input()->qtButtonStates() != Qt::NoButton
        || KWin::waylandServer()->seat()->isDragPointer() || KWin::waylandServer()->isScreenLocked() || (active && active->isFullScreen());
}

void KonveyorEffect::trackEdgeHover(const QPointF &position)
{
    const std::optional<Layout::EdgeHoverTarget> target
        = edgeSwitchBlocked() ? std::nullopt : edgeTargetAt(position, d->outputs.outputs(), d->config.config().gestures.edgeSwitchColumns);
    switch (d->edgeHover.move(target)) {
    case Layout::EdgeHoverTracker::Change::Entered:
        d->edgeHoverTimer.start(d->config.config().gestures.edgeSwitchColumns.delayMs);
        break;
    case Layout::EdgeHoverTracker::Change::Left:
        d->edgeHoverTimer.stop();
        break;
    case Layout::EdgeHoverTracker::Change::None:
        break;
    }
}

void KonveyorEffect::switchColumnAtEdge()
{
    const std::optional<Layout::EdgeHoverTarget> target = d->edgeHover.take();
    if (!target || edgeSwitchBlocked()) {
        return;
    }
    if (readEngine().focusedOutput() != target->output) {
        changeEngine().focusOutput(target->output);
    }
    const QString action
        = target->side == Layout::HoverSide::Left ? QStringLiteral("focus-column-left") : QStringLiteral("focus-column-right");
    performAction({action, {}, {}});
}

void KonveyorEffect::resetEdgeHover()
{
    d->edgeHover.reset();
    d->edgeHoverTimer.stop();
}

}
