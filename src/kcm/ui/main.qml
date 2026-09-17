import QtQuick
import org.kde.konveyor.settings

SettingsHub {
    id: root

    property Binding needsSaveBinding: Binding {
        target: kcm
        property: "needsSave"
        value: SettingsStore.needsSave
    }
    property Binding defaultsBinding: Binding {
        target: kcm
        property: "representsDefaults"
        value: SettingsStore.representsDefaults
    }
    property Binding depthBinding: Binding {
        target: SettingsNavigation
        property: "depth"
        value: kcm.depth
    }
    property Connections moduleConnections: Connections {
        target: kcm
        function onLoadRequested() {
            SettingsStore.load();
        }
        function onSaveRequested() {
            SettingsStore.save();
        }
        function onDefaultsRequested() {
            SettingsStore.defaults();
        }
    }
    property Connections navigationConnections: Connections {
        target: SettingsNavigation
        function onPushRequested(page, properties) {
            const item = PageFactory.create(page, properties, root);
            if (item) {
                kcm.push(item);
            }
        }
        function onPopRequested() {
            kcm.pop();
        }
    }

    initialPage: kcm.initialPage
}
