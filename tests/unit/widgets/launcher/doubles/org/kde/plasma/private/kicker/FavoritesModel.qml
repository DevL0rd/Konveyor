import QtQuick

KickerModel {
    id: favorites

    property string client: ""
    name: "favorites"

    function initForClient(id) {
        client = id
        fill(KickerFixture.appRows(KickerFixture.favorites))
    }
    function rowOf(id) {
        for (let row = 0; row < count; ++row) {
            if (get(row).favoriteId === id)
                return row
        }
        return -1
    }
    function isFavorite(id) {
        return rowOf(id) >= 0
    }
    function addFavorite(id) {
        if (!isFavorite(id))
            append(Object.assign({ display: "", decoration: "", url: "", description: "", isNewlyInstalled: false, hasActionList: false }, KickerFixture.app(id)))
    }
    function removeFavorite(id) {
        const row = rowOf(id)
        if (row >= 0)
            remove(row, 1)
    }
    function moveRow(from, to) {
        move(from, to, 1)
    }
}
