.pragma library

function typeName(page) {
    return page.split("/").pop().replace(/\.qml$/, "");
}

function create(page, properties, parent) {
    const component = Qt.createComponent("org.kde.konveyor.settings", typeName(page));
    if (component.status !== 1) {
        console.warn("Konveyor settings page", page, component.errorString());
        return null;
    }
    return component.createObject(parent, properties || {});
}
