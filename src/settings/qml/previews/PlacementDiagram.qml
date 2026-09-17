import QtQuick
import org.kde.konveyor.components

SceneAnimation {
    id: diagram

    property bool stack: false
    property int rows: 2
    property bool running: true

    animated: running
    scene: {
        const width = 0.28;
        const total = stack ? rows * 2 : 4;
        const frames = [];
        for (let count = 1; count <= total; ++count) {
            const keys = Array.from({ length: count }, (_, i) => "w" + i);
            const focus = keys[count - 1];
            const list = [];
            if (stack) {
                for (let c = 0; c * rows < count; ++c) {
                    list.push({ keys: keys.slice(c * rows, (c + 1) * rows), w: width, focus: focus });
                }
            } else {
                keys.forEach(key => list.push({ key: key, w: width, focus: focus }));
            }
            frames.push(columns(list, { offset: -Math.max(0, list.length - 3) * (width + 0.035) }));
        }
        return build(frames, { still: frames.length - 1 });
    }
}
