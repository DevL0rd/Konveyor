import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: page

    default property alias content: column.data
    property Component preview: null
    property bool previewBackdrop: false
    property var jumpSections: []
    readonly property bool previewPinned: page.height >= Kirigami.Units.gridUnit * 26

    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: Kirigami.Units.gridUnit

    header: ColumnLayout {
        spacing: 0

        StatusMessages {
            Layout.fillWidth: true
        }

        PreviewStage {
            Layout.fillWidth: true
            visible: page.preview !== null && page.previewPinned
            sourceComponent: page.previewPinned ? page.preview : null
            backdrop: page.previewBackdrop
        }

        SectionJumpBar {
            Layout.fillWidth: true
            visible: page.jumpSections.length > 0
            sections: page.jumpSections
            flickable: page.flickable
        }
    }

    ColumnLayout {
        width: page.width
        spacing: 0

        PreviewStage {
            Layout.fillWidth: true
            visible: page.preview !== null && !page.previewPinned
            sourceComponent: page.previewPinned ? null : page.preview
            backdrop: page.previewBackdrop
        }

        ColumnLayout {
            id: column
            Layout.fillWidth: true
            spacing: 0
        }
    }
}
