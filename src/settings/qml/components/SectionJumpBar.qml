import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "HeaderLookup.js" as HeaderLookup

Rectangle {
    id: root

    property var sections: []
    property Flickable flickable: null

    property string current: sections.length > 0 ? sections[0] : ""

    implicitHeight: row.implicitHeight + Kirigami.Units.smallSpacing * 2
    color: Kirigami.Theme.backgroundColor

    function updateCurrent() {
        const flick = root.flickable
        if (!flick)
            return
        const line = flick.contentY + Kirigami.Units.largeSpacing * 2
        let best = root.sections.length > 0 ? root.sections[0] : ""
        for (const title of root.sections) {
            const header = HeaderLookup.collect(flick.contentItem, title, [])[0]
            if (header && header.mapToItem(flick.contentItem, 0, 0).y <= line)
                best = title
        }
        const atEnd = flick.contentY >= flick.contentHeight - flick.height - 1
        root.current = atEnd && root.sections.length > 0 && flick.contentHeight > flick.height ? root.sections[root.sections.length - 1] : best
    }

    Timer {
        id: settle
        interval: 80
        onTriggered: root.updateCurrent()
    }

    Connections {
        target: root.flickable
        function onContentYChanged() { settle.restart() }
        function onContentHeightChanged() { settle.restart() }
    }

    onFlickableChanged: settle.restart()
    onSectionsChanged: settle.restart()

    property string jumpTarget

    Timer {
        id: jumpRetry
        interval: 50
        onTriggered: root.jump(root.jumpTarget)
    }

    function jump(title) {
        const flick = root.flickable
        if (!flick)
            return
        root.jumpTarget = title
        if (HeaderLookup.pending(flick.contentItem)) {
            jumpRetry.restart()
            return
        }
        const header = HeaderLookup.collect(flick.contentItem, title, [])[0]
        if (!header)
            return
        const y = header.mapToItem(flick.contentItem, 0, 0).y
        flick.contentY = Math.max(0, Math.min(y - Kirigami.Units.largeSpacing, flick.contentHeight - flick.height))
    }

    Flow {
        id: row
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(rowWidth, parent.width - Kirigami.Units.smallSpacing * 2)
        spacing: Kirigami.Units.smallSpacing

        property real rowWidth: 0

        function measure() {
            let total = 0
            for (let i = 0; i < jumpButtons.count; ++i) {
                const button = jumpButtons.itemAt(i)
                total += (button ? button.implicitWidth : 0) + (i > 0 ? spacing : 0)
            }
            rowWidth = total
        }

        Repeater {
            id: jumpButtons
            model: root.sections
            onItemAdded: row.measure()

            QQC2.AbstractButton {
                id: jumpButton
                required property var modelData

                text: modelData
                hoverEnabled: true
                activeFocusOnTab: true
                leftPadding: Kirigami.Units.largeSpacing
                rightPadding: Kirigami.Units.largeSpacing
                topPadding: Kirigami.Units.smallSpacing + 1
                bottomPadding: Kirigami.Units.smallSpacing + 1
                onClicked: root.jump(modelData)
                onImplicitWidthChanged: row.measure()

                contentItem: QQC2.Label {
                    text: jumpButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: Kirigami.Units.cornerRadius
                    color: root.current === jumpButton.modelData ? Qt.alpha(Kirigami.Theme.textColor, 0.14)
                        : jumpButton.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.07) : "transparent"
                    border.width: jumpButton.visualFocus ? 1 : 0
                    border.color: Kirigami.Theme.highlightColor
                }
            }
        }
    }

    Kirigami.Separator {
        anchors.bottom: parent.bottom
        width: parent.width
    }
}
