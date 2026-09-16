#pragma once

#include "layout/common/layouttypes.h"
#include "layout/strip/tabbar.h"
#include "layout/tile/tile.h"

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <cstddef>
#include <optional>
#include <vector>

namespace Konveyor::Layout
{

struct AreaInfo
{
    QSizeF viewSize;
    QRectF workingArea;
    QRectF parentArea;
    double scale = 1.0;
    bool operator==(const AreaInfo &) const = default;
};

struct TileSizing
{
    WindowHeight height;
    bool resizingFromLeft = false;
    void update(const Tile &tile);
    bool operator==(const TileSizing &) const = default;
};

class Column
{
public:
    Column(Tile tile, const AreaInfo &area, ColumnWidth width, bool fillsWidth);

    quint64 id() const { return m_id; }
    void updateConfig(const AreaInfo &area, OptionsPtr options);
    void tickAnimations();
    bool isAnimating() const;

    WindowMode requestedMode() const;
    WindowMode sizingMode() const;

    QPointF animationOffset() const;
    void slideFromWith(QPointF from, const Config::AnimationParams &config);
    void slideXFrom(double from);
    void slideXFromWith(double from, const Config::AnimationParams &config);
    void shiftSlideX(double offset);

    bool contains(WindowId id) const;
    std::optional<Config::ColumnPosition> pinnedPosition() const;
    std::optional<std::size_t> position(WindowId id) const;
    bool selectTile(std::size_t idx);
    void activateWindow(WindowId id);
    void insertTile(std::size_t idx, Tile tile);
    void updateWindow(WindowId id);

    QSizeF reservedSize() const;
    PresetExtent presetWidthExtent(const Config::PresetSize &preset) const;
    PresetExtent presetHeightExtent(const Config::PresetSize &preset) const;
    double widthInPixels(ColumnWidth width) const;
    void layoutTiles(bool animate);
    double width() const;

    void focusTileAt(std::size_t index);
    bool focusUp();
    bool focusDown();
    void focusTop();
    void focusBottom();
    bool moveUp();
    bool moveDown();

    void toggleWidth(std::optional<std::size_t> tileIndex, bool forwards);
    void toggleFillWidth();
    void setColumnWidth(SizeChange change, std::optional<std::size_t> tileIndex, bool animate);
    void setWindowHeight(SizeChange change, std::optional<std::size_t> tileIndex, bool animate);
    void resetWindowHeight(std::optional<std::size_t> tileIndex);
    void toggleWindowHeight(std::optional<std::size_t> tileIndex, bool forwards);
    void resetHeightsToAuto();
    void setFullscreen(bool fullscreen);
    void setMaximized(bool maximize);
    void setColumnDisplay(Config::ColumnDisplay display);

    QPointF tileAreaOrigin() const;
    std::vector<QPointF> tilePositions() const;
    QPointF tilePosition(std::size_t idx) const;
    std::vector<std::size_t> renderOrder() const;
    QRectF tabBarRect() const;
    bool animateOpening(WindowId id);
    bool isTabbed() const { return displayStyle == Config::ColumnDisplay::Tabbed; }
    const OptionsPtr &options() const { return m_options; }
    const AreaInfo &area() const { return m_area; }
    const Anim::Clock &clock() const { return m_clock; }

    std::vector<Tile> tiles;
    std::vector<TileSizing> data;
    std::size_t activeTileIndex = 0;
    ColumnWidth widthSetting;
    std::optional<std::size_t> presetWidthIndex;
    bool fillsWidth = false;
    bool fullscreenPending = false;
    bool maximizePending = false;
    Config::ColumnDisplay displayStyle = Config::ColumnDisplay::Normal;
    TabBar tabBar;

private:
    void requestExpandedSizes(WindowMode mode, bool animate);
    double columnTileWidth() const;
    std::optional<double> maxFixedWindowHeight(const std::vector<QSizeF> &minSizes, double maxTileHeight) const;
    std::vector<WindowHeight> fixedHeights(std::optional<double> maxNonAuto, double maxTileHeight) const;
    void distributeHeights(
        std::vector<WindowHeight> &heights, const std::vector<QSizeF> &minSizes, const std::vector<QSizeF> &maxSizes) const;
    std::size_t nextPresetIndex(std::optional<std::size_t> current, std::size_t len, bool forwards) const;
    std::size_t findPresetWidthIndex(std::size_t tileIndex, bool forwards) const;
    std::size_t findPresetHeightIndex(std::size_t tileIndex, bool forwards) const;
    double presetTileHeight(const Tile &tile, const Config::PresetSize &preset) const;
    ColumnWidth computeNewWidth(SizeChange change, std::optional<std::size_t> tileIndex) const;
    double computeNewWindowHeight(SizeChange change, std::size_t tileIndex) const;
    void swapActive(std::size_t newIndex);

    std::optional<SlideAnimation> m_slideX;
    std::optional<SlideAnimation> m_slideY;
    AreaInfo m_area;
    Anim::Clock m_clock;
    OptionsPtr m_options;
    quint64 m_id;
};

}
