#include "layout/engine/engineprivate.h"

#include "config/log.h"
#include "layout/common/geometry.h"

#include <cmath>

namespace Konveyor::Layout
{

bool Engine::Private::isLockedToOutputSize(WindowId id)
{
    const Tile *tile = tileOf(id);
    const Monitor *monitor = monitorOf(id);
    if (!tile || !monitor) {
        return false;
    }
    const WindowProperties &properties = tile->window().properties();
    const auto output = outputInfos.constFind(monitor->outputName());
    if (properties.isResizable || output == outputInfos.constEnd() || properties.minSize.isEmpty()
        || properties.minSize != properties.maxSize) {
        return false;
    }
    const QSizeF screen = output->geometry.size();
    return std::abs(properties.minSize.width() - screen.width()) < 1.0 && std::abs(properties.minSize.height() - screen.height()) < 1.0;
}

void Engine::Private::presentOverOutput(WindowId id)
{
    Workspace *workspace = workspaceOf(id);
    const Monitor *monitor = monitorOf(id);
    if (!workspace || !monitor) {
        return;
    }
    qCInfo(lcKonveyor) << "konveyor: window" << id << "is locked to the size of" << monitor->outputName()
                       << "so it covers it in its column";
    workspace->setFullscreen(id, true);
    Tile *tile = workspace->tileFor(id);
    tile->coversLockedOutput = true;
    tile->window().requestWindowedFullscreen(true);
}

void Engine::setWindowNativeSize(WindowId id, const QSizeF &frameSize)
{
    Tile *tile = d->tileOf(id);
    if (!tile) {
        return;
    }
    tile->window().setNativeSize(roundedSize(frameSize));
    if (Workspace *workspace = d->workspaceOf(id)) {
        workspace->updateWindow(id);
    } else {
        tile->updateWindow();
    }
    d->refresh();
}

}
