import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import "MenuModel.js" as MenuModel
import "TaskOrder.js" as TaskOrder

PlasmoidItem {
    id: root

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || vertical
    readonly property real thickness: inPanel ? (vertical ? width : height) : Kirigami.Units.iconSizes.large + Kirigami.Units.largeSpacing
    readonly property var shownWorkspaces: TaskOrder.shownWorkspaces(taskbar.workspaces, Plasmoid.configuration.showEmptyWorkspaces)
    readonly property bool showWorkspaces: Plasmoid.configuration.showWorkspaces && taskbar.bus.available && shownWorkspaces.length > 0
    readonly property bool showSeparator: showWorkspaces && Plasmoid.configuration.showSeparator
    readonly property string heldBadges: !Plasmoid.configuration.showShortcutBadges || !taskbar.bus.superHeld ? "" : taskbar.bus.altHeld ? "columns" : "workspaces"
    property string badges: ""

    preferredRepresentation: fullRepresentation
    Plasmoid.constraintHints: Plasmoid.CanFillArea
    Plasmoid.backgroundHints: inPanel ? PlasmaCore.Types.NoBackground : PlasmaCore.Types.DefaultBackground

    TaskLook {
        id: taskLook
        thickness: root.thickness
        vertical: root.vertical
        location: Plasmoid.location
        autoIconSize: Plasmoid.configuration.autoIconSize
        fixedIconSize: Plasmoid.configuration.iconSize
        padding: Plasmoid.configuration.buttonPadding
        spacing: Plasmoid.configuration.iconSpacing
        highlightStyle: Plasmoid.configuration.highlightStyle
        indicatorStyle: Plasmoid.configuration.indicatorStyle
        indicatorOpposite: Plasmoid.configuration.indicatorEdge === 1
        animate: Plasmoid.configuration.animations
        pulse: Plasmoid.configuration.attentionPulse
        tooltips: Plasmoid.configuration.showTooltips
    }

    TaskbarState {
        id: taskbar
        pins: Plasmoid.configuration.launchers
        screenGeometry: Plasmoid.containment ? Plasmoid.containment.screenGeometry : Qt.rect(0, 0, 0, 0)
        groupMode: Plasmoid.configuration.groupMode
        placePinnedLaunches: Plasmoid.configuration.placePinnedLaunches
        onlyThisScreen: Plasmoid.configuration.onlyThisScreen
        showFloating: Plasmoid.configuration.showFloating
        activeClick: Plasmoid.configuration.activeClick
        middleClick: Plasmoid.configuration.middleClick
        onPinsEdited: pins => Plasmoid.configuration.launchers = pins
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
        readonly property real fixedLength: stripLength + (root.showSeparator ? 1 + spacing * 2 : root.showWorkspaces ? spacing : 0)
        readonly property real spacing: Kirigami.Units.smallSpacing

        flow: root.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        layoutDirection: Qt.LeftToRight
        rowSpacing: 0
        columnSpacing: 0
        Layout.fillWidth: !root.vertical
        Layout.fillHeight: root.vertical
        Layout.minimumWidth: root.vertical ? 0 : fixedLength + taskLook.button
        Layout.minimumHeight: root.vertical ? fixedLength + taskLook.button : 0
        Layout.preferredWidth: root.vertical ? root.thickness : fixedLength + tasks.contentWidth
        Layout.preferredHeight: root.vertical ? fixedLength + tasks.contentHeight : root.thickness

        WorkspaceStrip {
            id: workspaces
            visible: root.showWorkspaces
            workspaces: root.shownWorkspaces
            size: root.thickness
            vertical: root.vertical
            badges: taskbar.labels.workspaces
            showBadges: root.badges === "workspaces"
            contentMode: Plasmoid.configuration.pillContent
            animate: taskLook.animate
            wheelSwitches: Plasmoid.configuration.wheelSwitchesWorkspaces
            Layout.alignment: Qt.AlignCenter
            Layout.row: root.vertical ? (Plasmoid.configuration.workspacesAfterTasks ? 2 : 0) : 0
            Layout.column: root.vertical ? 0 : (Plasmoid.configuration.workspacesAfterTasks ? 2 : 0)
            onPicked: workspace => taskbar.switchTo(workspace)
            onStepped: step => taskbar.stepWorkspace(step)
        }

        Item {
            visible: root.showWorkspaces
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
            look: taskLook
            items: taskbar.items
            columnBadges: taskbar.labels.columns
            showBadges: root.badges === "columns"
            wheelCycles: Plasmoid.configuration.wheelCyclesTasks
            Layout.row: root.vertical ? (Plasmoid.configuration.workspacesAfterTasks ? 0 : 2) : 0
            Layout.column: root.vertical ? 0 : (Plasmoid.configuration.workspacesAfterTasks ? 0 : 2)
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
