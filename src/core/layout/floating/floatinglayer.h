#pragma once

#include "layout/strip/columnstrip.h"
#include "layout/tile/tile.h"

#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <cstddef>
#include <optional>
#include <vector>

namespace Konveyor::Layout
{

struct FloatingData
{
    QPointF pos;
    QPointF absolutePos;
    QSizeF size;
    QRectF workingArea;

    void refreshAbsolutePos();
    void updateWorkingArea(QRectF area);
    void update(const Tile &tile);
    void setAbsolutePos(QPointF pos);
    bool operator==(const FloatingData &) const = default;
};

QPointF scaleByArea(QRectF area, QPointF frac);
QPointF logicalToFracInArea(QRectF area, QPointF logical);
QPointF applyPositionChange(QPointF current, QRectF area, PositionChange x, PositionChange y);
bool isAbsolutePositionChange(PositionChange change);

class FloatingLayer
{
public:
    FloatingLayer(QSizeF viewSize, QRectF workingArea, double scale, const Anim::Clock &clock, OptionsPtr options);

    void updateConfig(QSizeF viewSize, QRectF workingArea, double scale, OptionsPtr options);
    void tickAnimations();
    bool isAnimating() const;

    const std::vector<Tile> &tiles() const { return m_tiles; }
    std::vector<Tile> &tiles() { return m_tiles; }
    bool isEmpty() const { return m_tiles.empty(); }
    bool hasWindow(WindowId id) const;
    std::optional<WindowId> activeWindow() const { return m_activeWindow; }
    Tile *tileFor(WindowId id);
    QRectF workingArea() const { return m_workingArea; }
    QSizeF viewSize() const { return m_viewSize; }
    double scale() const { return m_scale; }
    const OptionsPtr &options() const { return m_options; }
    const Anim::Clock &clock() const { return m_clock; }
    const std::optional<ResizeSession> &interactiveResize() const { return m_resize; }

    void addTile(Tile tile, bool activate);
    void insertAbove(WindowId above, Tile tile, bool activate);
    DetachedTile removeTile(WindowId id);
    bool activateWindow(WindowId id);
    bool animateOpening(WindowId id);
    bool updateWindow(WindowId id);
    bool childrenAdded(WindowId id);

    void setWindowWidth(std::optional<WindowId> window, SizeChange change, bool animate);
    void setWindowHeight(std::optional<WindowId> window, SizeChange change, bool animate);
    void toggleWindowWidth(std::optional<WindowId> window, bool forwards);
    void toggleWindowHeight(std::optional<WindowId> window, bool forwards);

    void setFrame(WindowId window, QPointF tilePos, QSizeF windowSize);
    void moveWindow(std::optional<WindowId> window, PositionChange x, PositionChange y, bool animate);
    void centerWindow(std::optional<WindowId> window);

    std::vector<ConstTileRef> placedTiles(bool animated) const;
    std::vector<TileRef> renderedTilesMut(bool round);
    std::optional<WindowId> windowUnder(QPointF pos) const;

    bool beginResize(WindowId window, quint8 edges);
    bool updateResize(WindowId window, QPointF delta);
    void endResize(std::optional<WindowId> window);

    QPointF keepInsideWorkArea(QPointF pos, QSizeF size) const;
    QPointF relativeToAbsolute(QPointF frac) const;
    QPointF absoluteToRelative(QPointF logical) const;
    std::optional<QPointF> savedOrDefaultPosition(const Tile &tile) const;
    QSize initialWindowSize(const std::optional<Config::PresetSize> &width, const std::optional<Config::PresetSize> &height,
        const EffectiveWindowRules &rules) const;

    void refresh(bool isActive);
    QString checkConsistency() const;
    QString verifyTile(std::size_t index) const;

private:
    std::optional<std::size_t> indexOf(WindowId id) const;
    std::optional<std::size_t> targetIndex(std::optional<WindowId> window) const;
    void insertTile(std::size_t idx, Tile tile, bool activate);
    void raiseChildrenOf(std::size_t idx);
    void raiseWindow(std::size_t fromIndex, std::size_t toIndex);
    void moveTo(std::size_t idx, QPointF newPos, bool animate);
    void placeAnimated(std::size_t idx, QPointF newPos);
    void setWindowSize(std::size_t idx, SizeChange change, bool horizontal, bool animate);
    std::size_t togglePresetIndex(std::size_t idx, bool horizontal, bool forwards) const;
    void prepareTileSize(Tile &tile) const;

    std::vector<Tile> m_tiles;
    std::vector<FloatingData> m_data;
    std::optional<WindowId> m_activeWindow;
    std::optional<ResizeSession> m_resize;
    QSizeF m_viewSize;
    QRectF m_workingArea;
    double m_scale;
    Anim::Clock m_clock;
    OptionsPtr m_options;
};

}
