import QtQuick

ListModel {
    id: model

    enum GroupMode {
        GroupDisabled,
        GroupApplications
    }

    property int groupMode: TasksModel.GroupApplications
    property bool filterByVirtualDesktop: true
    property bool filterByScreen: true
    property bool filterByActivity: true
    property bool filterNotMinimized: false
    property var activated: []
    property var closed: []
    dynamicRoles: true

    function fill() {
        clear()
        for (const row of TaskFixture.windows)
            append(Object.assign({ display: "", decoration: "", AppName: "", AppId: "", GenericName: "", IsWindow: true, IsMinimized: false, IsActive: false }, row))
    }
    function makeModelIndex(row) {
        return row
    }
    function requestActivate(index) {
        activated = activated.concat([get(index).display])
    }
    function requestClose(index) {
        closed = closed.concat([get(index).display])
    }

    Component.onCompleted: fill()
    readonly property Connections fixture: Connections {
        target: TaskFixture
        function onWindowsChanged() { model.fill() }
    }
}
