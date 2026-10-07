import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.konveyor.settings

KCM.AbstractKCM {
    id: root

    implicitWidth: Kirigami.Units.gridUnit * 52
    implicitHeight: Kirigami.Units.gridUnit * 36
    framedView: false

    function openAppRule() {
        const path = kcm.initialApp !== "" ? RuleSummary.appRule(SettingsStore.children("", "window-rule"), kcm.initialApp) : ""
        if (path !== "")
            SettingsNavigation.push("pages/RuleEditorPage.qml", { rulePath: path })
    }

    Component.onCompleted: Qt.callLater(openAppRule)

    Connections {
        target: kcm
        function onInitialPageChanged() {
            if (kcm.initialPage !== "")
                view.pageId = kcm.initialPage
            Qt.callLater(root.openAppRule)
        }
    }
    Shortcut {
        sequences: [StandardKey.Undo]
        onActivated: SettingsStore.undo()
    }

    SettingsView {
        id: view
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        pageId: kcm.initialPage !== "" ? kcm.initialPage : Pages.pages[0].id
        onPageChosen: id => view.pageId = id
    }
}
