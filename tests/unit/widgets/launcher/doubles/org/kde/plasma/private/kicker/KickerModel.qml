import QtQuick

ListModel {
    id: model

    property string name: ""
    property var appletInterface: null
    property var triggered: []
    dynamicRoles: true

    function fill(rows) {
        clear()
        for (const row of rows || [])
            append(Object.assign({ display: "", decoration: "", favoriteId: "", url: "", description: "", isNewlyInstalled: false, hasActionList: false }, row))
    }
    function labelForRow(row) {
        return row >= 0 && row < count ? get(row).display : ""
    }
    function modelForRow(row) {
        return null
    }
    function trigger(row, actionId, argument) {
        triggered = triggered.concat([{ row: row, favoriteId: row >= 0 && row < count ? get(row).favoriteId : "", actionId: actionId || "", argument: argument === undefined ? null : argument }])
        KickerFixture.triggered = KickerFixture.triggered.concat([{ model: name, row: row, favoriteId: row >= 0 && row < count ? get(row).favoriteId : "", actionId: actionId || "" }])
        return true
    }
}
