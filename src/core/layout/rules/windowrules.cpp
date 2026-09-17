#include "layout/rules/windowrules.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

template<class T> void assignIfSet(std::optional<T> &target, const std::optional<T> &source)
{
    if (source) {
        target = source;
    }
}

bool flagMatches(const std::optional<bool> &expected, bool actual)
{
    return !expected || *expected == actual;
}

bool regexMatches(const std::optional<QRegularExpression> &regex, const QString &value)
{
    if (!regex) {
        return true;
    }
    if (value.isNull()) {
        return false;
    }
    return regex->match(value).hasMatch();
}

void mergeBorderRule(Config::BorderRule &target, const Config::BorderRule &source)
{
    assignIfSet(target.enabled, source.enabled);
    assignIfSet(target.width, source.width);
    assignIfSet(target.active, source.active);
    assignIfSet(target.inactive, source.inactive);
    assignIfSet(target.urgent, source.urgent);
}

void applyOpenRules(EffectiveWindowRules &resolved, const Config::WindowRule &rule)
{
    assignIfSet(resolved.defaultWidth, rule.defaultColumnWidth);
    assignIfSet(resolved.defaultHeight, rule.defaultWindowHeight);
    assignIfSet(resolved.defaultColumnDisplay, rule.defaultColumnDisplay);
    assignIfSet(resolved.defaultFloatingPosition, rule.defaultFloatingPosition);
    assignIfSet(resolved.openOnOutput, rule.openOnOutput);
    assignIfSet(resolved.openOnWorkspace, rule.openOnWorkspace);
    assignIfSet(resolved.openMaximized, rule.openMaximized);
    assignIfSet(resolved.openMaximizedToEdges, rule.openMaximizedToEdges);
    assignIfSet(resolved.openFullscreen, rule.openFullscreen);
    assignIfSet(resolved.openFloating, rule.openFloating);
    assignIfSet(resolved.openFocused, rule.openFocused);
    assignIfSet(resolved.manage, rule.manage);
    assignIfSet(resolved.columnPosition, rule.columnPosition);
    assignIfSet(resolved.groupAppWindows, rule.groupAppWindows);
    assignIfSet(resolved.maxRowsPerColumn, rule.maxRowsPerColumn);
    assignIfSet(resolved.floatChildWindows, rule.floatChildWindows);
}

void applyAppearanceRules(EffectiveWindowRules &resolved, const Config::WindowRule &rule)
{
    assignIfSet(resolved.minWidth, rule.minWidth);
    assignIfSet(resolved.minHeight, rule.minHeight);
    assignIfSet(resolved.maxWidth, rule.maxWidth);
    assignIfSet(resolved.maxHeight, rule.maxHeight);
    mergeBorderRule(resolved.focusRing, rule.focusRing);
    mergeBorderRule(resolved.border, rule.border);
    assignIfSet(resolved.opacity, rule.opacity);
    assignIfSet(resolved.geometryCornerRadius, rule.geometryCornerRadius);
    assignIfSet(resolved.clipToGeometry, rule.clipToGeometry);
}

}

QSize EffectiveWindowRules::limitMinSize(QSize minSize) const
{
    QSize size = minSize;
    if (minWidth) {
        size.setWidth(std::max(size.width(), *minWidth));
    }
    if (minHeight) {
        size.setHeight(std::max(size.height(), *minHeight));
    }
    return size;
}

namespace
{

int applyMaxComponent(int current, const std::optional<int> &rule)
{
    if (!rule) {
        return current;
    }
    if (current == 0) {
        return *rule;
    }
    if (*rule > 0) {
        return std::min(current, *rule);
    }
    return current;
}

}

QSize EffectiveWindowRules::limitMaxSize(QSize maxSize) const
{
    return {applyMaxComponent(maxSize.width(), maxWidth), applyMaxComponent(maxSize.height(), maxHeight)};
}

bool matchApplies(const Config::Match &match, const MatchContext &context, bool atStartup)
{
    return flagMatches(match.atStartup, atStartup) && flagMatches(match.isFocused, context.isFocused)
        && flagMatches(match.isUrgent, context.isUrgent) && flagMatches(match.isActive, context.isActive)
        && regexMatches(match.appId, context.appId) && regexMatches(match.title, context.title)
        && regexMatches(match.monitorProfile, context.monitorProfile) && flagMatches(match.isActiveInColumn, context.isActiveInColumn)
        && flagMatches(match.isFloating, context.isFloating);
}

bool ruleApplies(const Config::WindowRule &rule, const MatchContext &context, bool atStartup)
{
    const auto matches = [&](const Config::Match &m) { return matchApplies(m, context, atStartup); };
    if (!rule.matches.isEmpty() && std::ranges::none_of(rule.matches, matches)) {
        return false;
    }
    return std::ranges::none_of(rule.excludes, matches);
}

EffectiveWindowRules resolveWindowRules(const QList<Config::WindowRule> &rules, const MatchContext &context, bool atStartup)
{
    EffectiveWindowRules resolved;
    for (const Config::WindowRule &rule : rules) {
        if (!ruleApplies(rule, context, atStartup)) {
            continue;
        }
        applyOpenRules(resolved, rule);
        applyAppearanceRules(resolved, rule);
    }
    return resolved;
}

Config::Border mergeBorder(Config::Border border, const Config::BorderRule &rule)
{
    border.enabled = rule.enabled.value_or(border.enabled);
    border.width = rule.width.value_or(border.width);
    border.active = rule.active.value_or(border.active);
    border.inactive = rule.inactive.value_or(border.inactive);
    border.urgent = rule.urgent.value_or(border.urgent);
    return border;
}

}
