import QtQuick

KickerModel {
    property var systemApplications: []
    name: "places"
    Component.onCompleted: fill(KickerFixture.places)
}
