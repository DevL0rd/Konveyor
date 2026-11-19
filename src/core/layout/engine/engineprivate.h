#pragma once

#include <algorithm>

#include "layout/monitor/monitor.h"

#include <QHash>

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace Konveyor::Layout
{

QString monitorProfileName(const Config::Config &config, const OutputArea &area);

inline const OverviewView *viewAt(const QList<OverviewView> &views, const QString &output)
{
    const auto found = std::ranges::find(views, output, &OverviewView::output);
    return found == views.end() ? nullptr : &*found;
}

struct DragOrigin
{
    RestorePlacement placement;
    QString output;
    std::size_t workspaceIndex = 0;
    WindowMode mode = WindowMode::Normal;
};

struct WindowDrag
{
    bool moving = false;
    WindowId window = 0;
    QPointF pointerDelta;
    QPointF pointerRatio;
    QPointF lastPointer;
    std::optional<Tile> tile;
    QString output;
    QPointF pointerPos;
    ColumnWidth width;
    bool fillsWidth = false;
    bool isFloating = false;
    DragOrigin origin;

    QPointF renderLocation() const;
};

struct NewWindowPlan
{
    EffectiveWindowRules rules;
    bool isFloating = false;
    bool fillsWidth = false;
    bool wantsFullscreen = false;
    bool wantsMaximized = false;
    std::optional<Config::PresetSize> width;
    std::optional<Config::PresetSize> height;
    std::size_t monitorIndex = 0;
    std::optional<WorkspaceId> workspace;
    std::optional<WindowId> parent;
    std::optional<QSize> nativeSize;
    Activation activate = Activation::Smart;
};

struct WorkspaceRenderContext
{
    QPointF origin;
    double renderY = 0;
    double targetY = 0;
    QString output;
    int index = 0;
    bool onActive = false;
    bool visible = true;
    // Ring opacity override while a workspace transition runs; negative means none.
    double ringOpacity = -1.0;
};

struct OverviewDrop
{
    WorkspaceId workspace = 0;
    // Pointer position inside the target workspace, in workspace-local logical units.
    QPointF local;
};

struct OverviewPress
{
    WindowId window = 0;
    QString output;
    // Zero-based card index, or -1 when the press landed on a window.
    int workspaceIndex = -1;
    int cardIndex = -1;
    QPointF origin;
    bool dragging = false;
};

// A camera move that plays instead of snapping the board to a revealed window.
struct OverviewCameraPan
{
    OverviewCameraPan(const Anim::Clock &clock, const QPointF &from, const QPointF &to, const Config::AnimationParams &params)
        : from(from)
        , to(to)
        , progress(clock, 0.0, 1.0, 0.0, params)
    { }

    QPointF position() const { return from + (to - from) * progress.valueWithoutOvershoot(); }

    QPointF from;
    QPointF to;
    Anim::Animation progress;
};

struct OverviewRuntime
{
    std::optional<Anim::Animation> progressAnimation;
    double progress = 0.0;
    QHash<QString, QPointF> cameras;
    QHash<QString, Anim::Animation> cameraYAnimations;
    QHash<QString, OverviewCameraPan> cameraPans;
    QHash<QString, QMarginsF> reserved;
    std::optional<WindowId> selected;
    std::optional<WindowId> dragWindow;
    QString dragOutput;
    QPointF dragPointerRatio;
    QPointF dragPos;
    std::optional<OverviewDrop> drop;
    QString reorderOutput;
    int reorderFrom = -1;
    int reorderTo = -1;
    std::optional<OverviewPress> press;
    double verticalScrollAccum = 0.0;
    double horizontalScrollAccum = 0.0;
    std::optional<Anim::Duration> lastHorizontalEdgeStep;
    std::optional<Anim::Duration> lastVerticalEdgeStep;
};

struct Engine::Private
{
    Private(Anim::Clock &clock, Hooks hooks);

    Anim::Clock clock;
    Hooks hooks;
    Config::Config config;
    OptionsPtr options;
    std::vector<Monitor> monitors;
    std::vector<Workspace> orphanWorkspaces;
    std::size_t activeMonitorIndex = 0;
    QHash<QString, OutputInfo> outputInfos;
    QHash<QString, WorkspaceId> lastActiveWorkspace;
    QHash<WindowId, quint64> focusOrder;
    quint64 focusCounter = 0;
    std::optional<WindowId> focused;
    std::optional<WindowId> announcedFocus;
    bool layoutFocused = true;
    std::optional<WindowDrag> windowDrag;
    std::optional<std::pair<QString, QPointF>> dataDragPointer;
    bool edgeScrolling = false;
    std::optional<WorkspaceId> viewGestureWorkspace;
    QString switchGestureOutput;
    std::optional<WindowId> resizeWindow;
    bool overviewOpen = false;
    OverviewRuntime overview;
    bool nativeDesktopPresentation = false;
    Anim::Duration startupTime;
    bool rulesAtStartup = true;
    WindowMemory windowMemory;

    bool atStartup() const;
    OutputArea areaFor(const OutputInfo &info) const;
    QPointF originOf(const QString &outputName) const;
    bool usesWorkspaceMotion(const Monitor &monitor) const;
    std::vector<double> workspaceRenderOffsets(const Monitor &monitor) const;
    void setOverviewState(bool open);
    bool builtinOverviewOpen() const { return config.overview.enabled && overviewOpen; }
    void startOverviewAnimation(bool open);
    void tickOverview();
    void seedOverviewCamera();
    void revealOverviewSelection();
    void animateOverviewCamera(const QString &output, const QPointF &from, const QPointF &to);
    void stopOverviewCameraPan(const QString &output);
    std::optional<WindowId> centeredOverviewWindow(const OverviewView &view) const;
    void updateOverviewDrop();
    void clearOverviewDrag();

    Monitor *activeMonitor();
    Workspace *activeWorkspace();
    Monitor *monitorByName(const QString &name);
    std::optional<std::size_t> monitorIndexByName(const QString &name) const;
    Monitor *monitorOf(WindowId id);
    Workspace *workspaceOf(WindowId id);
    Tile *tileOf(WindowId id);
    Workspace *workspaceById(WorkspaceId id);
    std::optional<std::size_t> monitorIndexOf(WindowId id) const;
    std::vector<Workspace *> allWorkspaces();
    void rememberWindows();
    void applyRememberedSize(NewWindowPlan &plan, const QString &appId, const Config::Layout &layout) const;
    bool appHasWindow(const QString &appId) const;
    bool placeInAppGroup(Tile &tile, const NewWindowPlan &plan, Workspace &workspace, MonitorAddRequest &request);
    struct AppStackRequest
    {
        bool activate = false;
        std::size_t maxRows = 1;
        bool byPlacement = false;
    };
    void stackIntoColumns(
        std::size_t monitorIndex, Workspace &workspace, const std::vector<std::size_t> &columns, Tile tile, const AppStackRequest &stack);
    void reflowMonitorLayout(Monitor &monitor, const Config::Layout &previous);

    std::optional<WindowId> target(std::optional<WindowId> requested) const;
    Workspace *workspaceForTarget(std::optional<WindowId> requested);

    void refresh();
    void refreshWorkspaces();
    bool focusWindow(WindowId id);
    void takeLayoutFocus();
    void updateFocus();
    bool resolveRules();
    bool resolveWorkspaceRules(Workspace &workspace, std::vector<WindowId> &changed);
    std::size_t namedWorkspaceSlot(const std::vector<Workspace> &workspaces, const QString &name) const;
    void ensureNamedWorkspaces();
    void applyNamedWorkspaceLayouts();
    void applyNamedWorkspaceHomes(const QList<Config::NamedWorkspace> &previous);
    std::optional<std::size_t> homeMonitorOf(const Workspace &workspace) const;
    void sendHome(std::vector<Workspace> workspaces);
    void moveHome(WorkspaceId id);
    void followWindowsOnAllDesktops();
    void forgetRemovedWorkspaceNames(const QList<Config::NamedWorkspace> &previous);
    void applyOptions();

    NewWindowPlan planNewWindow(const WindowProperties &properties, const QString &preferredOutput, ActivationPolicy policy) const;
    std::optional<std::size_t> monitorForNewWindow(
        const NewWindowPlan &plan, const WindowProperties &properties, const QString &preferredOutput) const;
    Workspace *workspaceForNewWindow(NewWindowPlan &plan);
    Workspace *workspaceForPlacement(const NewWindowPlan &plan);
    void restorePresentation(Workspace &workspace, WindowId id, const RestorePlacement &restore);
    void finishPlacement(WindowId id, const NewWindowPlan &plan, const std::optional<RestorePlacement> &restore);
    void placeNewWindow(
        WindowId id, const WindowProperties &properties, const NewWindowPlan &plan, const std::optional<RestorePlacement> &restore);
    bool placeRestored(Tile &tile, const NewWindowPlan &plan, const RestorePlacement &restore, MonitorAddRequest &request);
    QRectF floatingFrameOf(const Workspace &workspace, WindowId id) const;
    void restoreFloatingFrame(WindowId id, const QRectF &frame);
    bool isLockedToOutputSize(WindowId id);
    void presentOverOutput(WindowId id);

    std::optional<std::size_t> monitorInDirection(const QString &direction) const;

    void moveWindowToMonitor(
        std::optional<WindowId> window, std::size_t monitorIndex, bool activate, std::optional<std::size_t> workspaceIndex);
    void moveColumnToMonitor(std::size_t monitorIndex, bool activate, std::optional<std::size_t> workspaceIndex);
    void moveWorkspaceToMonitor(std::size_t monitorIndex);
    std::vector<std::pair<std::size_t, WorkspaceId>> occupiedWorkspacesOutside(std::size_t monitorIndex) const;
    bool activateOnMonitor(std::size_t monitorIndex, const Workspace &workspace);
    void revealWindowOnMonitor(std::size_t monitorIndex, WindowId window, WorkspaceId fallback);
    void moveAllWindowsToMonitor(std::size_t monitorIndex);

    void appendWorkspaceStates(QList<WindowState> &states, const Workspace &workspace, const WorkspaceRenderContext &context) const;
    QList<WindowState> windowStates() const;
    QList<OverviewView> overviewViews() const;
    std::optional<OverviewView> overviewView(const QString &output) const;
    OverviewView overviewView(const Monitor &monitor, const QHash<WorkspaceId, QList<WindowState>> &byWorkspace,
        const std::optional<WindowState> &draggedState) const;
    OverviewGeometryInput overviewGeometryInput(const Monitor &monitor, int activeWorkspace,
        const QHash<WorkspaceId, QList<WindowState>> &byWorkspace, const QPointF &origin, QList<QList<WindowId>> &ids,
        const std::vector<std::size_t> &wsIndices) const;
    void appendOverviewWindows(OverviewView &view, const Monitor &monitor, const OverviewGeometry &geometry,
        const QList<QList<WindowId>> &ids, const QHash<WorkspaceId, QList<WindowState>> &byWorkspace, const QPointF &offset) const;
    void applyOverviewCamera(OverviewView &view, const OverviewGeometryInput &input, const OverviewGeometry &geometry) const;
    void applyOverviewDropState(
        OverviewView &view, const Monitor &monitor, const OverviewGeometry &geometry, const std::vector<std::size_t> &wsIndices) const;
    std::optional<QRectF> overviewDropHintRect(const Monitor &monitor, const QList<QRectF> &cards, int dropWorkspace, double scale) const;
    void appendOverviewDragWindow(OverviewView &view, const WindowState &draggedState, double scale) const;
    void settleOverviewAfterWorkspaceChange(
        const QString &output, const Monitor &monitor, bool workspaceChanged, bool panned, double beforeCameraY);
    bool stepOverviewVertical(Monitor &monitor, double units);
    bool stepOverviewHorizontal(Workspace &workspace, double units);
    Workspace *overviewWorkspaceUnderPointer(Monitor &monitor, const QString &output, const std::optional<QPointF> &pointer);
    std::optional<double> overviewCameraY(const QString &output) const;
    void restartOverviewCameraY(const QString &output, const std::optional<OverviewView> &afterView, double beforeCameraY);
    void focusOverviewMonitor(const QString &output);
    std::optional<QRectF> dropHintRect(const Monitor &monitor) const;
    void appendMovedWindowState(QList<WindowState> &states) const;

    void beginInteractiveMoving(const QString &output);
    void stopEdgeScroll();
    void finishResize();
    void edgeScrollAt(Monitor &monitor, QPointF local);
    bool scrollOverviewWorkspacesAtEdge(
        Monitor &monitor, const OverviewView &view, const QRectF &workArea, double edge, Anim::Duration now);
    bool scrollOverviewColumnsAtEdge(Monitor &monitor, const QRectF &workArea, double edge, Anim::Duration now);
    bool switchOverviewWorkspace(Monitor &monitor, const OverviewView &view, int step);
    void overviewAfterWorkspaceSwipe(WorkspaceId id);
    void scrollDragEdges();
    void continueEdgeScroll();
    void moveDragToActiveOutput();
    void updateDropHint();
    void setDragFloating(bool floating);
    void placeDragPointerOnFloatingTile();
    void interactiveMoveFinish();
    void dropDraggedWindow(std::optional<WindowId> window);
    void returnDraggedTile(WindowDrag &move);
    void dropInteractiveTile(Monitor &monitor, const Monitor::InsertTarget &insertTarget);
};

using ActionHandler = std::function<ActionResult(Engine::Private &, const Config::Action &, std::optional<WindowId>)>;
using ActionTable = QHash<QString, ActionHandler>;

const ActionTable &actionTable();

QString actionArgument(const Config::Action &action, int index);
std::optional<QString> actionProperty(const Config::Action &action, const QString &name);
ActionResult actionError(const QString &message);
std::optional<WindowId> actionWindowId(const Config::Action &action, std::optional<WindowId> fallback);
bool actionFlag(const Config::Action &action, const QString &name, bool fallback);

void addEngineAction(ActionTable &table, const char *name, ActionHandler handler);
void addWorkspaceAction(ActionTable &table, const char *name, void (*fn)(Workspace &));
void addTargetAction(ActionTable &table, const char *name, void (*fn)(Workspace &, std::optional<WindowId>));
void addMonitorAction(ActionTable &table, const char *name, void (*fn)(Monitor &));
void addForwardedAction(ActionTable &table, const char *name);

void registerFocusActions(ActionTable &table);
void registerMoveActions(ActionTable &table);
void registerWorkspaceActions(ActionTable &table);
void registerMonitorActions(ActionTable &table);
void registerSizeActions(ActionTable &table);
void registerWindowActions(ActionTable &table);
void registerCompositorActions(ActionTable &table);

}
