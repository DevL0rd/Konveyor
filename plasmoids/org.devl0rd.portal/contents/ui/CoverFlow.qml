import QtQuick
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami

Item {
    id: cf
    clip: true
    property var items: []
    property var friendsByAppid: ({})
    function friendCountFor(g) {
        return (g && g.appid && friendsByAppid[g.appid]) ? friendsByAppid[g.appid].length : 0
    }
    property bool tilt: false
    property bool shrink: true
    property int cardWidth: 150
    property bool showTitles: true
    signal launchRequested(var game)
    signal menuRequested(var game)

    readonly property real _pad: Kirigami.Units.largeSpacing
    readonly property real _maxFitH: Math.max(80, (height - _pad * 2) / 1.1)
    readonly property real _minH: Math.min(_maxFitH, Kirigami.Units.gridUnit * 6)
    readonly property real _zoomFrac: Math.max(0, Math.min(1, (cardWidth - 100) / 220))
    readonly property real cardH: _minH + (_maxFitH - _minH) * _zoomFrac
    readonly property real cw: cardH / 1.5
    readonly property real reflH: cardH * 0.4

    HoverHandler { id: cfHover; onHoveredChanged: if (!hovered) cf.centerArmed = false }

    property real pos: 0
    property int targetIndex: 0
    property bool centerArmed: false
    readonly property real unitSpacing: Math.max(20, cw * 0.6)

    function wrapD(raw) {
        var n = items ? items.length : 0
        if (n <= 1) return raw
        return raw - n * Math.round(raw / n)
    }

    NumberAnimation { id: posAnim; target: cf; property: "pos"; easing.type: Easing.OutCubic }
    function glideTo(i) {
        targetIndex = Math.round(i)
        posAnim.stop()
        posAnim.from = pos
        posAnim.to = targetIndex
        posAnim.duration = Math.max(150, Math.min(750, 150 + Math.abs(targetIndex - pos) * 110))
        posAnim.start()
    }
    function browse(step) { centerArmed = false; glideTo(targetIndex + step) }
    onItemsChanged: posAnim.stop()

    DragHandler {
        id: dragH
        target: null
        xAxis.enabled: true
        yAxis.enabled: false
        property real startPos: 0
        onActiveChanged: {
            if (active) { posAnim.stop(); startPos = cf.pos; cf.centerArmed = false }
            else cf.glideTo(cf.pos - (centroid.velocity.x / cf.unitSpacing) * 0.32)
        }
        onTranslationChanged: if (active) cf.pos = startPos - translation.x / cf.unitSpacing
    }

    readonly property real baseGap: cw * (tilt ? 0.9 : 1.05)
    readonly property real maxOffset: Math.max(baseGap * 1.5,
        width / 2 + (tilt ? cw * 0.1 : -cw * 0.35))
    readonly property real falloff: Math.max(0.5, Math.min(0.85, 1 - baseGap / maxOffset))
    function offsetFor(d) {
        var off = maxOffset * (1 - Math.pow(falloff, Math.abs(d)))
        return d < 0 ? -off : off
    }

    Repeater {
        model: cf.items
        delegate: Item {
            id: tile
            readonly property real d: cf.wrapD(index - cf.pos)
            readonly property real ad: Math.abs(d)
            readonly property real off: cf.offsetFor(d)
            readonly property real cardH: cf.cardH
            readonly property real reflH: cf.reflH
            width: cf.cw
            height: cardH
            visible: Math.abs(off) < cf.width / 2 + cf.cw
            x: cf.width / 2 + off - width / 2
            y: (cf.height - cardH) / 2
            z: Math.round(2000 - ad * 10)
            scale: cf.shrink ? Math.max(0.5, 0.55 + 0.45 * Math.pow(0.82, ad)) : 1.0
            opacity: {
                var dist = Math.abs(off)
                var fs = cf.width / 2 - cf.cw * 0.6
                return dist < fs ? 1.0 : Math.max(0.0, 1.0 - (dist - fs) / (cf.cw * 1.2))
            }
            transform: Rotation {
                origin.x: tile.width / 2; origin.y: tile.height / 2
                axis { x: 0; y: 1; z: 0 }
                angle: cf.tilt ? (d < 0 ? 1 : -1) * 72 * (1 - Math.pow(0.6, ad)) : 0
            }

            Rectangle {
                x: 0; y: Kirigami.Units.smallSpacing
                width: tile.width; height: tile.cardH
                radius: Kirigami.Units.smallSpacing
                color: "#000000"; opacity: 0.35
                z: -1
            }

            GameCard {
                id: cfCard
                anchors.fill: parent
                game: modelData
                friendCount: cf.friendCountFor(modelData)
                showTitle: cf.showTitles
                disarmOnExit: false
                armed: tile.ad < 0.5 && cf.centerArmed
                onCardClicked: { cf.glideTo(cf.pos + tile.d); cf.centerArmed = true }
                onLaunchRequested: cf.launchRequested(modelData)
                onMenuRequested: cf.menuRequested(modelData)
            }

            Item {
                id: reflWrap
                anchors.top: cfCard.bottom
                anchors.horizontalCenter: cfCard.horizontalCenter
                width: cfCard.width
                height: cfCard.height
                opacity: 0.32
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: LinearGradient {
                        width: reflWrap.width; height: reflWrap.height
                        start: Qt.point(0, 0); end: Qt.point(0, reflWrap.height)
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#c0ffffff" }
                            GradientStop { position: tile.reflH / tile.cardH; color: "#00ffffff" }
                            GradientStop { position: 1.0; color: "#00ffffff" }
                        }
                    }
                }
                ShaderEffectSource {
                    anchors.fill: parent
                    sourceItem: cfCard
                    live: true
                    transform: Scale { origin.y: reflWrap.height / 2; yScale: -1 }
                }
            }
        }
    }
}
