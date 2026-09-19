#include "layout/strip/column.h"

#include "layout/common/sizelimits.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>

namespace Konveyor::Layout
{

namespace
{

template<class Pred> std::optional<std::size_t> findPreset(std::size_t len, bool forwards, Pred pred)
{
    for (std::size_t n = 0; n < len; ++n) {
        const std::size_t i = forwards ? n : len - 1 - n;
        if (pred(i)) {
            return i;
        }
    }
    return std::nullopt;
}

}

void Column::toggleWidth(std::optional<std::size_t> tileIndex, bool forwards, bool fromNative)
{
    const std::size_t idx = tileIndex.value_or(activeTileIndex);
    const auto current = (fillsWidth || maximizePending) ? std::nullopt : presetWidthIndex;
    const auto len = static_cast<std::size_t>(m_options->layout.presetColumnWidths.size());
    const auto closest = fromNative ? closestWidthPresetIndex(idx) : std::nullopt;
    const std::size_t presetIndex = closest ? (forwards ? *closest : (*closest + len - 1) % len)
                                            : current ? nextPresetIndex(current, len, forwards) : findPresetWidthIndex(idx, forwards);
    const Config::PresetSize preset = m_options->layout.presetColumnWidths[static_cast<qsizetype>(presetIndex)];
    setColumnWidth(sizeChangeFromPreset(preset), idx, true);
    presetWidthIndex = presetIndex;
}

std::optional<std::size_t> Column::closestWidthPresetIndex(std::size_t tileIndex) const
{
    const auto nativeSize = tiles[tileIndex].window().nativeSize();
    const auto &presets = m_options->layout.presetColumnWidths;
    if (!nativeSize || presets.isEmpty()) {
        return std::nullopt;
    }
    const double nativeWindow = nativeSize->width();
    const double nativeTile = tiles[tileIndex].outerWidthFor(nativeWindow);
    std::size_t closest = 0;
    double closestDistance = std::numeric_limits<double>::max();
    for (qsizetype i = 0; i < presets.size(); ++i) {
        const PresetExtent resolved = presetWidthExtent(presets[i]);
        const double distance = std::abs(resolved.value - (resolved.isTile ? nativeTile : nativeWindow));
        if (distance < closestDistance) {
            closest = static_cast<std::size_t>(i);
            closestDistance = distance;
        }
    }
    return closest;
}

ColumnWidth Column::computeNewWidth(SizeChange change, std::optional<std::size_t> tileIndex) const
{
    const ColumnWidth current = (fillsWidth || maximizePending) ? ColumnWidth::proportion(1.0) : widthSetting;
    const double currentPx = widthInPixels(current);
    switch (change.kind) {
    case ChangeKind::SetFixed:
        return ColumnWidth::fixed(std::clamp(tiles[tileIndex.value_or(activeTileIndex)].outerWidthFor(change.value), 1.0, MaxPixelSize));
    case ChangeKind::SetProportion:
        return ColumnWidth::proportion(std::clamp(change.value / 100.0, 0.0, MaxProportion));
    case ChangeKind::AdjustFixed:
        return ColumnWidth::fixed(std::clamp(currentPx + change.value, 1.0, MaxPixelSize));
    case ChangeKind::AdjustProportion:
        break;
    }
    if (current.isProportion) {
        return ColumnWidth::proportion(std::clamp(current.value + change.value / 100.0, 0.0, MaxProportion));
    }
    const double gaps = m_options->layout.gaps;
    const double full = m_area.workingArea.width() - gaps;
    const double currentProp = full == 0.0 ? 1.0 : (currentPx + gaps + reservedSize().width()) / full;
    return ColumnWidth::proportion(std::clamp(currentProp + change.value / 100.0, 0.0, MaxProportion));
}

void Column::setColumnWidth(SizeChange change, std::optional<std::size_t> tileIndex, bool animate)
{
    widthSetting = computeNewWidth(change, tileIndex);
    presetWidthIndex.reset();
    fillsWidth = false;
    maximizePending = false;
    layoutTiles(animate);
}

std::size_t Column::nextPresetIndex(std::optional<std::size_t> current, std::size_t len, bool forwards) const
{
    return (*current + (forwards ? 1 : len - 1)) % len;
}

std::size_t Column::findPresetWidthIndex(std::size_t tileIndex, bool forwards) const
{
    const Tile &tile = tiles[tileIndex];
    const double currentWindow = tile.pendingWindowSize().width();
    const double currentTile = tile.pendingOuterSize().width();
    const auto &presets = m_options->layout.presetColumnWidths;
    const auto len = static_cast<std::size_t>(presets.size());
    const auto found = findPreset(len, forwards, [&](std::size_t i) {
        const PresetExtent resolved = presetWidthExtent(presets[static_cast<qsizetype>(i)]);
        const double current = resolved.isTile ? currentTile : currentWindow;
        return forwards ? current + 1.0 < resolved.value : resolved.value + 1.0 < current;
    });
    return found.value_or(forwards ? 0 : len - 1);
}

double Column::presetTileHeight(const Tile &tile, const Config::PresetSize &preset) const
{
    const PresetExtent resolved = presetHeightExtent(preset);
    const double windowHeight = resolved.isTile ? tile.innerHeightFor(resolved.value) : resolved.value;
    return tile.outerHeightFor(std::clamp(std::round(windowHeight), 1.0, MaxPixelSize));
}

std::size_t Column::findPresetHeightIndex(std::size_t tileIndex, bool forwards) const
{
    const double current = tiles[tileIndex].pendingOuterSize().height();
    const auto &presets = m_options->layout.presetWindowHeights;
    const auto len = static_cast<std::size_t>(presets.size());
    const auto found = findPreset(len, forwards, [&](std::size_t i) {
        const double resolved = presetTileHeight(tiles[tileIndex], presets[static_cast<qsizetype>(i)]);
        return forwards ? current + 1.0 < resolved : resolved + 1.0 < current;
    });
    return found.value_or(forwards ? 0 : len - 1);
}

double Column::computeNewWindowHeight(SizeChange change, std::size_t tileIndex) const
{
    const Tile &tile = tiles[tileIndex];
    const WindowHeight current = data[tileIndex].height;
    const double currentWindowPx = current.kind == WindowHeight::Kind::Fixed ? current.value : tile.windowSize().height();
    const double currentTilePx = tile.outerHeightFor(currentWindowPx);
    const double working = m_area.workingArea.height();
    const double gaps = m_options->layout.gaps;
    const double extra = reservedSize().height();
    const double full = working - gaps;
    const double currentProp = full == 0.0 ? 1.0 : (currentTilePx + gaps) / full;

    double windowHeight = 0.0;
    switch (change.kind) {
    case ChangeKind::SetFixed:
        windowHeight = change.value;
        break;
    case ChangeKind::SetProportion:
        windowHeight = tile.innerHeightFor((working - gaps) * (change.value / 100.0) - gaps - extra);
        break;
    case ChangeKind::AdjustFixed:
        windowHeight = currentWindowPx + change.value;
        break;
    case ChangeKind::AdjustProportion:
        windowHeight = tile.innerHeightFor((working - gaps) * (currentProp + change.value / 100.0) - gaps - extra);
        break;
    }

    double minHeightTaken = 0.0;
    for (std::size_t i = 0; i < tiles.size() && !isTabbed(); ++i) {
        if (i != tileIndex) {
            minHeightTaken += std::max(1.0, tiles[i].minNormalSize().height()) + gaps;
        }
    }
    const double heightLeft = std::max(1.0, tile.innerHeightFor(working - extra - gaps - minHeightTaken - gaps));
    windowHeight = std::min(heightLeft, windowHeight);

    const QSize minSize = tile.window().tiledMinSize();
    const QSize maxSize = tile.window().tiledMaxSize();
    if (maxSize.height() > 0) {
        windowHeight = std::min(windowHeight, static_cast<double>(maxSize.height()));
    }
    if (minSize.height() > 0) {
        windowHeight = std::max(windowHeight, static_cast<double>(minSize.height()));
    }
    return windowHeight;
}

void Column::setWindowHeight(SizeChange change, std::optional<std::size_t> tileIndex, bool animate)
{
    const std::size_t idx = tileIndex.value_or(activeTileIndex);
    if (data[idx].height.isAuto()) {
        resetHeightsToAuto();
    }
    const double windowHeight = computeNewWindowHeight(change, idx);
    data[idx].height = WindowHeight::fixed(std::clamp(windowHeight, 1.0, MaxPixelSize));
    maximizePending = false;
    layoutTiles(animate);
}

void Column::resetWindowHeight(std::optional<std::size_t> tileIndex)
{
    if (isTabbed()) {
        for (TileSizing &tileData : data) {
            tileData.height = WindowHeight::autoWeight(1.0);
        }
    } else {
        data[tileIndex.value_or(activeTileIndex)].height = WindowHeight::autoWeight(1.0);
    }
    layoutTiles(true);
}

void Column::toggleWindowHeight(std::optional<std::size_t> tileIndex, bool forwards)
{
    const std::size_t idx = tileIndex.value_or(activeTileIndex);
    if (data[idx].height.isAuto()) {
        resetHeightsToAuto();
    }
    const auto len = static_cast<std::size_t>(m_options->layout.presetWindowHeights.size());
    const WindowHeight current = data[idx].height;
    std::size_t presetIndex = 0;
    if (current.kind == WindowHeight::Kind::Preset && !maximizePending) {
        presetIndex = nextPresetIndex(current.preset, len, forwards);
    } else {
        presetIndex = findPresetHeightIndex(idx, forwards);
    }
    data[idx].height = WindowHeight::presetIndex(presetIndex);
    maximizePending = false;
    layoutTiles(true);
}

}
