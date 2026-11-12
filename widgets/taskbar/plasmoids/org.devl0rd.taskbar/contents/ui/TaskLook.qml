import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore

QtObject {
    property real thickness: 32
    property bool vertical: false
    property int location: PlasmaCore.Types.BottomEdge
    property bool autoIconSize: true
    property int fixedIconSize: 32
    property int padding: 3
    property int spacing: 2
    property int highlightStyle: 0
    property int indicatorStyle: 0
    property bool indicatorOpposite: false
    property bool animate: true
    property bool pulse: true
    property bool tooltips: true
    property int nameMode: 0
    property int nameWidth: 140

    readonly property int nameGap: Kirigami.Units.smallSpacing
    readonly property int listInset: padding + Kirigami.Units.smallSpacing * 2
    readonly property int listIcon: autoIconSize ? Kirigami.Units.iconSizes.medium : fixedIconSize
    readonly property bool listed: vertical && namesFit(thickness)
    readonly property int iconSize: Math.max(12, Math.min(Math.round(thickness) - 2,
        listed ? listIcon : autoIconSize ? Math.round(thickness * 0.6) : fixedIconSize))
    readonly property int button: iconSize + padding * 2
    readonly property int edge: indicatorOpposite ? opposite(location) : location
    readonly property int shortDuration: animate ? Kirigami.Units.shortDuration : 0
    readonly property int longDuration: animate ? Kirigami.Units.longDuration : 0

    function nameRoom(across) {
        return vertical ? across - listInset * 2 - 2 - (nameMode === 1 ? listIcon + nameGap : 0) : nameWidth
    }

    function namesFit(across) {
        return nameMode !== 0 && (!vertical || nameRoom(across) >= Kirigami.Units.gridUnit * 2)
    }

    function opposite(side) {
        switch (side) {
        case PlasmaCore.Types.TopEdge:
            return PlasmaCore.Types.BottomEdge
        case PlasmaCore.Types.LeftEdge:
            return PlasmaCore.Types.RightEdge
        case PlasmaCore.Types.RightEdge:
            return PlasmaCore.Types.LeftEdge
        default:
            return PlasmaCore.Types.TopEdge
        }
    }
}
