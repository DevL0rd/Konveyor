#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"
#include "layout/rules/windowrules.h"

namespace Konveyor::Layout
{

namespace
{

template<typename T> bool store(std::optional<T> &slot, const T &value)
{
    if (slot == value) {
        return false;
    }
    slot = value;
    return true;
}

bool rememberColumns(WindowMemory &memory, const ColumnStrip &strip)
{
    bool changed = false;
    for (const Column &column : strip.columns()) {
        if (column.fillsWidth || column.requestedMode() != WindowMode::Normal) {
            continue;
        }
        for (const Tile &tile : column.tiles) {
            const QString &appId = tile.window().properties().appId;
            if (!appId.isEmpty()) {
                changed |= store(memory[appId].columnWidth, column.widthSetting);
            }
        }
    }
    return changed;
}

bool rememberFloating(WindowMemory &memory, const FloatingLayer &floating, bool sizes, bool positions)
{
    bool changed = false;
    for (std::size_t i = 0; i < floating.tiles().size(); ++i) {
        const LayoutWindow &window = floating.tiles()[i].window();
        const QString &appId = window.properties().appId;
        if (appId.isEmpty() || window.requestedMode() != WindowMode::Normal) {
            continue;
        }
        if (sizes && !window.size().isEmpty()) {
            changed |= store(memory[appId].floatingSize, window.size().toSize());
        }
        if (positions) {
            changed |= store(memory[appId].floatingPosition, floating.data()[i].pos);
        }
    }
    return changed;
}

}

void Engine::Private::rememberWindows()
{
    const bool sizes = config.layout.rememberWindowSizes;
    const bool positions = config.layout.rememberWindowPositions;
    if (!sizes && !positions) {
        return;
    }
    bool changed = false;
    for (Workspace *workspace : allWorkspaces()) {
        if (sizes) {
            changed |= rememberColumns(windowMemory, workspace->scrolling());
        }
        changed |= rememberFloating(windowMemory, workspace->floating(), sizes, positions);
    }
    if (changed && hooks.windowMemoryChanged) {
        hooks.windowMemoryChanged();
    }
}

void Engine::Private::applyRememberedSize(NewWindowPlan &plan, const QString &appId) const
{
    const auto remembered = windowMemory.constFind(appId);
    if (!config.layout.rememberWindowSizes || remembered == windowMemory.constEnd()) {
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
    const Config::Border border = mergeBorder(config.layout.border, plan.rules.border);
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
