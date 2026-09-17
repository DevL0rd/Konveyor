import QtQuick
import org.kde.konveyor.components

SceneAnimation {
    id: diagram

    property string side: "left"
    property bool running: true

    animated: running
    scene: {
        const opened = side === "left" ? [{ key: "a", w: 0.24 }, { key: "n", w: 0.24, focus: "n" }, { key: "b", w: 0.32 }, { key: "c", w: 0.24 }] : [{ key: "a", w: 0.24 }, { key: "b", w: 0.32 }, { key: "n", w: 0.24, focus: "n" }, { key: "c", w: 0.24 }];
        return build([columns([{ key: "a", w: 0.24 }, { key: "b", w: 0.32, focus: "b" }, { key: "c", w: 0.24 }]), columns(opened)]);
    }
}
