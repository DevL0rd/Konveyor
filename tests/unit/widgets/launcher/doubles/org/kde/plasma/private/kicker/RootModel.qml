import QtQuick

KickerModel {
    id: root

    property bool autoPopulate: false
    property bool flat: false
    property bool sorted: false
    property bool showSeparators: false
    property bool showAllApps: false
    property bool showAllAppsCategorized: false
    property bool showRecentApps: false
    property bool showRecentDocs: false
    property bool showPowerSession: false
    property bool highlightNewlyInstalledApps: false
    readonly property FavoritesModel favoritesModel: FavoritesModel {}
    property var categoryModels: []
    property Component categoryComponent: Component {
        KickerModel {}
    }
    name: "root"

    function modelForRow(row) {
        return row >= 0 && row < categoryModels.length ? categoryModels[row] : null
    }
    Component.onCompleted: {
        const models = []
        for (const category of KickerFixture.categories) {
            const model = categoryComponent.createObject(root, { name: category.display })
            const rows = KickerFixture.appRows(category.apps)
            model.fill(sorted ? rows.sort((a, b) => a.display.localeCompare(b.display)) : rows)
            models.push(model)
        }
        categoryModels = models
        fill(KickerFixture.categories.map(category => ({ display: category.display })))
    }
}
