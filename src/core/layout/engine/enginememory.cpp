#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"
#include "layout/rules/windowrules.h"

namespace Konveyor::Layout
{

namespace
{

template<typename T> void store(WindowMemory &memory, const QString &appId, std::optional<T> RememberedWindow::*field, const T &value)
{
    const auto it = memory.constFind(appId);
    if (it != memory.constEnd() && (*it).*field == value) {
        return;
    }
    memory[appId].*field = value;
}

void rememberColumns(WindowMemory &memory, const ColumnStrip &strip)
{
    for (const Column &column : strip.columns()) {
        if (column.fillsWidth || column.requestedMode() != WindowMode::Normal) {
            continue;
        }
        for (const Tile &tile : column.tiles) {
            const QString &appId = tile.window().properties().appId;
            if (!appId.isEmpty()) {
                store(memory, appId, &RememberedWindow::columnWidth, column.widthSetting);
            }
        }
    }
}

void rememberFloating(WindowMemory &memory, const FloatingLayer &floating, bool sizes, bool positions)
{
    for (std::size_t i = 0; i < floating.tiles().size(); ++i) {
        const LayoutWindow &window = floating.tiles()[i].window();
        const QString &appId = window.properties().appId;
        if (appId.isEmpty() || window.requestedMode() != WindowMode::Normal) {
            continue;
        }
        if (sizes && !window.size().isEmpty()) {
            store(memory, appId, &RememberedWindow::floatingSize, window.size().toSize());
        }
        if (positions) {
            store(memory, appId, &RememberedWindow::floatingPosition, floating.data()[i].pos);
        }
    }
}

void rememberNativeSizes(WindowMemory &memory, const Workspace &workspace)
{
    for (const ConstTileRef &ref : workspace.placedTiles(false)) {
        const LayoutWindow &window = ref.tile->window();
        const QString &appId = window.properties().appId;
        if (!appId.isEmpty() && window.nativeSize()) {
            store(memory, appId, &RememberedWindow::nativeSize, *window.nativeSize());
        }
    }
}

}

void Engine::Private::rememberWindows()
{
    WindowMemory updated = windowMemory;
    for (Workspace *workspace : allWorkspaces()) {
        const Config::Layout &layout = workspace->options()->layout;
        rememberNativeSizes(updated, *workspace);
        if (layout.rememberWindowSizes) {
            rememberColumns(updated, workspace->scrolling());
        }
        rememberFloating(updated, workspace->floating(), layout.rememberWindowSizes, layout.rememberWindowPositions);
    }
    if (updated == windowMemory) {
        return;
    }
    windowMemory = std::move(updated);
    if (hooks.windowMemoryChanged) {
        hooks.windowMemoryChanged();
    }
}

void Engine::Private::applyRememberedSize(NewWindowPlan &plan, const QString &appId, const Config::Layout &layout) const
{
    const auto remembered = windowMemory.constFind(appId);
    if (remembered == windowMemory.constEnd()) {
        return;
    }
    plan.nativeSize = remembered->nativeSize;
    if (!layout.rememberWindowSizes) {
        return;
    }
    if (plan.isFloating) {
        if (remembered->floatingSize && !plan.rules.defaultWidth) {
            plan.width = Config::Fixed {static_cast<double>(remembered->floatingSize->width())};
        }
        if (remembered->floatingSize && !plan.rules.defaultHeight) {
            plan.height = Config::Fixed {static_cast<double>(remembered->floatingSize->height())};
        }
        return;
    }
    if (!remembered->columnWidth || plan.rules.defaultWidth) {
        return;
    }
    const ColumnWidth width = *remembered->columnWidth;
    if (width.isProportion) {
        plan.width = Config::Proportion {width.value};
        return;
    }
    const Config::Border border = mergeBorder(layout.border, plan.rules.border);
    plan.width = Config::Fixed {border.enabled ? width.value - border.width * 2.0 : width.value};
}

void Engine::setWindowMemory(const WindowMemory &memory)
{
    d->windowMemory = memory;
}

const WindowMemory &Engine::windowMemory() const
{
    return d->windowMemory;
}

}
