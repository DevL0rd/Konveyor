#pragma once

#include "layout/monitor/monitor.h"

#include <QHash>

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace Konveyor::Layout
{

QString monitorProfileName(const Config::Config &config, const OutputArea &area);

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
    QString viewGestureOutput;
    QString switchGestureOutput;
    std::optional<WindowId> resizeWindow;
    bool overviewOpen = false;
    Anim::Duration startupTime;
    WindowMemory windowMemory;

    bool atStartup() const;
    OutputArea areaFor(const OutputInfo &info) const;
    QPointF originOf(const QString &outputName) const;

    Monitor *activeMonitor();
    Workspace *activeWorkspace();
    Monitor *monitorByName(const QString &name);
    std::optional<std::size_t> monitorIndexByName(const QString &name) const;
    Monitor *monitorOf(WindowId id);
    Workspace *workspaceOf(WindowId id);
    Workspace *workspaceById(WorkspaceId id);
    std::optional<std::size_t> monitorIndexOf(WindowId id) const;
    std::vector<Workspace *> allWorkspaces();
    void rememberWindows();
    void applyRememberedSize(NewWindowPlan &plan, const QString &appId) const;
    bool appHasWindow(const QString &appId) const;
    bool placeInAppGroup(Tile &tile, const NewWindowPlan &plan, Workspace &workspace, MonitorAddRequest &request);
    const Config::Layout &layoutForMonitor(std::size_t monitorIndex) const;
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
    bool focusWindow(WindowId id);
    void updateFocus();
    void resolveRules();
    bool resolveWorkspaceRules(Workspace &workspace, std::vector<WindowId> &changed);
    void ensureNamedWorkspaces();
    void applyOptions();

    NewWindowPlan planNewWindow(const WindowProperties &properties, const QString &preferredOutput, ActivationPolicy policy) const;
    std::optional<std::size_t> monitorForNewWindow(
        const NewWindowPlan &plan, const WindowProperties &properties, const QString &preferredOutput) const;
    Workspace *workspaceForNewWindow(NewWindowPlan &plan);
    Workspace *workspaceForPlacement(const NewWindowPlan &plan);
    void finishPlacement(WindowId id, const NewWindowPlan &plan);
    void placeNewWindow(
        WindowId id, const WindowProperties &properties, const NewWindowPlan &plan, const std::optional<RestorePlacement> &restore);
    bool placeRestored(Tile &tile, const NewWindowPlan &plan, const RestorePlacement &restore, MonitorAddRequest &request);

    std::optional<std::size_t> monitorInDirection(const QString &direction) const;

    void moveWindowToMonitor(std::optional<WindowId> window, std::size_t monitorIndex, bool activate);
    void moveColumnToMonitor(std::size_t monitorIndex, bool activate);
    void moveWorkspaceToMonitor(std::size_t monitorIndex);

    void appendWorkspaceStates(QList<WindowState> &states, const Workspace &workspace, const WorkspaceRenderContext &context) const;
    std::optional<QRectF> dropHintRect(const Monitor &monitor) const;
    void appendMovedWindowState(QList<WindowState> &states) const;

    void beginInteractiveMoving(const QString &output);
    void updateDropHint();
    void interactiveMoveFinish();
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
