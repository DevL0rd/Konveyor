import QtQuick
import org.kde.konveyor.components

SceneAnimation {
    id: diagram

    property string kind
    property bool running: true

    animated: running
    scene: {
        const col = (key, extra) => Object.assign({ key: key, w: 0.28 }, extra || {});
        switch (kind) {
        case "group-off":
            return build([columns([col("a", { alt: true }), col("b"), col("c", { focus: "c" })]), columns([col("a", { alt: true }), col("b"), col("c"), col("n", { alt: true, focus: "n" })], { offset: -0.315 })]);
        case "group-beside":
            return build([columns([col("a", { alt: true }), col("b"), col("c", { focus: "c" })]), columns([col("a", { alt: true }), col("n", { alt: true, focus: "n" }), col("b"), col("c")])]);
        case "group-stack":
            return build([columns([col("a", { alt: true }), col("b"), col("c", { focus: "c" })]), columns([{ keys: ["a", "n"], w: 0.28, alt: true, focus: "n" }, col("b"), col("c")])]);
        case "stacked":
            return build([columns([col("a"), col("b", { focus: "b" }), col("c")]), columns([col("a"), { keys: ["b", "n"], w: 0.28, focus: "n" }, col("c")])]);
        case "tabbed":
            return build([columns([col("a"), col("b", { focus: "b" }), col("c")]), columns([col("a"), { keys: ["b", "n"], w: 0.28, focus: "n", tabbed: true }, col("c")])]);
        case "never":
        case "always":
            return build(scroll([0.36, 0.36, 0.36, 0.36], kind), { still: 2 });
        case "on-overflow":
            return build(scroll([0.3, 0.3, 0.62, 0.3], kind), { still: 2 });
        }
        return null;
    }
}
