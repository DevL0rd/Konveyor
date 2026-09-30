#include "layout/engine/engineprivate.h"

namespace Konveyor::Layout
{

bool Engine::beginResize(WindowId id, quint8 edges)
{
    Workspace *workspace = d->workspaceOf(id);
    if (!workspace) {
        return false;
    }
    d->finishResize();
    if (!workspace->beginResize(id, edges)) {
        return false;
    }
    d->resizeWindow = id;
    return true;
}

void Engine::updateResize(const QPointF &delta)
{
    if (!d->resizeWindow) {
        return;
    }
    if (Workspace *workspace = d->workspaceOf(*d->resizeWindow)) {
        workspace->updateResize(*d->resizeWindow, delta);
    }
    d->refresh();
}

void Engine::Private::finishResize()
{
    if (!resizeWindow) {
        return;
    }
    if (Workspace *workspace = workspaceOf(*resizeWindow)) {
        workspace->endResize(resizeWindow);
    }
    resizeWindow.reset();
}

void Engine::endResize()
{
    if (!d->resizeWindow) {
        return;
    }
    d->finishResize();
    d->refresh();
}

}
