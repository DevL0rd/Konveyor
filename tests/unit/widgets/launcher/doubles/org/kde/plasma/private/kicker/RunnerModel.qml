import QtQuick

KickerModel {
    id: runner

    property var favoritesModel: null
    property bool mergeResults: false
    property var runners: []
    property string query: ""
    property bool querying: false
    property var groupModels: []
    property var queries: []
    signal queryFinished()
    property Component groupComponent: Component {
        KickerModel {}
    }
    name: "runner"

    function modelForRow(row) {
        return row >= 0 && row < groupModels.length ? groupModels[row] : null
    }
    function matching(rows) {
        const term = query.toLowerCase()
        return rows.filter(row => String(row.display).toLowerCase().indexOf(term) >= 0 || !!row.always)
    }
    function rebuild() {
        for (const model of groupModels)
            model.destroy()
        const models = []
        if (query !== "") {
            queries = queries.concat([{ query: query, runners: runners.slice() }])
            for (const group of KickerFixture.runnerGroups) {
                if (runners.indexOf(group.runner) < 0)
                    continue
                const all = group.apps ? KickerFixture.appRows(Object.keys(KickerFixture.apps)) : group.rows.map(row => Object.assign({ always: true }, row))
                const rows = matching(all)
                if (rows.length === 0)
                    continue
                const model = groupComponent.createObject(runner, { name: group.name })
                model.fill(rows)
                models.push(model)
            }
        }
        groupModels = models
        fill(models.map(model => ({ display: model.name })))
        queryFinished()
    }
    onQueryChanged: rebuild()
    onRunnersChanged: if (query !== "") rebuild()
}
