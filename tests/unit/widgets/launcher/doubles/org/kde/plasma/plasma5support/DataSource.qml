import QtQuick

QtObject {
    id: source

    property string engine: ""
    property var connectedSources: []
    signal newData(string sourceName, var data)

    function answer(command) {
        const result = Commands.record(command)
        if (result)
            Qt.callLater(() => source.newData(command, result))
    }
    function connectSource(command) {
        answer(command)
    }
    function disconnectSource(command) {
        Commands.finish(command)
    }
    onConnectedSourcesChanged: {
        for (const command of connectedSources)
            answer(command)
    }
}
