#pragma once

#include "layout/floating/floatinglayer.h"
#include "layout/strip/columnstrip.h"

#include <QString>

#include <cstddef>
#include <optional>
#include <vector>

namespace Konveyor::Layout
{

struct OutputArea
{
    QString outputName;
    QString outputId;
    QSizeF viewSize {1280, 720};
    QRectF workingArea {0, 0, 1280, 720};
    double scale = 1.0;
    bool operator==(const OutputArea &) const = default;
};

struct AddTarget
{
    enum class Kind
    {
        Auto,
        NewColumnAt,
        NextTo
    };
    Kind kind = Kind::Auto;
    std::size_t columnIndex = 0;
    WindowId nextTo = 0;

    static AddTarget atColumn(std::size_t index) { return {Kind::NewColumnAt, index, 0}; }
    static AddTarget besideWindow(WindowId id) { return {Kind::NextTo, 0, id}; }
};

struct AddTileRequest
{
    AddTarget target;
    Activation activate = Activation::Smart;
    ColumnWidth width;
    bool fillsWidth = false;
    bool isFloating = false;
    std::optional<Config::AnimationParams> anim;
};

class Workspace
{
public:
    Workspace(const OutputArea &area, const Anim::Clock &clock, OptionsPtr globalOptions, std::optional<Config::NamedWorkspace> config);

    WorkspaceId id() const { return m_id; }
    const QString &name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    void unname() { m_name.clear(); }
    bool hasWindows() const;
    bool isOccupiedOrNamed() const;
    bool hasWindow(WindowId id) const;
    bool isUrgent() const;

    const QString &homeOutput() const { return m_homeOutput; }
    void setHomeOutput(const QString &outputId) { m_homeOutput = outputId; }
    const QString &outputName() const { return m_area.outputName; }
    bool hasOutput() const { return !m_area.outputName.isEmpty(); }
    const OutputArea &area() const { return m_area; }
    void setOutput(const OutputArea &area);
    void clearOutput();

    void updateConfig(OptionsPtr globalOptions);
    const OptionsPtr &options() const { return m_options; }
    const std::optional<Config::Layout> &layoutOverride() const { return m_layoutOverride; }
    const Anim::Clock &clock() const { return m_clock; }

    void tickAnimations();
    bool isAnimating() const;

    ColumnStrip &scrolling() { return m_strip; }
    const ColumnStrip &scrolling() const { return m_strip; }
    bool defaultWidthsPending() const { return m_strip.defaultWidthsPending(); }
    void applyDefaultColumnWidths() { m_strip.applyDefaultColumnWidths(); }
    FloatingLayer &floating() { return m_floating; }
    const FloatingLayer &floating() const { return m_floating; }

    Tile createTile(LayoutWindow window) const;
    Tile *tileFor(WindowId id);
    void addTile(Tile tile, const AddTileRequest &request);
    void insertIntoColumn(std::size_t columnIndex, std::optional<std::size_t> tileIndex, Tile tile, bool activate);
    void addColumn(Column column, bool activate, std::optional<Config::AnimationParams> anim);
    DetachedTile removeTile(WindowId id);
    std::optional<Column> removeActiveColumn();

    std::optional<WindowId> activeWindow() const;
    bool activateWindow(WindowId id);
    bool isFloating(WindowId id) const;
    bool isFloatingFocused() const;
    bool showsFloating() const;
    bool activeTileWantsFullscreen() const;
    bool animateOpening(WindowId id);
    void updateWindow(WindowId id);
    bool childrenAdded(WindowId id);

    std::optional<Config::PresetSize> defaultWidthFor(const std::optional<std::optional<Config::PresetSize>> &rule, bool isFloating) const;
    std::optional<Config::PresetSize> defaultHeightFor(const std::optional<std::optional<Config::PresetSize>> &rule, bool isFloating) const;
    QSize initialWindowSize(const std::optional<Config::PresetSize> &width, const std::optional<Config::PresetSize> &height,
        bool isFloating, const EffectiveWindowRules &rules, QSize minSize, QSize maxSize) const;
    ColumnWidth tiledWidthFor(const LayoutWindow &window, const std::optional<Config::PresetSize> &width) const;

    bool focusLeft();
    bool focusRight();
    void focusColumnFirst();
    void focusColumnLast();
    void focusColumnRightOrFirst();
    void focusColumnLeftOrLast();
    void focusColumn(std::size_t index);
    void focusWindowInColumn(std::size_t index);
    bool focusDown();
    bool focusUp();
    void focusDownOrLeft();
    void focusDownOrRight();
    void focusUpOrLeft();
    void focusUpOrRight();
    void focusWindowTop();
    void focusWindowBottom();
    void focusWindowDownOrTop();
    void focusWindowUpOrBottom();

    bool moveLeft();
    bool moveRight();
    void moveColumnToFirst();
    void moveColumnToLast();
    void moveColumnToIndex(std::size_t index);
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

    void toggleWidth(bool forwards);
    void toggleFillWidth();
    void toggleFillWidthFor(WindowId window);
    void setColumnWidth(SizeChange change);
    void setWindowWidth(std::optional<WindowId> window, SizeChange change);
    void setWindowHeight(std::optional<WindowId> window, SizeChange change);
    void resetWindowHeight(std::optional<WindowId> window);
    void toggleWindowWidth(std::optional<WindowId> window, bool forwards);
    void toggleWindowHeight(std::optional<WindowId> window, bool forwards);
    void expandColumnToAvailableWidth();

    void setFullscreen(WindowId window, bool fullscreen);
    void toggleFullscreen(WindowId window);
    void setMaximized(WindowId window, bool maximize);
    void cycleExpansion(WindowId window);
    void toggleMaximized(WindowId window);
    void toggleWindowFloating(std::optional<WindowId> window);
    void placeWindowFloating(std::optional<WindowId> window, bool floating);
    void focusFloating();
    void focusTiling();
    void switchFocusFloatingTiling();
    void moveFloatingWindow(std::optional<WindowId> window, PositionChange x, PositionChange y, bool animate);

    std::vector<ConstTileRef> tilesWithRenderPositions() const;
    std::vector<ConstTileRef> placedTiles(bool animated) const;
    std::vector<TileRef> renderedTilesMut(bool round);
    std::optional<WindowId> windowUnder(QPointF pos) const;
    std::optional<QPointF> tileRenderPosition(WindowId id) const;
    DropSlot tiledDropSlotAt(QPointF pos, std::optional<Config::ColumnPosition> movingPin) const;
    std::optional<QRectF> dropSlotRect(DropSlot position) const;

    void beginSwipe(bool isTouchpad);
    std::optional<bool> updateSwipe(double deltaX, Anim::Duration timestamp, bool isTouchpad);
    bool endSwipe(std::optional<bool> isTouchpad, std::optional<WindowId> keepActive = std::nullopt);
    void beginEdgeScroll();
    bool edgeScrollBy(QPointF pos, double speed);
    void endEdgeScroll();

    bool beginResize(WindowId window, quint8 edges);
    bool updateResize(WindowId window, QPointF delta);
    void endResize(std::optional<WindowId> window);

    void refresh(bool isActive);
    QString checkConsistency() const;

private:
    enum class FloatingFocus
    {
        No,
        NoButRaised,
        Yes
    };

    bool targetIsFloating(std::optional<WindowId> window) const;
    void applyOptions();
    void addTileAuto(Tile tile, const AddTileRequest &request);
    void addTileNextTo(Tile tile, const AddTileRequest &request);
    void addTileFloatingNextTo(Tile tile, const AddTileRequest &request, bool activate);
    void updateFocusAfterRemoving(bool removedFromFloating);
    void moveTileToFloating(WindowId window, bool activate, QPointF renderPos);
    void setSizingMode(WindowId window, bool enable, bool maximize);
    bool shouldRestoreToFloating(WindowId window, bool maximize) const;
    void rememberRestoreToFloating(WindowId window, bool wasNormal, bool restore);
    void storeFloatingPosition(std::optional<WindowId> window, PositionChange x, PositionChange y);

    QString m_homeOutput;
    QString m_name;
    OutputArea m_area;
    Anim::Clock m_clock;
    OptionsPtr m_globalOptions;
    std::optional<Config::Layout> m_layoutOverride;
    OptionsPtr m_options;
    ColumnStrip m_strip;
    FloatingLayer m_floating;
    FloatingFocus m_floatingFocus = FloatingFocus::No;
    WorkspaceId m_id;
};

bool outputMatches(const OutputArea &area, const QString &reference);

}
