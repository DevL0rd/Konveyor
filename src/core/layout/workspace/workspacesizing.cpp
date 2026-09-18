#include "layout/workspace/workspace.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void Workspace::addToAppStack(const std::vector<std::size_t> &appColumns, Tile tile, bool activate, std::size_t maxRows)
{
    tile.returnsToFloating = false;
    m_strip.addToAppStack(appColumns, std::move(tile), activate, maxRows);
    if (activate) {
        m_floatingFocus = FloatingFocus::No;
    }
}

std::optional<Config::PresetSize> Workspace::defaultWidthFor(
    const std::optional<std::optional<Config::PresetSize>> &rule, bool isFloating) const
{
    if (rule) {
        return *rule;
    }
    return isFloating ? std::nullopt : m_options->layout.defaultColumnWidth;
}

std::optional<Config::PresetSize> Workspace::defaultHeightFor(
    const std::optional<std::optional<Config::PresetSize>> &rule, bool isFloating) const
{
    Q_UNUSED(isFloating)
    return rule ? *rule : std::nullopt;
}

QSize Workspace::initialWindowSize(const std::optional<Config::PresetSize> &width, const std::optional<Config::PresetSize> &height,
    bool isFloating, const EffectiveWindowRules &rules, QSize minSize, QSize maxSize) const
{
    QSize size = isFloating ? m_floating.initialWindowSize(width, height, rules) : m_strip.initialWindowSize(width, height, rules);
    const QSize resolvedMin = isFloating ? rules.limitMinSize(minSize) : rules.limitMinSize(QSize(0, 0));
    const QSize resolvedMax = isFloating ? rules.limitMaxSize(maxSize) : rules.limitMaxSize(QSize(0, 0));
    size.setWidth(clampToSizeLimitsAllowZero(size.width(), resolvedMin.width(), resolvedMax.width()));
    if (resolvedMin.height() == resolvedMax.height()) {
        size.setHeight(clampToSizeLimits(size.height(), resolvedMin.height(), resolvedMax.height()));
    } else if (size.height() > 0) {
        size.setHeight(std::max(size.height(), resolvedMin.height()));
    }
    return size;
}

ColumnWidth Workspace::tiledWidthFor(const LayoutWindow &window, const std::optional<Config::PresetSize> &width) const
{
    const Config::PresetSize preset = width.value_or(Config::PresetSize(Config::Fixed {window.size().width()}));
    if (const auto *proportion = std::get_if<Config::Proportion>(&preset)) {
        return ColumnWidth::proportion(proportion->value);
    }
    double fixed = std::get<Config::Fixed>(preset).value;
    const Config::Border border = mergeBorder(m_options->layout.border, window.rules().border);
    if (border.enabled) {
        fixed += border.width * 2.0;
    }
    return ColumnWidth::fixed(fixed);
}

void Workspace::setSizingMode(WindowId window, bool enable, bool maximize)
{
    bool returnsToFloating = false;
    if (m_floating.hasWindow(window)) {
        if (!enable) {
            return;
        }
        returnsToFloating = true;
        toggleWindowFloating(window);
    } else if (!enable && shouldRestoreToFloating(window, maximize)) {
        toggleWindowFloating(window);
        return;
    }

    const Tile *tile = m_strip.tileFor(window);
    if (!tile) {
        return;
    }
    const bool wasNormal = tile->window().requestedMode() == WindowMode::Normal;
    if (maximize) {
        m_strip.setMaximized(window, enable);
    } else {
        m_strip.setFullscreen(window, enable);
    }
    rememberRestoreToFloating(window, wasNormal, returnsToFloating);
}

void Workspace::setFullscreen(WindowId window, bool fullscreen)
{
    setSizingMode(window, fullscreen, false);
}

void Workspace::setMaximized(WindowId window, bool maximize)
{
    setSizingMode(window, maximize, true);
}

void Workspace::toggleFullscreen(WindowId window)
{
    const Tile *tile = m_strip.tileFor(window);
    const bool current = tile && tile->window().requestedMode() == WindowMode::Fullscreen;
    setFullscreen(window, !current);
}

void Workspace::toggleMaximized(WindowId window)
{
    const Column *column = m_strip.columnFor(window);
    setMaximized(window, !(column && column->maximizePending));
}

void Workspace::cycleExpansion(WindowId window)
{
    const Column *column = m_strip.columnFor(window);
    if (!column || m_strip.activeColumn() != column) {
        toggleMaximized(window);
        return;
    }
    if (column->maximizePending) {
        setMaximized(window, false);
        return;
    }
    const bool wasFullWidth = column->fillsWidth;
    m_strip.toggleFillWidth();
    if (wasFullWidth) {
        setMaximized(window, true);
    }
}

void Workspace::toggleWidth(bool forwards)
{
    if (isFloatingFocused()) {
        m_floating.toggleWindowWidth(std::nullopt, forwards);
    } else {
        m_strip.toggleWidth(forwards);
    }
}

void Workspace::toggleFillWidth()
{
    if (!isFloatingFocused()) {
        m_strip.toggleFillWidth();
    }
}

void Workspace::toggleFillWidthFor(WindowId window)
{
    m_strip.toggleFillWidthFor(window);
}

void Workspace::setColumnWidth(SizeChange change)
{
    if (isFloatingFocused()) {
        m_floating.setWindowWidth(std::nullopt, change, true);
    } else {
        m_strip.setWindowWidth(std::nullopt, change);
    }
}

void Workspace::setWindowWidth(std::optional<WindowId> window, SizeChange change)
{
    if (targetIsFloating(window)) {
        m_floating.setWindowWidth(window, change, true);
    } else {
        m_strip.setWindowWidth(window, change);
    }
}

void Workspace::setWindowHeight(std::optional<WindowId> window, SizeChange change)
{
    if (targetIsFloating(window)) {
        m_floating.setWindowHeight(window, change, true);
    } else {
        m_strip.setWindowHeight(window, change);
    }
}

void Workspace::resetWindowHeight(std::optional<WindowId> window)
{
    if (!targetIsFloating(window)) {
        m_strip.resetWindowHeight(window);
    }
}

void Workspace::toggleWindowWidth(std::optional<WindowId> window, bool forwards)
{
    if (targetIsFloating(window)) {
        m_floating.toggleWindowWidth(window, forwards);
    } else {
        m_strip.toggleWindowWidth(window, forwards);
    }
}

void Workspace::toggleWindowHeight(std::optional<WindowId> window, bool forwards)
{
    if (targetIsFloating(window)) {
        m_floating.toggleWindowHeight(window, forwards);
    } else {
        m_strip.toggleWindowHeight(window, forwards);
    }
}

void Workspace::expandColumnToAvailableWidth()
{
    if (!isFloatingFocused()) {
        m_strip.expandColumnToAvailableWidth();
    }
}

}
