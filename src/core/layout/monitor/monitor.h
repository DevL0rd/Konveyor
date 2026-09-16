#pragma once

#include "layout/monitor/workspacetransition.h"
#include "layout/workspace/workspace.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace Konveyor::Layout
{

struct MonitorAddTarget
{
    enum class Kind
    {
        Auto,
        Workspace,
        NextTo
    };
    Kind kind = Kind::Auto;
    WorkspaceId workspace = 0;
    std::optional<std::size_t> columnIndex;
    WindowId window = 0;

    static MonitorAddTarget onWorkspace(WorkspaceId id, std::optional<std::size_t> columnIndex = {});
    static MonitorAddTarget besideWindow(WindowId id);
};

struct MonitorAddRequest
{
    MonitorAddTarget target;
    Activation activate = Activation::Smart;
    bool allowActivateWorkspace = true;
    ColumnWidth width;
    bool fillsWidth = false;
    bool isFloating = false;
    std::optional<Config::AnimationParams> anim;
};

struct DropHint
{
    DropWorkspace workspace;
    DropSlot position;
    bool operator==(const DropHint &) const = default;
};

class Monitor
{
public:
    Monitor(const OutputArea &area, std::vector<Workspace> workspaces, std::optional<WorkspaceId> toActivate, const Anim::Clock &clock,
        OptionsPtr globalOptions, std::optional<Config::Layout> layoutOverride);

    std::vector<Workspace> releaseWorkspaces();

    const QString &outputName() const { return m_area.outputName; }
    const OutputArea &area() const { return m_area; }
    void setArea(const OutputArea &area);
    void updateConfig(OptionsPtr globalOptions);
    void setLayoutOverride(std::optional<Config::Layout> layoutOverride);
    const std::optional<Config::Layout> &layoutOverride() const { return m_layoutOverride; }
    const OptionsPtr &options() const { return m_options; }

    std::vector<Workspace> &workspaces() { return m_workspaces; }
    const std::vector<Workspace> &workspaces() const { return m_workspaces; }
    std::size_t activeWorkspaceIndex() const { return m_activeWorkspaceIndex; }
    Workspace &activeWorkspace() { return m_workspaces[m_activeWorkspaceIndex]; }
    const Workspace &activeWorkspace() const { return m_workspaces[m_activeWorkspaceIndex]; }
    std::optional<std::size_t> indexOfWorkspace(WorkspaceId id) const;
    std::optional<std::size_t> workspaceNamed(const QString &name) const;
    std::optional<std::size_t> workspaceOfWindow(WindowId id) const;
    bool hasWindow(WindowId id) const;

    void insertEmptyWorkspace(std::size_t idx);
    void prependEmptyWorkspace();
    void appendEmptyWorkspace();
    void activateWorkspace(std::size_t idx);
    void selectWorkspaceWith(std::size_t idx, const std::optional<Config::AnimationParams> &config);
    void pruneWorkspaces();
    bool clearWorkspaceName(WorkspaceId id);
    Workspace detachWorkspaceAt(std::size_t idx);
    void insertWorkspace(Workspace workspace, std::size_t idx, bool activate);
    void appendWorkspaces(std::vector<Workspace> workspaces);
    std::vector<Workspace> takeWorkspacesForOutput(const OutputArea &target);

    void addTile(Tile tile, const MonitorAddRequest &request);
    void insertIntoColumn(std::size_t workspaceIndex, std::size_t columnIndex, std::optional<std::size_t> tileIndex, Tile tile,
        bool activate, bool allowActivateWorkspace);
    void addColumn(std::size_t workspaceIndex, Column column, bool activate, std::optional<Config::AnimationParams> anim);

    void moveDownOrToWorkspaceDown();
    void moveUpOrToWorkspaceUp();
    void focusWindowOrWorkspaceDown();
    void focusWindowOrWorkspaceUp();
    void moveToWorkspaceUp(Activation activate);
    void moveToWorkspaceDown(Activation activate);
    void moveToWorkspace(std::optional<WindowId> window, std::size_t idx, Activation activate);
    void moveColumnToWorkspaceUp(bool activate);
    void moveColumnToWorkspaceDown(bool activate);
    void moveColumnToWorkspace(std::size_t idx, bool activate);
    void switchWorkspaceUp();
    void switchWorkspaceDown();
    void switchWorkspace(std::size_t idx);
    void goToWorkspaceOrBack(std::size_t idx);
    void switchWorkspacePrevious();
    void moveWorkspaceDown();
    void moveWorkspaceUp();
    void moveWorkspaceToIndex(std::size_t oldIndex, std::size_t newIndex);

    void tickAnimations();
    bool isAnimating() const;
    void refresh(bool isActive);

    double visibleWorkspacePosition() const;
    double workspaceHeightWithGap() const;
    std::vector<double> workspaceRenderOffsets() const;
    double transitionProgress() const;
    bool isWorkspaceSwitching() const { return m_transition.has_value(); }

    void beginWorkspaceSwipe(bool isTouchpad);
    std::optional<bool> updateWorkspaceSwipe(double deltaY, Anim::Duration timestamp, bool isTouchpad);
    bool endWorkspaceSwipe(std::optional<bool> isTouchpad);
    void beginEdgeScroll();
    bool edgeScrollBy(QPointF pos, double speed);
    void endEdgeScroll();

    struct InsertTarget
    {
        DropWorkspace workspace;
        double workspaceY = 0;
    };
    InsertTarget insertTargetAt(QPointF pos) const;

    std::optional<DropHint> dropHint;
    bool overviewOpen = false;

    QString checkConsistency() const;

private:
    void applyOptions();
    void shiftForInsertion(std::size_t idx);
    void ownWorkspace(std::size_t idx);
    void addEmptyWorkspacesAround(std::size_t &workspaceIndex);
    std::optional<std::size_t> previousWorkspaceIndex() const;
    std::size_t resolveAddWorkspace(const MonitorAddTarget &target, AddTarget &workspaceTarget) const;
    double swipeDistance(const WorkspaceSwipe &gesture) const;
    void animateTileBetweenWorkspaces(
        WindowId window, QPointF oldRenderPos, std::size_t sourceIndex, std::size_t newIndex, const Config::AnimationParams &config);

    OutputArea m_area;
    Anim::Clock m_clock;
    OptionsPtr m_globalOptions;
    std::optional<Config::Layout> m_layoutOverride;
    OptionsPtr m_options;
    std::vector<Workspace> m_workspaces;
    std::size_t m_activeWorkspaceIndex = 0;
    std::optional<WorkspaceId> m_previousWorkspaceId;
    std::optional<WorkspaceTransition> m_transition;
};

}
