import QtQuick

Item {
    id: clock

    property real duration: 1
    property real rest: 0.8
    property bool playing: false
    property bool still: false
    property real progress: 0
    readonly property real time: still ? duration * 0.35 : progress

    function restart() {
        progress = 0;
        if (loop.running) {
            loop.restart();
        }
    }

    onDurationChanged: restart()

    NumberAnimation on progress {
        id: loop
        from: 0
        to: clock.duration + clock.rest
        duration: (clock.duration + clock.rest) * 1000
        loops: Animation.Infinite
        running: clock.playing && !clock.still
    }
}
