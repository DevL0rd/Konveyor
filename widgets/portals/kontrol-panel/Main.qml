import QtQuick

Item {
    id: root

    required property QtObject service
    required property var config

    property bool open: false
    readonly property bool openedByKey: true
    property var pendingPins: []
    signal pinsRequested()
    property string requestedPage: ""
    property string currentPage: ""
    signal pageRequested(string page)

    readonly property var launcherConfig: config
    readonly property string favoritesClient: config.favoritesClient
    readonly property var kickerApplet: null

    function show() {
        open = true
    }
    function hide() {
        open = false
    }
    function toggle() {
        if (open)
            hide()
        else
            show()
    }
    function handleRequest(page) {
        if (open && currentPage === page) {
            hide()
        } else if (open) {
            pageRequested(page)
        } else {
            requestedPage = page
            show()
        }
    }
    function configure() {
        hide()
        configWindow.active = true
        configWindow.item.raise()
        configWindow.item.requestActivate()
    }

    onOpenChanged: service.setOpen(open)

    Component.onCompleted: {
        const page = config.openPageOnStart
        if (page !== "" && service.konveyorRunning()) {
            config.openPageOnStart = ""
            handleRequest(page)
        }
    }

    Connections {
        target: root.service
        function onToggleRequested() {
            root.toggle()
        }
        function onOpenRequested(page) {
            root.handleRequest(page)
        }
        function onHideRequested() {
            root.hide()
        }
        function onPinRequested(files) {
            root.pendingPins = root.pendingPins.concat(files.filter(path => path.endsWith(".desktop")))
            root.pinsRequested()
        }
        function onConfigureRequested() {
            root.configure()
        }
    }

    Overlay {}

    Loader {
        id: configWindow
        active: false
        sourceComponent: ConfigWindow {
            onClosing: configWindow.active = false
        }
    }
}
