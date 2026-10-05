import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.TextField {
    id: field
    Layout.fillWidth: true
    Layout.preferredWidth: 0
    background: null
    leftPadding: 0
    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
    placeholderText: i18n("Search apps, games, files, settings, friends and packages")
    onTextChanged: Qt.callLater(launcher.resetSelection)
    Keys.onPressed: function(event) {
        const ctrl = event.modifiers & Qt.ControlModifier
        const alt = event.modifiers & Qt.AltModifier
        if (event.key === Qt.Key_Escape) {
            if (field.text !== "")
                field.text = ""
            else
                root.hide()
        } else if (event.key === Qt.Key_Down) {
            launcher.navigate(0, 1)
        } else if (event.key === Qt.Key_Up) {
            launcher.navigate(0, -1)
        } else if (event.key === Qt.Key_Left && (field.text === "" || ctrl)) {
            launcher.navigate(-1, 0)
        } else if (event.key === Qt.Key_Right && (field.text === "" || ctrl || field.cursorPosition === field.length)) {
            launcher.navigate(1, 0)
        } else if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && alt) {
            if (launcher.searchSettled) {
                launcher.menuForCurrent()
            } else {
                launcher.settleSearch()
                Qt.callLater(launcher.menuForCurrent)
            }
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (launcher.searchSettled) {
                launcher.activateCurrent()
            } else {
                launcher.settleSearch()
                Qt.callLater(launcher.activateCurrent)
            }
        } else if (event.key === Qt.Key_Menu) {
            launcher.menuForCurrent()
        } else if (event.key === Qt.Key_Tab && ctrl) {
            launcher.stepPage(1)
        } else if (event.key === Qt.Key_Backtab && ctrl) {
            launcher.stepPage(-1)
        } else if (event.key === Qt.Key_Tab) {
            launcher.stepSection(true)
        } else if (event.key === Qt.Key_Backtab) {
            launcher.stepSection(false)
        } else if (ctrl && event.key === Qt.Key_Z && launcher.page === "settings" && !launcher.searching) {
            const view = launcher.currentView()
            if (view && view.undo)
                view.undo()
        } else if (ctrl && event.key === Qt.Key_Comma) {
            launcher.goToPage("settings")
        } else if (ctrl && (event.modifiers & Qt.ShiftModifier) && event.key === Qt.Key_P) {
            launcher.sidebarPinCurrent()
        } else if (ctrl && event.key === Qt.Key_P) {
            launcher.pinCurrent()
        } else if (alt && event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
            const target = event.key - Qt.Key_1
            if (target < launcher.pageDefs.length)
                launcher.goToPage(launcher.pageDefs[target].key)
        } else if (event.key === Qt.Key_Alt) {
            launcher.altHeld = true
            return
        } else if (ctrl && event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
            const at = event.key - Qt.Key_1
            if (at < launcherData.favorites.count)
                launcher.trigger(launcherData.favorites, at)
        } else {
            return
        }
        event.accepted = true
    }
    Keys.onReleased: function(event) {
        if (event.key === Qt.Key_Alt)
            launcher.altHeld = false
    }
    onActiveFocusChanged: if (!activeFocus) launcher.altHeld = false
}
