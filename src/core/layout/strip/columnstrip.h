#pragma once

#include "layout/strip/column.h"
#include "layout/strip/stripscroll.h"

#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace Konveyor::Layout
{

struct ResizeSession
{
    WindowId window = 0;
    QSizeF originalWindowSize;
    quint8 edges = 0;
};

struct ColumnRef
{
    const Column *column = nullptr;
    QPointF pos;
};

class ColumnStrip
{
public:
    ColumnStrip(QSizeF viewSize, QRectF parentArea, double scale, const Anim::Clock &clock, OptionsPtr options);

    void updateConfig(QSizeF viewSize, QRectF parentArea, double scale, OptionsPtr options);
    void tickAnimations();
    bool isAnimating() const;

    const std::vector<Column> &columns() const { return m_columns; }
    void applyDefaultColumnWidths();
    bool defaultWidthsPending() const { return m_defaultWidthsPending; }
    std::vector<Column> &columns() { return m_columns; }
    bool isEmpty() const { return m_columns.empty(); }
    bool hasWindow(WindowId id) const;
    std::size_t activeColumnIndex() const { return m_activeColumnIndex; }
    const Tile *activeTile() const;
    Tile *activeTile();
    const Column *columnFor(WindowId id) const;
    Tile *tileFor(WindowId id);
    const Tile *tileFor(WindowId id) const;
    bool activeTileWantsFullscreen() const;
    bool centersActiveColumn() const;
    const AreaInfo &area() const { return m_area; }
    const OptionsPtr &options() const { return m_options; }
    const StripScroll &viewOffset() const { return m_scroll; }
    const std::optional<ResizeSession> &interactiveResize() const { return m_resize; }
    std::optional<double> scrollToRestore() const { return m_scrollToRestore; }

    DropSlot dropSlotAt(QPointF pos, std::optional<Config::ColumnPosition> movingPin) const;
    QSize initialWindowSize(const std::optional<Config::PresetSize> &width, const std::optional<Config::PresetSize> &height,
        const EffectiveWindowRules &rules) const;
    void addTile(std::optional<std::size_t> colIndex, Tile tile, bool activate, ColumnWidth width, bool fillsWidth,
        std::optional<Config::AnimationParams> anim);
    void insertIntoColumn(std::size_t colIndex, std::optional<std::size_t> tileIndex, Tile tile, bool activate);
    void insertTileAfter(WindowId rightOf, Tile tile, bool activate, ColumnWidth width, bool fillsWidth);
    std::vector<std::size_t> appColumnIndices(const QString &appId) const;
    void addToAppStack(const std::vector<std::size_t> &appColumns, Tile tile, bool activate, std::size_t maxRows);
    void addColumn(std::optional<std::size_t> idx, Column column, bool activate, std::optional<Config::AnimationParams> anim);
    DetachedTile removeTile(WindowId id);
    DetachedTile detachTileAt(std::size_t columnIndex, std::size_t tileIndex, std::optional<Config::AnimationParams> anim);
    const Column *activeColumn() const;
    std::optional<Column> removeActiveColumn();
    Column detachColumnAt(std::size_t columnIndex, std::optional<Config::AnimationParams> anim);
    void updateWindow(WindowId id);
    bool activateWindow(WindowId id);
    bool animateOpening(WindowId id);

    bool focusLeft();
    bool focusRight();
    void focusColumnFirst();
    void focusColumnLast();
    void focusColumn(std::size_t index);
    void focusWindowInColumn(std::size_t index);
    bool focusDown();
    bool focusUp();
    void focusDownOrLeft();
    void focusDownOrRight();
    void focusUpOrLeft();
    void focusUpOrRight();
    void focusTop();
    void focusBottom();
    void moveColumnToIndex(std::size_t index);
    bool moveLeft();
    bool moveRight();
    void placeColumnWithinPins(std::size_t from);
    void activateAfterRemovingActive(std::size_t removedIndex, const Config::AnimationParams &viewConfig);
    DropSlot unpinnedDropSlotAt(QPointF pos) const;
    std::size_t allowedColumnIndex(
        std::optional<Config::ColumnPosition> pin, std::size_t desired, std::optional<std::size_t> excluding) const;
    void moveColumnToFirst();
    void moveColumnToLast();
    bool moveDown();
    bool moveUp();
    void consumeOrExpelWindowLeft(std::optional<WindowId> window);
    void consumeOrExpelWindowRight(std::optional<WindowId> window);
    void consumeIntoColumn();
    void expelFromColumn();
    void swapWindowInDirection(ScrollDirection direction);
    void toggleColumnTabbedDisplay();
    void setColumnDisplay(Config::ColumnDisplay display);
    void centerColumn();
    void centerWindow(std::optional<WindowId> window);
    void centerVisibleColumns();

    double scrollPosition() const;
    double targetScrollPosition() const;
    std::vector<double> columnOffsets() const;
    double columnOffset(std::size_t idx) const;
    std::vector<ColumnRef> renderedColumns() const;
    std::vector<ColumnRef> placedColumns(bool animated) const;
    std::vector<ConstTileRef> placedTiles(bool animated) const;
    std::vector<TileRef> renderedTilesMut(bool round);
    std::optional<QPointF> columnRenderPosition(quint64 columnId) const;
    Column *columnById(quint64 columnId);
    std::optional<QRectF> dropSlotRect(DropSlot position) const;

    void toggleWidth(bool forwards);
    void toggleFillWidth();
    void toggleFillWidthFor(WindowId window);
    void setWindowWidth(std::optional<WindowId> window, SizeChange change);
    void setWindowHeight(std::optional<WindowId> window, SizeChange change);
    void resetWindowHeight(std::optional<WindowId> window);
    void toggleWindowWidth(std::optional<WindowId> window, bool forwards);
    void toggleWindowHeight(std::optional<WindowId> window, bool forwards);
    void expandColumnToAvailableWidth();
    bool setFullscreen(WindowId window, bool fullscreen);
    bool setMaximized(WindowId window, bool maximize);
    bool drawsAbovePanels() const;
    std::optional<WindowId> windowUnder(QPointF pos) const;

    void beginSwipe(bool isTouchpad);
    std::optional<bool> updateSwipe(double deltaX, Anim::Duration timestamp, bool isTouchpad);
    bool endSwipe(std::optional<bool> isTouchpad, std::optional<WindowId> keepActive = std::nullopt);
    void beginEdgeScroll();
    bool edgeScrollBy(double delta);
    void endEdgeScroll();

    bool beginResize(WindowId window, quint8 edges);
    bool updateResize(WindowId window, QPointF delta);
    void endResize(std::optional<WindowId> window);

    void refresh(bool isActive);
    QString checkConsistency() const;
    QString verifyArea() const;
    QString verifyColumns() const;

private:
    struct Location
    {
        std::size_t column;
        std::size_t tile;
    };

    std::optional<Location> locate(WindowId id) const;
    std::size_t columnIndexOf(WindowId id) const;
    Location targetLocation(std::optional<WindowId> window) const;
    double fitScroll(std::optional<double> targetX, double colX, double width, WindowMode mode) const;
    double centeredScroll(std::optional<double> targetX, double colX, double width, WindowMode mode) const;
    double fitScrollForColumn(std::optional<double> targetX, std::size_t idx) const;
    double centeredScrollForColumn(std::optional<double> targetX, std::size_t idx) const;
    double scrollForColumn(std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> prevIndex) const;
    double overflowScrollForColumn(std::optional<double> targetX, std::size_t idx, std::size_t prevIndex) const;
    void toggleFillWidthAt(std::size_t idx);
    void reconcileView();
    std::optional<double> overlayViewOffset() const;
    double constrainScrollPosition(double scrollPosition) const;
    static std::optional<std::optional<Config::PresetSize>> ruleWidthFor(const Column &column);
    void animateScroll(std::size_t idx, double newViewOffset);
    void animateScrollWith(std::size_t idx, double newViewOffset, const Config::AnimationParams &config);
    void scrollToColumnCentered(std::optional<double> targetX, std::size_t idx, const Config::AnimationParams &config);
    void scrollToColumnWith(
        std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> prevIndex, const Config::AnimationParams &config);
    void scrollToColumn(std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> prevIndex);
    void activateColumn(std::size_t idx);
    void selectColumnWith(std::size_t idx, const Config::AnimationParams &config);
    void animateColumnsAfter(std::size_t idx, double offset, const Config::AnimationParams &config, bool inclusiveBefore);
    void moveColumnTo(std::size_t requestedIndex);
    void cancelResizeForColumn(Column &column);
    void consumeOrExpelWindow(std::optional<WindowId> window, ScrollDirection direction);
    void consumeLeftIntoAdjacent(Location source, QPointF prevOff, bool sourceWasActive);
    void expelLeft(Location source, QPointF prevOff, bool sourceWasActive);
    void consumeRightIntoAdjacent(Location source, QPointF prevOff, bool sourceWasActive);
    void expelRight(Location source, QPointF prevOff, bool sourceWasActive);
    void swapTiles(std::size_t sourceColumnIndex, std::size_t targetColumnIndex, ScrollDirection direction);
    void updateActiveColumnViewAfterUpdate(
        std::size_t colIndex, std::optional<quint8> resizeEdges, double offset, bool wasNormal, bool ongoingResizeAnim);
    void moveOtherColumnsForResize(std::size_t colIndex, double offset, bool ongoingResizeAnim);
    struct VisibleColumns
    {
        double widthTaken = 0;
        std::optional<double> leftmostColX;
        std::optional<double> activeColX;
        bool countedNonActive = false;
    };
    VisibleColumns fullyVisibleColumns() const;
    std::optional<QRectF> rawDropSlotRect(DropSlot position) const;
    std::pair<double, double> inColumnHintGeometry(const Column &column, std::size_t tileIndex) const;
    std::pair<double, double> dndScrollBounds() const;
    std::size_t furthestColumnInDirection(std::size_t snapColIndex, double snapViewPos, bool forward) const;
    bool columnStaysInView(std::size_t idx, double snapViewPos, bool forward) const;
    double columnPadding(const Column &column, QRectF area) const;
    QRectF areaForMode(WindowMode mode) const;
    struct Snap
    {
        double scrollPosition;
        std::size_t colIndex;
    };
    std::vector<Snap> centeredSnapPoints() const;
    std::vector<Snap> edgeSnapPoints() const;
    Snap closestSnapPoint(double targetScrollPosition) const;
    std::pair<double, double> columnSnapPoints(double colX, std::size_t idx) const;

    std::vector<Column> m_columns;
    bool m_defaultWidthsPending = false;
    std::size_t m_activeColumnIndex = 0;
    std::optional<ResizeSession> m_resize;
    StripScroll m_scroll;
    struct ReturnColumn
    {
        double viewOffset = 0.0;
        bool onRight = false;
    };

    std::optional<ReturnColumn> m_returnColumnOnClose;
    std::optional<double> m_scrollToRestore;
    AreaInfo m_area;
    Anim::Clock m_clock;
    OptionsPtr m_options;
};

}
