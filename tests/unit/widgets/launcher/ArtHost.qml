import QtQuick
import "lib"

Item {
    id: host

    required property var game
    property alias count: cards.model
    readonly property Item art: cards.count > 0 ? cards.itemAt(0) : null

    width: 200
    height: 200

    Repeater {
        id: cards
        model: 1
        delegate: GameArt {
            width: 100
            height: 100
            game: host.game
        }
    }
}
