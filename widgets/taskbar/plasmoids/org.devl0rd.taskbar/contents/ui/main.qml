import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || vertical
    readonly property real thickness: inPanel ? (vertical ? width : height) : Kirigami.Units.iconSizes.large + Kirigami.Units.largeSpacing
    readonly property bool showWorkspaces: Plasmoid.configuration.showWorkspaces && taskbar.bus.available && taskbar.workspaces.length > 0
    property string badges: ""
    readonly property string heldBadges: !Plasmoid.configuration.showShortcutBadges || !taskbar.bus.superHeld ? "" : taskbar.bus.altHeld ? "columns" : "workspaces"

    preferredRepresentation: fullRepresentation
    Plasmoid.constraintHints: Plasmoid.CanFillArea
    Plasmoid.backgroundHints: inPanel ? PlasmaCore.Types.NoBackground : PlasmaCore.Types.DefaultBackground

    TaskbarState {
        id: taskbar
        pins: Plasmoid.configuration.launchers
        screenGeometry: Plasmoid.containment ? Plasmoid.containment.screenGeometry : Qt.rect(0, 0, 0, 0)
        groupMode: Plasmoid.configuration.groupMode
        placePinnedLaunches: Plasmoid.configuration.placePinnedLaunches
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

    TaskMenu {
        id: menu
        onWindowPicked: window => taskbar.source.activate(window)
        onNewInstance: item => taskbar.newInstance(item)
        onPinToggled: item => taskbar.togglePin(item)
        onCloseAll: item => taskbar.closeAll(item)
    }

    fullRepresentation: GridLayout {
        id: layout

        flow: root.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rowSpacing: Kirigami.Units.smallSpacing
        columnSpacing: Kirigami.Units.smallSpacing
        Layout.fillWidth: !root.vertical
        Layout.fillHeight: root.vertical
        Layout.minimumWidth: root.vertical ? 0 : (root.showWorkspaces ? workspaces.implicitWidth + columnSpacing * 2 : 0) + root.thickness
        Layout.minimumHeight: root.vertical ? (root.showWorkspaces ? workspaces.implicitHeight + rowSpacing * 2 : 0) + root.thickness : 0
        Layout.preferredWidth: root.vertical ? root.thickness : Layout.minimumWidth + tasks.contentWidth
        Layout.preferredHeight: root.vertical ? Layout.minimumHeight + tasks.contentHeight : root.thickness

        WorkspaceStrip {
            id: workspaces
            visible: root.showWorkspaces
            workspaces: taskbar.workspaces
            size: root.thickness
            vertical: root.vertical
            badges: taskbar.labels.workspaces
            showBadges: root.badges === "workspaces"
            Layout.alignment: Qt.AlignCenter
            onPicked: workspace => taskbar.switchTo(workspace)
            onStepped: step => taskbar.step(step)
        }

        Rectangle {
            visible: root.showWorkspaces
            color: Kirigami.Theme.textColor
            opacity: 0.18
            Layout.preferredWidth: root.vertical ? root.thickness * 0.5 : 1
            Layout.preferredHeight: root.vertical ? 1 : root.thickness * 0.5
            Layout.alignment: Qt.AlignCenter
        }

        TaskStrip {
            id: tasks
            items: taskbar.items
            size: root.thickness
            vertical: root.vertical
            edge: Plasmoid.location
            columnBadges: taskbar.labels.columns
            showBadges: root.badges === "columns"
            Layout.fillWidth: !root.vertical
            Layout.fillHeight: root.vertical
            Layout.preferredWidth: root.vertical ? root.thickness : contentWidth
            Layout.preferredHeight: root.vertical ? contentHeight : root.thickness
            onItemActivated: (item, button) => {
                if (item.windows.length === 0)
                    button.bounce()
                taskbar.activate(item)
            }
            onItemClosed: item => taskbar.close(item)
            onMenuRequested: (item, button) => menu.show(item, button)
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
