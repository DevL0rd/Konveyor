import QtQuick

ScopedRow {
    id: sliderRow

    required property string blockPath
    required property string name
    property string propertyName
    property real current
    property real from: 0
    property real to: 32
    property string unit: "px"
    property real factor: 1

    subPaths: [name]
    showOverride: false

    ValueSlider {
        value: sliderRow.current * sliderRow.factor
        from: sliderRow.from
        to: sliderRow.to
        unit: sliderRow.unit
        onEdited: value => {
            const number = Math.round(value) / sliderRow.factor;
            if (sliderRow.propertyName.length > 0) {
                const props = {};
                props[sliderRow.propertyName] = number;
                kcm.setValue(sliderRow.blockPath + "/" + sliderRow.name, [], props);
            } else {
                kcm.setValue(sliderRow.blockPath + "/" + sliderRow.name, [number]);
            }
        }
    }
}
