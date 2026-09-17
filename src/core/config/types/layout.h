#pragma once

#include "config/types/paint.h"

#include <QList>
#include <QString>

#include <optional>

namespace Konveyor::Config
{

struct Layout
{
    double gaps = 16;
    CenterFocusedColumn centerFocusedColumn = CenterFocusedColumn::Never;
    NewColumnPosition newColumnPosition = NewColumnPosition::Right;
    bool alwaysCenterSingleColumn = false;
    bool emptyWorkspaceAboveFirst = false;
    ColumnDisplay defaultColumnDisplay = ColumnDisplay::Normal;
    QList<PresetSize> presetColumnWidths;
    std::optional<PresetSize> defaultColumnWidth = Proportion {0.25};
    bool rememberWindowSizes = false;
    bool rememberWindowPositions = false;
    GroupAppWindows groupAppWindows = GroupAppWindows::Beside;
    int maxRowsPerColumn = 3;
    NewWindowPlacement newWindowPlacement = NewWindowPlacement::Column;
    bool floatChildWindows = false;
    QList<PresetSize> presetWindowHeights;
    Struts struts;
    Border focusRing;
    Border border;
    TabIndicator tabIndicator;
    InsertHint insertHint;
    QColor backgroundColor {0x40, 0x40, 0x40};
    bool operator==(const Layout &) const = default;
};

struct LayoutPart
{
    std::optional<double> gaps;
    std::optional<CenterFocusedColumn> centerFocusedColumn;
    std::optional<NewColumnPosition> newColumnPosition;
    std::optional<bool> alwaysCenterSingleColumn;
    std::optional<bool> emptyWorkspaceAboveFirst;
    std::optional<ColumnDisplay> defaultColumnDisplay;
    std::optional<QList<PresetSize>> presetColumnWidths;
    std::optional<std::optional<PresetSize>> defaultColumnWidth;
    std::optional<bool> rememberWindowSizes;
    std::optional<bool> rememberWindowPositions;
    std::optional<GroupAppWindows> groupAppWindows;
    std::optional<int> maxRowsPerColumn;
    std::optional<NewWindowPlacement> newWindowPlacement;
    std::optional<bool> floatChildWindows;
    std::optional<QList<PresetSize>> presetWindowHeights;
    std::optional<Struts> struts;
    std::optional<BorderRule> focusRing;
    std::optional<BorderRule> border;
    std::optional<TabIndicatorPart> tabIndicator;
    std::optional<InsertHintPart> insertHint;
    std::optional<QColor> backgroundColor;
    bool operator==(const LayoutPart &) const = default;
};

}
