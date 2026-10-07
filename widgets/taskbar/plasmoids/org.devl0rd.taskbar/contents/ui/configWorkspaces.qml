import QtQuick
import org.kde.kirigami as Kirigami

ConfigPage {
    id: page

    title: i18n("Workspaces")

    ConfigCheck {
        id: showWorkspaces
        Kirigami.FormData.label: i18n("Workspaces:")
        page: page
        key: "showWorkspaces"
        text: i18n("Show the workspaces of this screen")
    }

    ConfigChoice {
        id: workspacesAfterTasks
        Kirigami.FormData.label: i18n("Position:")
        page: page
        key: "workspacesAfterTasks"
        flag: true
        enabled: showWorkspaces.checked
        model: [i18n("Before the apps"), i18n("After the apps")]
    }

    ConfigChoice {
        id: pillContent
        Kirigami.FormData.label: i18n("Each workspace shows:")
        page: page
        key: "pillContent"
        enabled: showWorkspaces.checked
        model: [i18n("A dot for each column"), i18n("Its name, or its number"), i18n("Its number")]
    }

    ConfigCheck {
        id: showEmptyWorkspaces
        Kirigami.FormData.label: i18n("Show:")
        page: page
        key: "showEmptyWorkspaces"
        enabled: showWorkspaces.checked
        text: i18n("Empty workspaces")
    }

    ConfigCheck {
        id: wheelSwitchesWorkspaces
        Kirigami.FormData.label: i18n("Scroll wheel:")
        page: page
        key: "wheelSwitchesWorkspaces"
        enabled: showWorkspaces.checked
        text: i18n("Scroll over the workspaces to switch between them")
    }
}
