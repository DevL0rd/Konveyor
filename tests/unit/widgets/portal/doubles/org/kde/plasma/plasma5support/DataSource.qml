import QtQuick

QtObject {
    objectName: "dataSource"
    property string engine
    property int interval
    property var connectedSources: []
    property var requested: []
    signal newData(string sourceName, var data)

    function connectSource(source) {
        requested = requested.concat([source])
        connectedSources = connectedSources.concat([source])
    }
    function disconnectSource(source) {
        connectedSources = connectedSources.filter(connected => connected !== source)
    }
}
