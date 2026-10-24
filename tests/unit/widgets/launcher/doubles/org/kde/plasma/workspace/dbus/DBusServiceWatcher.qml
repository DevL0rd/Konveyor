import QtQuick

QtObject {
    property int busType: BusType.Session
    property string watchedService: ""
    readonly property bool registered: Services.registered[watchedService] === true
}
