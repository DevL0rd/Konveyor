import QtQuick

KickerModel {
    id: recent

    enum ShownItems {
        AppsAndDocs,
        OnlyApps,
        OnlyDocs,
        OnlyFolders
    }
    enum Ordering {
        Recent,
        Popular
    }

    property int shownItems: RecentUsageModel.AppsAndDocs
    property int ordering: RecentUsageModel.Recent
    name: "recent"

    function rows() {
        if (shownItems === RecentUsageModel.OnlyApps)
            return KickerFixture.appRows(KickerFixture.recentApps)
        if (shownItems === RecentUsageModel.OnlyDocs)
            return KickerFixture.recentDocs
        if (shownItems === RecentUsageModel.OnlyFolders)
            return KickerFixture.recentFolders
        return KickerFixture.appRows(KickerFixture.recentApps).concat(KickerFixture.recentDocs)
    }
    Component.onCompleted: fill(rows())
}
