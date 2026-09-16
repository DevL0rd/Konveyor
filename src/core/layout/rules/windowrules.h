#pragma once

#include "config/types.h"

#include <QList>
#include <QSize>
#include <QString>

#include <optional>

namespace Konveyor::Layout
{

struct EffectiveWindowRules
{
    std::optional<std::optional<Config::PresetSize>> defaultWidth;
    std::optional<std::optional<Config::PresetSize>> defaultHeight;
    std::optional<Config::ColumnDisplay> defaultColumnDisplay;
    std::optional<Config::FloatingPosition> defaultFloatingPosition;
    std::optional<QString> openOnOutput;
    std::optional<QString> openOnWorkspace;
    std::optional<bool> openMaximized;
    std::optional<bool> openMaximizedToEdges;
    std::optional<bool> openFullscreen;
    std::optional<bool> openFloating;
    std::optional<bool> openFocused;
    std::optional<bool> manage;
    std::optional<Config::ColumnPosition> columnPosition;
    std::optional<int> minWidth;
    std::optional<int> minHeight;
    std::optional<int> maxWidth;
    std::optional<int> maxHeight;
    Config::BorderRule focusRing;
    Config::BorderRule border;
    std::optional<double> opacity;
    std::optional<Config::CornerRadius> geometryCornerRadius;
    std::optional<bool> clipToGeometry;
    bool operator==(const EffectiveWindowRules &) const = default;

    QSize limitMinSize(QSize minSize) const;
    QSize limitMaxSize(QSize maxSize) const;
};

struct MatchContext
{
    QString appId;
    QString title;
    QString monitorProfile;
    bool isActive = false;
    bool isFocused = false;
    bool isActiveInColumn = true;
    bool isFloating = false;
    bool isUrgent = false;
};

bool matchApplies(const Config::Match &match, const MatchContext &context, bool atStartup);
bool ruleApplies(const Config::WindowRule &rule, const MatchContext &context, bool atStartup);
EffectiveWindowRules resolveWindowRules(const QList<Config::WindowRule> &rules, const MatchContext &context, bool atStartup);
Config::Border mergeBorder(Config::Border border, const Config::BorderRule &rule);

}
