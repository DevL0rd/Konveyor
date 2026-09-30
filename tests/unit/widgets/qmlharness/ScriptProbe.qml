import QtQuick
import "Format.js" as Fmt
import "Ring.js" as Ring
import "History.js" as History
import "PopStage.js" as Stage
import "PopStyle.js" as Style
import "Highlight.js" as Highlight
QtObject {
    readonly property var theme: ({ negativeTextColor: "red", neutralTextColor: "orange", positiveTextColor: "green", textColor: "black",
                                    highlightColor: "blue", backgroundColor: Qt.color("#202020") })
    readonly property var light: ({ highlightColor: "blue", backgroundColor: Qt.color("#f0f0f0") })
    function filled(capacity, values) {
        const ring = Ring.make(capacity)
        for (const value of values)
            Ring.push(ring, value)
        return ring
    }
    function history(length, key, values) {
        const made = History.make(length)
        for (const value of values)
            History.push(made, key, value)
        return made
    }
    readonly property var chips: [{ low: 0, high: 3, max: 100, sizes: [10, 20, 30, 40], flex: false },
                                  { low: 1, high: 2, max: 50, sizes: [5, 15, 25, 35], flex: true },
                                  { low: 0, high: 3, max: 0, sizes: [9, 9, 9, 9], flex: true }]
}
