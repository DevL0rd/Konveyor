#pragma once

#include "config/types/outputs.h"

#include <QList>
#include <QRegularExpression>
#include <QString>

#include <optional>

namespace Konveyor::Config
{

struct Match
{
    std::optional<QRegularExpression> appId;
    std::optional<QRegularExpression> title;
    std::optional<QRegularExpression> monitorProfile;
    std::optional<bool> isActive;
    std::optional<bool> isFocused;
    std::optional<bool> isActiveInColumn;
    std::optional<bool> isFloating;
    std::optional<bool> isUrgent;
    std::optional<bool> atStartup;
    bool operator==(const Match &) const = default;
};

enum class FloatingRelativeTo
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Top,
    Bottom,
    Left,
    Right
};

struct FloatingPosition
{
    double x = 0;
    double y = 0;
    FloatingRelativeTo relativeTo = FloatingRelativeTo::TopLeft;
    bool operator==(const FloatingPosition &) const = default;
};

struct CornerRadius
{
    double topLeft = 0;
    double topRight = 0;
    double bottomRight = 0;
    double bottomLeft = 0;
    bool operator==(const CornerRadius &) const = default;
};

struct WindowRule
{
    QList<Match> matches;
    QList<Match> excludes;

    std::optional<std::optional<PresetSize>> defaultColumnWidth;
    std::optional<std::optional<PresetSize>> defaultWindowHeight;
    std::optional<QString> openOnOutput;
    std::optional<QString> openOnWorkspace;
    std::optional<bool> openMaximized;
    std::optional<bool> openMaximizedToEdges;
    std::optional<bool> openFullscreen;
    std::optional<bool> openFloating;
    std::optional<bool> openFocused;
    std::optional<bool> manage;
    std::optional<ColumnPosition> columnPosition;
    std::optional<GroupAppWindows> groupAppWindows;
    std::optional<int> maxRowsPerColumn;

    std::optional<double> opacity;
    std::optional<int> minWidth;
    std::optional<int> maxWidth;
    std::optional<int> minHeight;
    std::optional<int> maxHeight;
    BorderRule focusRing;
    BorderRule border;
    std::optional<CornerRadius> geometryCornerRadius;
    std::optional<bool> clipToGeometry;
    std::optional<ColumnDisplay> defaultColumnDisplay;
    std::optional<FloatingPosition> defaultFloatingPosition;
    bool operator==(const WindowRule &) const = default;
};

}
