import QtQuick
import QtQuick.Controls as QQC2
import org.kde.konveyor.settings

QQC2.StackView {
    id: stack

    required property var page
    property string reveal
    property string section
    signal revealed()

    function prepare(item) {
        if (item && item.background !== undefined)
            item.background = null
    }
    function collect(item, text, found) {
        if (!item || !item.visible)
            return found
        if (item.title === text || item.label === text || item.text === text)
            found.push(item)
        const children = item.children || []
        for (let i = 0; i < children.length; ++i)
            collect(children[i], text, found)
        return found
    }
    function revealTarget() {
        const item = stack.currentItem
        if (!item || stack.reveal === "" || !item.flickable)
            return
        const flick = item.flickable
        const top = entry => entry.mapToItem(flick.contentItem, 0, 0).y
        const headers = collect(flick.contentItem, stack.section, [])
        const floor = headers.length > 0 ? top(headers[0]) : 0
        const target = collect(flick.contentItem, stack.reveal, []).find(entry => top(entry) >= floor)
        stack.revealed()
        if (target)
            flick.contentY = Math.max(0, Math.min(top(target) - flick.height * 0.25, flick.contentHeight - flick.height))
    }
    function open(file) {
        const component = PageFactory.component(file)
        if (!component)
            return
        replace(null, component, {}, QQC2.StackView.Immediate)
        prepare(currentItem)
        revealTimer.restart()
    }

    clip: true
    onRevealChanged: if (reveal !== "") revealTimer.restart()
    Timer {
        id: revealTimer
        interval: 120
        onTriggered: stack.revealTarget()
    }
    Component.onCompleted: open(page.file)
    onPageChanged: open(page.file)

    Binding {
        target: SettingsNavigation
        property: "depth"
        value: stack.depth
    }
    Connections {
        target: SettingsNavigation
        function onPushRequested(file, properties) {
            const component = PageFactory.component(file)
            if (component) {
                stack.push(component, properties)
                stack.prepare(stack.currentItem)
            }
        }
        function onPopRequested() {
            if (stack.depth > 1)
                stack.pop()
        }
    }
}
