#include "layout/engine/engineprivate.h"

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

void Engine::Private::tickOverview()
{
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
        }
        return;
    }
    overview.progress = std::clamp(overview.progressAnimation->valueWithoutOvershoot(), 0.0, 1.0);
}

void Engine::Private::seedOverviewCamera()
{
    overview.cameras.clear();
    overview.cameraYAnimations.clear();
    const Workspace *workspace = activeWorkspace();
    overview.selected = focused ? focused : (workspace ? workspace->activeWindow() : std::nullopt);
}
}
