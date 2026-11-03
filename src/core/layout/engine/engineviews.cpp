#include "layout/engine/engineprivate.h"

namespace Konveyor::Layout
{

QList<OutputViewState> Engine::outputViewStates() const
{
    QList<OutputViewState> views;
    for (const Monitor &monitor : d->monitors) {
        const Workspace &workspace = monitor.activeWorkspace();
        const ColumnStrip &strip = workspace.scrolling();
        views.append({monitor.outputName(), workspace.id(), strip.viewOffset().current(), monitor.area().viewSize.width(),
            strip.activeColumnIndex()});
    }
    return views;
}

}
