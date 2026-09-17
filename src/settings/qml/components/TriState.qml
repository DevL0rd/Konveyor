import QtQuick

Segmented {
    id: root

    property var triState
    property string yesLabel: "Yes"
    property string noLabel: "No"
    property string unsetLabel: "Default"
    signal stateChosen(var state)

    options: [
        { value: "unset", label: root.unsetLabel },
        { value: "yes", label: root.yesLabel },
        { value: "no", label: root.noLabel }
    ]
    currentValue: root.triState === undefined || root.triState === null ? "unset" : (root.triState ? "yes" : "no")
    onChosen: value => root.triStateChosen(value === "unset" ? null : value === "yes")
}
