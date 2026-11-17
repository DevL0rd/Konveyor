#pragma once

#include "consistencystrip.h"
#include "layout/engine/engine.h"
#include "layout/engine/engineprivate.h"
#include "layout/floating/floatinglayer.h"
#include "layout/monitor/monitor.h"
#include "layout/workspace/workspace.h"

#include <QHash>
#include <QSet>
#include <QString>

#include <cmath>

namespace Konveyor::Layout
{

struct ConsistencyCheck
{
    static QString areaError(const ColumnStrip &strip)
    {
        if (!(strip.m_area.viewSize.width() > 0.0) || !(strip.m_area.viewSize.height() > 0.0)) {
            return QStringLiteral("scrolling: view size must be positive");
        }
        if (!(strip.m_area.scale > 0.0) || !std::isfinite(strip.m_area.scale)) {
            return QStringLiteral("scrolling: scale must be positive and finite");
        }
        if (strip.m_area.workingArea != workAreaWithStruts(strip.m_area.parentArea, strip.m_area.scale, strip.m_options->layout.struts)) {
            return QStringLiteral("scrolling: working area must match the struts");
        }
        return {};
    }

    static QString columnsError(const ColumnStrip &strip)
    {
        int previousRank = 0;
        for (const Column &column : strip.m_columns) {
            if (const QString error = StripChecks::columnError(column); !error.isEmpty()) {
                return error;
            }
            const std::optional<Config::ColumnPosition> pin = strip.wrapsColumns() ? std::nullopt : column.pinnedPosition();
            const int rank = pin == Config::ColumnPosition::Start ? 0 : (pin == Config::ColumnPosition::End ? 2 : 1);
            if (rank < previousRank) {
                return QStringLiteral("scrolling: column %1 is outside its pinned end of the row").arg(column.id());
            }
            previousRank = rank;
        }
        return {};
    }

    static QString viewError(const ColumnStrip &strip)
    {
        if (strip.m_columns.empty() || strip.centersActiveColumn() || strip.m_scroll.isSwiping() || strip.m_resize
            || !std::ranges::all_of(strip.m_columns, StripChecks::isNormal)) {
            return {};
        }
        const double gaps = strip.m_options->layout.gaps;
        const QRectF &area = strip.m_area.workingArea;
        const double viewLeft = strip.targetScrollPosition() + area.x();
        const double viewRight = viewLeft + area.width();
        if (strip.wrapPeriod() > 0.0) {
            const double activeX = strip.columnOffset(strip.m_activeColumnIndex);
            const double activeRight = activeX + strip.m_columns[strip.m_activeColumnIndex].width();
            if (activeRight < viewLeft - StripChecks::Tolerance || activeX > viewRight + StripChecks::Tolerance) {
                return QStringLiteral("scrolling: the view %1..%2 left the active column %3..%4")
                    .arg(viewLeft)
                    .arg(viewRight)
                    .arg(activeX)
                    .arg(activeRight);
            }
            return {};
        }
        const double rowRight = strip.columnOffset(strip.m_columns.size() - 1) + strip.m_columns.back().width();
        const bool fits = rowRight + gaps * 2.0 <= area.width();
        const bool aligned = fits ? std::abs(viewLeft + gaps) <= StripChecks::Tolerance
                                  : viewLeft >= -gaps - StripChecks::Tolerance && viewRight <= rowRight + gaps + StripChecks::Tolerance;
        if (!aligned) {
            return QStringLiteral("scrolling: the view %1..%2 is past the row 0..%3").arg(viewLeft).arg(viewRight).arg(rowRight);
        }
        return {};
    }

    static QString of(const ColumnStrip &strip)
    {
        if (const QString error = areaError(strip); !error.isEmpty()) {
            return error;
        }
        if (strip.m_resize && !strip.hasWindow(strip.m_resize->window)) {
            return QStringLiteral("scrolling: interactive resize window must be present");
        }
        if (strip.m_columns.empty()) {
            return {};
        }
        if (strip.m_activeColumnIndex >= strip.m_columns.size()) {
            return QStringLiteral("scrolling: active column index out of range");
        }
        if (const QString error = columnsError(strip); !error.isEmpty()) {
            return error;
        }
        if (strip.m_scrollToRestore && strip.m_columns[strip.m_activeColumnIndex].sizingMode() == WindowMode::Normal) {
            return QStringLiteral("scrolling: the active column must be fullscreen or maximized to restore a view offset");
        }
        return {};
    }

    static bool outsideLimit(double size, int min, int max)
    {
        constexpr double Tolerance = 1.0;
        return size + Tolerance < min || (max > 0 && min <= max && size > max + Tolerance);
    }

    static QString tileError(const FloatingLayer &layer, std::size_t index)
    {
        const Tile &tile = layer.m_tiles[index];
        if (tile.window().requestedMode() != WindowMode::Normal) {
            return QStringLiteral("floating: windows cannot be maximized or fullscreen");
        }
        if (tile.window().isExpansionForceResizable()) {
            return QStringLiteral("floating: window %1 keeps the size exemption of an expanded column").arg(tile.id());
        }
        const QSizeF size = tile.targetWindowSize();
        const QSize min = tile.window().minSize();
        const QSize max = tile.window().maxSize();
        if (outsideLimit(size.width(), min.width(), max.width()) || outsideLimit(size.height(), min.height(), max.height())) {
            return QStringLiteral("floating: window %1 is outside its minimum or maximum size").arg(tile.id());
        }
        FloatingData expected = layer.m_data[index];
        expected.update(layer.m_tiles[index]);
        expected.updateWorkingArea(layer.m_workingArea);
        if (!(expected == layer.m_data[index])) {
            return QStringLiteral("floating: tile data must be up to date");
        }
        for (std::size_t other = index + 1; other < layer.m_tiles.size(); ++other) {
            if (layer.m_tiles[other].window().isChildOf(layer.m_tiles[index].window())) {
                return QStringLiteral("floating: children must be stacked above parents");
            }
        }
        return {};
    }

    static QString of(const FloatingLayer &layer)
    {
        if (!(layer.m_scale > 0.0) || !std::isfinite(layer.m_scale)) {
            return QStringLiteral("floating: scale must be positive and finite");
        }
        if (layer.m_tiles.size() != layer.m_data.size()) {
            return QStringLiteral("floating: tiles and data must have the same length");
        }
        if (layer.m_activeWindow.has_value() != !layer.m_tiles.empty()) {
            return QStringLiteral("floating: active window must be set exactly when tiles are present");
        }
        if (layer.m_activeWindow && !layer.hasWindow(*layer.m_activeWindow)) {
            return QStringLiteral("floating: active window must be present in tiles");
        }
        if (layer.m_resize && !layer.hasWindow(layer.m_resize->window)) {
            return QStringLiteral("floating: interactive resize window must be present in tiles");
        }
        for (std::size_t index = 0; index < layer.m_tiles.size(); ++index) {
            if (const QString error = tileError(layer, index); !error.isEmpty()) {
                return error;
            }
        }
        return {};
    }

    static QString of(const Workspace &workspace)
    {
        if (const QString error = of(workspace.m_strip); !error.isEmpty()) {
            return error;
        }
        if (const QString error = of(workspace.m_floating); !error.isEmpty()) {
            return error;
        }
        if (workspace.m_floating.isEmpty() && workspace.isFloatingFocused()) {
            return QStringLiteral("workspace: floating must not be active when empty");
        }
        if (!workspace.m_floating.isEmpty() && workspace.m_strip.isEmpty() && !workspace.isFloatingFocused()) {
            return QStringLiteral("workspace: floating must be active when scrolling is empty");
        }
        const OptionsPtr expected
            = makeOptions(scaledFor(withLayoutPart(*workspace.m_globalOptions, workspace.m_layoutOverride), workspace.m_area.scale));
        if (!(*workspace.m_options == *expected)) {
            return QStringLiteral("workspace: options must be derived from the base options");
        }
        return {};
    }

    static QString of(const Monitor &monitor)
    {
        if (monitor.m_workspaces.empty()) {
            return QStringLiteral("monitor: must have at least one workspace");
        }
        if (monitor.m_activeWorkspaceIndex >= monitor.m_workspaces.size()) {
            return QStringLiteral("monitor: active workspace index out of range");
        }
        if (monitor.m_options->layout.fixedWorkspaceCount == 0 && monitor.m_workspaces.back().isOccupiedOrNamed()) {
            return QStringLiteral("monitor: the last workspace must be empty");
        }
        if (monitor.m_options->layout.fixedWorkspaceCount == 0 && monitor.m_options->layout.emptyWorkspaceAboveFirst
            && monitor.m_workspaces.size() > 1 && monitor.m_workspaces[0].isOccupiedOrNamed()) {
            return QStringLiteral("monitor: the first workspace must be empty");
        }
        for (const Workspace &workspace : monitor.m_workspaces) {
            if (const QString error = of(workspace); !error.isEmpty()) {
                return error;
            }
        }
        return {};
    }

    static void countWorkspaceWindows(const Workspace &workspace, QHash<WindowId, int> &counts)
    {
        for (const Column &column : workspace.scrolling().columns()) {
            for (const Tile &tile : column.tiles) {
                counts[tile.id()] += 1;
            }
        }
        for (const Tile &tile : workspace.floating().tiles()) {
            counts[tile.id()] += 1;
        }
    }

    static QString duplicateNameError(const Workspace &workspace, QSet<QString> &names)
    {
        if (workspace.name().isEmpty()) {
            return {};
        }
        const QString key = workspace.name().toLower();
        if (names.contains(key)) {
            return QStringLiteral("engine: workspace name %1 is used more than once").arg(workspace.name());
        }
        names.insert(key);
        return {};
    }

    static QString viewsError(const Engine::Private &engine)
    {
        for (const Monitor &monitor : engine.monitors) {
            for (const Workspace &workspace : monitor.workspaces()) {
                if (const QString error = viewError(workspace.scrolling()); !error.isEmpty()) {
                    return error;
                }
            }
        }
        for (const Workspace &workspace : engine.orphanWorkspaces) {
            if (const QString error = viewError(workspace.scrolling()); !error.isEmpty()) {
                return error;
            }
        }
        return {};
    }

    static QString windowPlacement(const Engine::Private &engine)
    {
        QHash<WindowId, int> counts;
        QSet<QString> names;
        std::vector<const Workspace *> workspaces;
        for (const Monitor &monitor : engine.monitors) {
            for (const Workspace &workspace : monitor.workspaces()) {
                workspaces.push_back(&workspace);
            }
        }
        for (const Workspace &workspace : engine.orphanWorkspaces) {
            workspaces.push_back(&workspace);
        }
        for (const Workspace *workspace : workspaces) {
            countWorkspaceWindows(*workspace, counts);
            if (const QString error = duplicateNameError(*workspace, names); !error.isEmpty()) {
                return error;
            }
        }
        if (engine.windowDrag && engine.windowDrag->tile) {
            counts[engine.windowDrag->tile->id()] += 1;
        }
        for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
            if (it.value() != 1) {
                return QStringLiteral("engine: window %1 is placed %2 times").arg(it.key()).arg(it.value());
            }
        }
        if (engine.focused && !counts.contains(*engine.focused)) {
            return QStringLiteral("engine: focused window %1 is not in the layout").arg(*engine.focused);
        }
        return {};
    }

    static QString of(const Engine &engine)
    {
        const Engine::Private &state = *engine.d;
        for (const Monitor &monitor : state.monitors) {
            if (const QString error = of(monitor); !error.isEmpty()) {
                return error;
            }
        }
        if (!state.monitors.empty() && state.activeMonitorIndex >= state.monitors.size()) {
            return QStringLiteral("engine: active monitor index out of range");
        }
        for (const Workspace &workspace : state.orphanWorkspaces) {
            if (const QString error = of(workspace); !error.isEmpty()) {
                return error;
            }
        }
        if (const QString error = windowPlacement(state); !error.isEmpty()) {
            return error;
        }
        return state.windowDrag && state.windowDrag->moving ? QString() : viewsError(state);
    }
};

}
