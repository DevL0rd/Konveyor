import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: page

    default property alias content: column.data
    property Component preview: null
    property bool previewPinned: true

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
            sourceComponent: page.preview
        }
    }

    ColumnLayout {
        id: column
        width: page.width
        spacing: 0
    }
}
