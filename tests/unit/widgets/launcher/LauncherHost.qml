import QtQuick

Item {
    id: root

    required property var config
    required property bool portal

    width: 1400
    height: 900

    property bool open: false
    property var pendingPins: []
    signal pinsRequested()
    property string requestedPage: ""
    property string currentPage: ""
    property int hideCount: 0
    property int configureCount: 0
    readonly property var launcherConfig: config
    readonly property string favoritesClient: portal ? "org.kde.plasma.kicker.favorites.instance-7" : config.favoritesClient
    readonly property var kickerApplet: portal ? root : null
    readonly property alias view: view

    function hide() {
        hideCount++
        open = false
    }

    function configure() {
        configureCount++
        hide()
    }

    LauncherView {
        id: view
        anchors.fill: parent
        compact: root.portal
        onPageChanged: root.currentPage = page
    }
}
