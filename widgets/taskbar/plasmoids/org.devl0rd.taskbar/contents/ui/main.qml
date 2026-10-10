import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.konveyor.settings
import "MenuModel.js" as MenuModel
import "TaskOrder.js" as TaskOrder

PlasmoidItem {
    id: root

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || vertical
    readonly property real thickness: inPanel ? (vertical ? width : height) : Kirigami.Units.iconSizes.large + Kirigami.Units.largeSpacing
    readonly property var shownWorkspaces: TaskOrder.shownWorkspaces(taskbar.workspaces, TaskbarSettings.values.showEmptyWorkspaces)
    readonly property bool showWorkspaces: TaskbarSettings.values.showWorkspaces && taskbar.bus.available && shownWorkspaces.length > 0
    readonly property bool showTasks: TaskbarSettings.values.showApps || !showWorkspaces
    readonly property bool showSeparator: showWorkspaces && showTasks && TaskbarSettings.values.showSeparator
    readonly property string heldBadges: !TaskbarSettings.values.showShortcutBadges || !taskbar.bus.superHeld ? "" : taskbar.bus.altHeld ? "items" : "workspaces"
    property string badges: ""

    preferredRepresentation: fullRepresentation
    Plasmoid.constraintHints: Plasmoid.CanFillArea
    Plasmoid.backgroundHints: inPanel ? PlasmaCore.Types.NoBackground : PlasmaCore.Types.DefaultBackground

    TaskLook {
        id: taskLook
        thickness: root.thickness
        vertical: root.vertical
        location: Plasmoid.location
        autoIconSize: TaskbarSettings.values.autoIconSize
        fixedIconSize: TaskbarSettings.values.iconSize
        padding: TaskbarSettings.values.buttonPadding
        spacing: TaskbarSettings.values.iconSpacing
        highlightStyle: TaskbarSettings.values.highlightStyle
        indicatorStyle: TaskbarSettings.values.indicatorStyle
        indicatorOpposite: TaskbarSettings.values.indicatorEdge === 1
        animate: TaskbarSettings.values.animations
        pulse: TaskbarSettings.values.attentionPulse
        tooltips: TaskbarSettings.values.showTooltips
    }

    TaskbarState {
        id: taskbar
        pins: TaskbarSettings.values.launchers
        screenGeometry: Plasmoid.containment ? Plasmoid.containment.screenGeometry : Qt.rect(0, 0, 0, 0)
        groupMode: TaskbarSettings.values.groupMode
        placePinnedLaunches: TaskbarSettings.values.placePinnedLaunches
        onlyThisScreen: TaskbarSettings.values.onlyThisScreen
        showFloating: TaskbarSettings.values.showFloating
        activeClick: TaskbarSettings.values.activeClick
        middleClick: TaskbarSettings.values.middleClick
        onPinsEdited: pins => TaskbarSettings.values.launchers = pins
    }

    Timer {
        id: badgeDelay
        interval: 250
        onTriggered: root.badges = root.heldBadges
    }

    onHeldBadgesChanged: {
        if (heldBadges && !badges) {
            badgeDelay.restart()
        } else {
            badgeDelay.stop()
            badges = heldBadges
        }
    }

    function openMenu(entry, window, button) {
        const target = taskbar.targetOf(entry, window)
        taskbar.appActions.appKey = entry.appKey || ""
        taskbar.appActions.pid = target ? target.pid || 0 : 0
        const tr = (text, ...args) => i18n(text, ...args)
        const show = rules => menu.show(MenuModel.entries(taskbar.menuContext(entry, target, rules), tr), entry, target, button)
        if (target && target.konveyorId !== undefined && taskbar.bus.available)
            taskbar.bus.appRules(target.konveyorId, show)
        else
            show(null)
    }

    TaskMenu {
        id: menu
        onActionTriggered: (action, entry, window) => taskbar.run(action, entry, window)
    }

    fullRepresentation: GridLayout {
        id: layout

        readonly property real stripLength: root.showWorkspaces ? (root.vertical ? workspaces.implicitHeight : workspaces.implicitWidth) : 0
        readonly property real fixedLength: stripLength + (root.showSeparator ? 1 + spacing * 2 : root.showWorkspaces && root.showTasks ? spacing : 0)
        readonly property real tasksMinimum: root.showTasks ? taskLook.button : 0
        readonly property real spacing: Kirigami.Units.smallSpacing

        flow: root.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        layoutDirection: Qt.LeftToRight
        rowSpacing: 0
        columnSpacing: 0
        Layout.fillWidth: !root.vertical
        Layout.fillHeight: root.vertical
        Layout.minimumWidth: root.vertical ? 0 : fixedLength + tasksMinimum
        Layout.minimumHeight: root.vertical ? fixedLength + tasksMinimum : 0
        Layout.preferredWidth: root.vertical ? root.thickness : fixedLength + (root.showTasks ? tasks.contentWidth : 0)
        Layout.preferredHeight: root.vertical ? fixedLength + (root.showTasks ? tasks.contentHeight : 0) : root.thickness

        WorkspaceStrip {
            id: workspaces
            visible: root.showWorkspaces
            workspaces: root.shownWorkspaces
            size: root.thickness
            vertical: root.vertical
            badges: taskbar.labels.workspaces
            showBadges: root.badges === "workspaces"
            contentMode: TaskbarSettings.values.pillContent
            animate: taskLook.animate
            wheelSwitches: TaskbarSettings.values.wheelSwitchesWorkspaces
            Layout.alignment: Qt.AlignCenter
            Layout.row: root.vertical ? (TaskbarSettings.values.workspacesAfterTasks ? 2 : 0) : 0
            Layout.column: root.vertical ? 0 : (TaskbarSettings.values.workspacesAfterTasks ? 2 : 0)
            onPicked: workspace => taskbar.switchTo(workspace)
            onStepped: step => taskbar.stepWorkspace(step)
        }

        Item {
            visible: root.showWorkspaces && root.showTasks
            Layout.preferredWidth: root.vertical ? root.thickness : (root.showSeparator ? 1 + layout.spacing * 2 : layout.spacing)
            Layout.preferredHeight: root.vertical ? (root.showSeparator ? 1 + layout.spacing * 2 : layout.spacing) : root.thickness
            Layout.row: root.vertical ? 1 : 0
            Layout.column: root.vertical ? 0 : 1

            Rectangle {
                visible: root.showSeparator
                anchors.centerIn: parent
                color: Kirigami.Theme.textColor
                opacity: 0.18
                width: root.vertical ? root.thickness * 0.5 : 1
                height: root.vertical ? 1 : root.thickness * 0.5
            }
        }

        TaskStrip {
            id: tasks
            visible: root.showTasks
            look: taskLook
            items: taskbar.items
            itemBadges: taskbar.labels.items
            showBadges: root.badges === "items"
            wheelCycles: TaskbarSettings.values.wheelCyclesTasks
            Layout.row: root.vertical ? (TaskbarSettings.values.workspacesAfterTasks ? 0 : 2) : 0
            Layout.column: root.vertical ? 0 : (TaskbarSettings.values.workspacesAfterTasks ? 0 : 2)
            Layout.fillWidth: !root.vertical
            Layout.fillHeight: root.vertical
            Layout.preferredWidth: root.vertical ? root.thickness : contentWidth
            Layout.preferredHeight: root.vertical ? contentHeight : root.thickness
            Layout.maximumWidth: root.vertical ? root.thickness : Math.max(taskLook.button, contentWidth)
            Layout.maximumHeight: root.vertical ? Math.max(taskLook.button, contentHeight) : root.thickness
            Layout.minimumWidth: root.vertical ? root.thickness : taskLook.button
            Layout.minimumHeight: root.vertical ? taskLook.button : root.thickness
            onItemActivated: (entry, window, button) => {
                if (taskbar.click(entry, window) === "launch")
                    button.bounce()
            }
            onItemMiddleClicked: (entry, window) => taskbar.middle(entry, window)
            onMenuRequested: (entry, window, button) => root.openMenu(entry, window, button)
            onWindowPicked: window => taskbar.source.activate(window)
            onStepped: step => taskbar.step(step)
            onReordered: keys => {
                if (taskbar.reorder(keys) > 0)
                    releaseLater.restart()
                else
                    release()
            }

            Timer {
                id: releaseLater
                interval: 1500
                onTriggered: tasks.release()
            }

            Connections {
                target: taskbar.bus
                function onLayoutRefreshed() {
                    if (releaseLater.running) {
                        releaseLater.stop()
                        tasks.release()
                    }
                }
            }
        }
    }
}
