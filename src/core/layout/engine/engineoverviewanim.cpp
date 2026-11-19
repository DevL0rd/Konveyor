#include "layout/engine/engineprivate.h"

#include <utility>

namespace Konveyor::Layout
{

void Engine::Private::startOverviewAnimation(bool open)
{
    const double target = open ? 1.0 : 0.0;
    if (!config.animations.enabled || qFuzzyCompare(overview.progress, target)) {
        overview.progress = target;
        overview.progressAnimation.reset();
        return;
    }
    overview.progressAnimation.emplace(clock, overview.progress, target, 0.0, config.animations.workspaceSwitch);
}

void Engine::Private::animateOverviewCamera(const QString &output, const QPointF &from, const QPointF &to)
{
    overview.cameraYAnimations.remove(output);
    if (from == to || !config.animations.enabled) {
        overview.cameraPans.remove(output);
        overview.cameras.insert(output, to);
        return;
    }
    OverviewCameraPan pan(clock, from, to, config.animations.workspaceSwitch);
    if (pan.progress.isFinished()) {
        overview.cameraPans.remove(output);
        overview.cameras.insert(output, to);
        return;
    }
    overview.cameras.insert(output, from);
    overview.cameraPans.insert(output, std::move(pan));
}

void Engine::Private::stopOverviewCameraPan(const QString &output)
{
    const auto it = overview.cameraPans.find(output);
    if (it == overview.cameraPans.end()) {
        return;
    }
    // Keep the camera where the pan had reached, not where the last tick left it.
    overview.cameras.insert(output, it->position());
    overview.cameraPans.erase(it);
}

void Engine::Private::tickOverview()
{
    for (auto it = overview.cameraPans.begin(); it != overview.cameraPans.end();) {
        if (it->progress.isFinished()) {
            overview.cameras.insert(it.key(), it->to);
            it = overview.cameraPans.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = overview.cameraYAnimations.begin(); it != overview.cameraYAnimations.end();) {
        if (it->isFinished()) {
            it = overview.cameraYAnimations.erase(it);
        } else {
            ++it;
        }
    }
    if (!overview.progressAnimation) {
        return;
    }
    if (overview.progressAnimation->isFinished()) {
        overview.progress = overview.progressAnimation->to();
        overview.progressAnimation.reset();
        if (!overviewOpen) {
            overview.selected.reset();
            overview.dragWindow.reset();
            overview.reorderFrom = -1;
            overview.reorderOutput.clear();
            overview.cameraYAnimations.clear();
            overview.cameraPans.clear();
        }
        return;
    }
    overview.progress = std::clamp(overview.progressAnimation->valueWithoutOvershoot(), 0.0, 1.0);
}

void Engine::Private::seedOverviewCamera()
{
    overview.cameras.clear();
    overview.cameraYAnimations.clear();
    overview.cameraPans.clear();
    const Workspace *workspace = activeWorkspace();
    overview.selected = focused ? focused : (workspace ? workspace->activeWindow() : std::nullopt);
}
}
