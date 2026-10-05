#include "layout/engine/engineprivate.h"

#include "config/log.h"

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

void Engine::Private::floatOverOutput(WindowId id)
{
    Workspace *workspace = workspaceOf(id);
    const Monitor *monitor = monitorOf(id);
    if (!workspace || !monitor) {
        return;
    }
    const QRectF screen = outputInfos.value(monitor->outputName()).geometry;
    qCInfo(lcKonveyor) << "konveyor: window" << id << "is locked to the size of" << monitor->outputName() << "so it floats over" << screen;
    if (!workspace->isFloating(id)) {
        workspace->placeWindowFloating(id, true);
    }
    setFloatingFrame(id, screen);
}

}
