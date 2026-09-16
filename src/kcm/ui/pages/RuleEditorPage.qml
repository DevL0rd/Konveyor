import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../components/rules"
import "../catalog/Kdl.js" as Kdl
import "../catalog/RuleSummary.js" as RuleSummary

SettingsPage {
    id: page

    property string rulePath
    readonly property var rule: kcm.revision >= 0 ? kcm.node(rulePath) : ({})
    readonly property var matches: kcm.revision >= 0 ? kcm.children(rulePath, "match") : []
    readonly property var excludes: kcm.revision >= 0 ? kcm.children(rulePath, "exclude") : []

    title: "Window rule"

    Component.onCompleted: kcm.live.refresh()

    Kirigami.PlaceholderMessage {
        visible: page.rule.name === undefined
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
        icon.name: "dialog-question"
        text: "This rule no longer exists"
    }

    Card {
        visible: page.rule.name !== undefined
        Layout.topMargin: Kirigami.Units.gridUnit

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Heading {
                level: 3
                text: RuleSummary.target(page.rule, appId => kcm.live.nameFor(appId))
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            QQC2.Label {
                text: RuleSummary.summary(page.rule)
                wrapMode: Text.Wrap
                opacity: 0.75
                Layout.fillWidth: true
            }

            RuleMatchesPreview {
                Layout.fillWidth: true
                rulePath: page.rulePath
            }
        }
    }

    CardHeader {
        visible: page.rule.name !== undefined
        title: "Which windows"
    }

    Card {
        visible: page.rule.name !== undefined

        QQC2.Label {
            visible: page.matches.length === 0
            text: "Applies to every window. Add a match to narrow it down."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
        }

        Repeater {
            model: page.matches.length

            MatchBlockEditor {
                required property int index
                Layout.fillWidth: true
                heading: index === 0 ? "Windows where" : "Or windows where"
                path: page.matches[index] ? page.matches[index].path : ""
                node: page.matches[index] ? page.matches[index].node : ({ props: {} })
                onRemoveRequested: kcm.remove(path)
            }
        }

        QQC2.Button {
            Layout.margins: Kirigami.Units.largeSpacing
            icon.name: "list-add"
            text: page.matches.length === 0 ? "Match specific windows…" : "Or also match…"
            onClicked: kcm.append(page.rulePath, Kdl.leaf("match"))
        }
    }

    CardHeader {
        visible: page.rule.name !== undefined
        title: "Except"
    }

    Card {
        visible: page.rule.name !== undefined

        Repeater {
            model: page.excludes.length

            MatchBlockEditor {
                required property int index
                Layout.fillWidth: true
                heading: index === 0 ? "Skip windows where" : "Or skip windows where"
                path: page.excludes[index] ? page.excludes[index].path : ""
                node: page.excludes[index] ? page.excludes[index].node : ({ name: "exclude", props: {} })
                onRemoveRequested: kcm.remove(path)
            }
        }

        QQC2.Button {
            Layout.margins: Kirigami.Units.largeSpacing
            icon.name: "list-remove"
            text: "Except windows that…"
            onClicked: kcm.append(page.rulePath, Kdl.leaf("exclude"))
        }
    }

    CardHeader {
        visible: page.rule.name !== undefined
        title: "When it opens"
    }

    Card {
        visible: page.rule.name !== undefined

        RuleOpenSection {
            Layout.fillWidth: true
            rulePath: page.rulePath
        }
    }

    CardHeader {
        visible: page.rule.name !== undefined
        title: "Size limits"
    }

    Card {
        visible: page.rule.name !== undefined

        LimitRow {
            path: page.rulePath + "/min-width"
            label: "Minimum width"
            iconName: "distribute-horizontal-x"
        }

        LimitRow {
            path: page.rulePath + "/max-width"
            label: "Maximum width"
            iconName: "distribute-horizontal-x"
        }

        LimitRow {
            path: page.rulePath + "/min-height"
            label: "Minimum height"
            iconName: "distribute-vertical-y"
        }

        LimitRow {
            path: page.rulePath + "/max-height"
            label: "Maximum height"
            iconName: "distribute-vertical-y"
        }
    }

    CardHeader {
        visible: page.rule.name !== undefined
        title: "Look"
    }

    Card {
        visible: page.rule.name !== undefined

        RuleLookSection {
            Layout.fillWidth: true
            rulePath: page.rulePath
        }
    }

    QQC2.Button {
        visible: page.rule.name !== undefined
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.gridUnit
        icon.name: "edit-delete"
        text: "Delete this rule"
        onClicked: {
            kcm.remove(page.rulePath);
            kcm.pop();
        }
    }
}
