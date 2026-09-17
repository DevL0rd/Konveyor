.pragma library

function typeName(page) {
    return page.split("/").pop().replace(/\.qml$/, "");
}

function component(page) {
    const loaded = Qt.createComponent("org.kde.konveyor.settings", typeName(page));
    if (loaded.status !== 1) {
        console.warn("Konveyor settings page", page, loaded.errorString());
        return null;
    }
    return loaded;
}

function create(page, properties, parent) {
    const loaded = component(page);
    return loaded ? loaded.createObject(parent, properties || {}) : null;
}
