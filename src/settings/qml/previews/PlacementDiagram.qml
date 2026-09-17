import QtQuick
import org.kde.konveyor.components

Item {
    id: diagram

    property bool stack: false
    property int rows: 2
    property bool running: true
    property int opened: 0

    readonly property int total: diagram.stack ? diagram.rows : 3

    Timer {
        interval: 1100
        repeat: true
        running: diagram.running
        onTriggered: diagram.opened = (diagram.opened + 1) % (diagram.total + 1)
        onRunningChanged: diagram.opened = running ? 0 : diagram.total
    }

    Component.onCompleted: opened = running ? 0 : total

    MiniColumns {
        anchors.fill: parent
        offset: diagram.stack ? 0 : -Math.max(0, diagram.opened - 2) * 0.28
        columns: {
            const width = diagram.stack ? 0.36 : 0.26;
            const result = [{ width: width, focused: !diagram.stack && diagram.opened === 0 }];
            if (!diagram.stack) {
                for (let i = 0; i < diagram.opened; ++i) {
                    result.push({ width: width, focused: i === diagram.opened - 1 });
                }
                return result;
            }
            const count = diagram.opened + 1;
            if (count <= diagram.rows) {
                return result.concat([{ width: width, focused: true, stack: count }]);
            }
            const columns = Math.ceil(count / diagram.rows);
            const spread = [];
            for (let c = 0; c < columns; ++c) {
                spread.push({ width: width, stack: Math.floor(count / columns) + (c < count % columns ? 1 : 0), focused: c === columns - 1 });
            }
            return [{ width: width * 0.5 }].concat(spread);
        }
    }
}
