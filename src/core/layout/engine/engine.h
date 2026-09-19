#pragma once

#include "config/types.h"
#include "layout/engine/windowmemory.h"

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>

#include <functional>
#include <memory>
#include <optional>

namespace Konveyor::Anim
{
class Clock;
}

namespace Konveyor::Layout
{

using WindowId = quint64;
using WorkspaceId = quint64;

struct OutputInfo
{
    QString name;
    QString makeModelSerial;
    QRectF geometry;
    QRectF workArea;
    double scale = 1.0;
    bool operator==(const OutputInfo &) const = default;
};

struct WindowProperties
{
    QString appId;
    QString title;
    QSizeF minSize;
    QSizeF maxSize;
    std::optional<WindowId> parent;
    bool isUrgent = false;
    bool isDialog = false;
    bool isResizable = true;
    bool wantsFullscreen = false;
    bool wantsMaximized = false;
    QSizeF frameSize;
    bool operator==(const WindowProperties &) const = default;
};

enum class ActivationPolicy
{
    Smart,
    Focus,
    NoFocus
};

enum class WindowMode
{
    Normal,
    Maximized,
    Fullscreen
};

enum class ResizeEdge : quint8
{
    None = 0,
    Top = 1,
    Bottom = 2,
    Left = 4,
    Right = 8
};

struct ResolvedPaint
{
    Config::ColorSource source = Config::ColorSource::Explicit;
    QColor color;
    std::optional<Config::Gradient> gradient;
    bool operator==(const ResolvedPaint &) const = default;
};

struct DecorationState
{
    bool enabled = false;
    double width = 0;
    ResolvedPaint paint;
    bool operator==(const DecorationState &) const = default;
};

struct TabBarState
{
    bool visible = false;
    QRectF rect;
    QList<QRectF> tabRects;
    QList<ResolvedPaint> tabPaints;
    bool operator==(const TabBarState &) const = default;
};

struct WindowState
{
    WindowId id = 0;
    QString output;
    WorkspaceId workspace = 0;
    int workspaceIndex = 0;
    QRectF targetFrame;
    QRectF renderFrame;
    double renderAlpha = 1.0;
    double ruleOpacity = 1.0;
    bool visible = true;
    bool onActiveWorkspace = true;
    bool isFloating = false;
    bool isForceResizable = false;
    bool isForceResizableByRule = false;
    bool isExpansionForceResizable = false;
    bool isActive = false;
    bool isFocused = false;
    bool isUrgent = false;
    WindowMode sizingMode = WindowMode::Normal;
    WindowMode requestedSizingMode = WindowMode::Normal;
    bool isWindowedFullscreen = false;
    int stackingIndex = 0;
    std::optional<QSizeF> requestedMinSize;
    std::optional<QSizeF> requestedMaxSize;
    DecorationState focusRing;
    DecorationState border;
    Config::CornerRadius cornerRadius;
    bool clipToGeometry = false;
    TabBarState tabBar;
    int columnIndex = 0;
    int tileIndex = 0;
    std::optional<int> widthPresetIndex;
    std::optional<int> nativeWidthSuccessorIndex;
    int widthPresetCount = 0;
    bool operator==(const WindowState &) const = default;
};

struct RestorePlacement
{
    WorkspaceId workspace = 0;
    bool isFloating = false;
    std::size_t columnIndex = 0;
    std::optional<std::size_t> tileIndex;
    ColumnWidth width;
    QRectF floatingFrame;
};

struct WorkspaceState
{
    WorkspaceId id = 0;
    int index = 0;
    QString name;
    QString output;
    bool isActive = false;
    bool isFocused = false;
    bool isUrgent = false;
    std::optional<WindowId> activeWindow;
    bool operator==(const WorkspaceState &) const = default;
};

struct OutputState
{
    QString name;
    double transitionProgress = 0;
    std::optional<QRectF> dropHint;
    ResolvedPaint dropHintPaint;
    QColor backgroundColor;
    bool operator==(const OutputState &) const = default;
};

struct ActionResult
{
    bool ok = true;
    QString error;
};

struct Hooks
{
    std::function<void(WindowId)> closeWindow;
    std::function<void(const QString &)> spawn;
    std::function<void()> toggleOverview;
    std::function<void(bool)> setOverviewOpen;
    std::function<void(const QString &)> compositorAction;
    std::function<void(WindowId)> focusWindow;
    std::function<void()> windowMemoryChanged;
};

class Engine
{
public:
    explicit Engine(Anim::Clock &clock, Hooks hooks = {});
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void setConfig(const Config::Config &config);

    void addOutput(const OutputInfo &output);
    void updateOutput(const OutputInfo &output);
    void removeOutput(const QString &name);
    void focusOutput(const QString &name);
    std::optional<QString> focusedOutput() const;

    void addWindow(WindowId id, const WindowProperties &properties, const QString &preferredOutput, ActivationPolicy policy,
        const std::optional<RestorePlacement> &restore = std::nullopt);
    std::optional<RestorePlacement> placementOf(WindowId id) const;
    void removeWindow(WindowId id);
    void updateWindowProperties(WindowId id, const WindowProperties &properties);
    void windowSizeCommitted(WindowId id, const QSizeF &frameSize);
    void setFloatingFrame(WindowId id, const QRectF &frame);
    void activateWindow(WindowId id);
    void setLayoutFocused(bool focused);
    void setWindowFullscreen(WindowId id, bool fullscreen);
    void toggleWindowFillWidth(WindowId id);
    void setWindowUrgent(WindowId id, bool urgent);
    bool hasWindow(WindowId id) const;
    bool wantsWindow(const WindowProperties &properties) const;

    ActionResult perform(const Config::Action &action, std::optional<WindowId> target = {});

    void beginSwipe(const QString &output, bool isTouchpad);
    void updateSwipe(double delta, qint64 timestampMs, bool isTouchpad);
    void endSwipe(std::optional<bool> isTouchpad, std::optional<WindowId> keepActive = std::nullopt);
    void beginWorkspaceSwipe(const QString &output, bool isTouchpad);
    void updateWorkspaceSwipe(double delta, qint64 timestampMs, bool isTouchpad);
    void endWorkspaceSwipe(std::optional<bool> isTouchpad);

    bool beginWindowDrag(WindowId id, const QPointF &pointer);
    void updateWindowDrag(const QPointF &pointer, const QString &output);
    void toggleWindowDragFloating();
    void endWindowDrag();
    bool beginResize(WindowId id, quint8 edges);
    void updateResize(const QPointF &delta);
    void endResize();
    void beginDataDrag();
    void dataDragEdgeScroll(const QString &output, const QPointF &pointer, qint64 timestampMs);
    void endDataDrag();

    void tickAnimations();
    bool isAnimating() const;

    QList<WindowState> windowStates() const;
    std::optional<WindowState> windowState(WindowId id) const;
    QList<WorkspaceState> workspaceStates() const;
    QList<OutputState> outputStates() const;
    std::optional<WindowId> focusedWindow() const;
    std::optional<WindowId> windowAt(const QPointF &globalPos) const;

    void focusWorkspace(const QString &output, int index);
    void setWindowMemory(const WindowMemory &memory);
    const WindowMemory &windowMemory() const;
    bool isOverviewOpen() const;
    void setOverviewOpen(bool open);

    QString checkConsistency() const;

    struct Private;

private:
    std::unique_ptr<Private> d;
};

}
