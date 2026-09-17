#include "ipc/model.h"

#include "config/loader.h"

namespace Konveyor::Ipc
{

namespace
{

QJsonArray pair(double first, double second)
{
    return QJsonArray {first, second};
}

QJsonValue optionalWindow(const std::optional<Layout::WindowId> &id)
{
    return id ? QJsonValue(static_cast<qint64>(*id)) : QJsonValue(QJsonValue::Null);
}

QJsonObject windowLayoutToJson(const Layout::WindowState &state)
{
    QJsonObject layout;
    layout[QStringLiteral("pos_in_scrolling_layout")]
        = state.isFloating ? QJsonValue(QJsonValue::Null) : QJsonValue(pair(state.columnIndex + 1, state.tileIndex + 1));
    layout[QStringLiteral("tile_size")] = pair(state.targetFrame.width(), state.targetFrame.height());
    layout[QStringLiteral("window_size")] = pair(qRound(state.targetFrame.width()), qRound(state.targetFrame.height()));
    layout[QStringLiteral("tile_pos_in_workspace_view")] = pair(state.targetFrame.x(), state.targetFrame.y());
    layout[QStringLiteral("window_offset_in_tile")] = pair(0, 0);
    return layout;
}

QString flagName(const QString &argument)
{
    return argument.startsWith(QLatin1String("--")) ? argument.mid(2) : QString();
}

std::expected<Layout::WindowId, QString> parseWindowId(const QString &text)
{
    bool ok = false;
    const quint64 id = text.toULongLong(&ok);
    if (!ok) {
        return std::unexpected(QStringLiteral("invalid window id: %1").arg(text));
    }
    return id;
}

QStringList jsonStrings(const QJsonValue &value)
{
    QStringList result;
    const QJsonArray array = value.toArray();
    for (const auto &item : array) {
        result.append(item.isString() ? item.toString() : QString::number(item.toDouble()));
    }
    return result;
}

}

QJsonObject windowToJson(const WindowIdentity &identity, const Layout::WindowState &state)
{
    QJsonObject object;
    object[QStringLiteral("id")] = static_cast<qint64>(identity.id);
    object[QStringLiteral("title")] = identity.title;
    object[QStringLiteral("app_id")] = identity.appId;
    object[QStringLiteral("pid")] = identity.pid;
    object[QStringLiteral("workspace_id")] = static_cast<qint64>(state.workspace);
    object[QStringLiteral("is_focused")] = state.isFocused;
    object[QStringLiteral("is_floating")] = state.isFloating;
    object[QStringLiteral("is_urgent")] = state.isUrgent;
    object[QStringLiteral("layout")] = windowLayoutToJson(state);
    return object;
}

QJsonObject workspaceToJson(const Layout::WorkspaceState &state)
{
    QJsonObject object;
    object[QStringLiteral("id")] = static_cast<qint64>(state.id);
    object[QStringLiteral("idx")] = state.index;
    object[QStringLiteral("name")] = state.name.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(state.name);
    object[QStringLiteral("output")] = state.output;
    object[QStringLiteral("is_urgent")] = state.isUrgent;
    object[QStringLiteral("is_active")] = state.isActive;
    object[QStringLiteral("is_focused")] = state.isFocused;
    object[QStringLiteral("active_window_id")] = optionalWindow(state.activeWindow);
    return object;
}

QJsonObject outputToJson(const Layout::OutputInfo &info)
{
    QJsonObject object;
    object[QStringLiteral("name")] = info.name;
    object[QStringLiteral("description")] = info.makeModelSerial;
    object[QStringLiteral("logical")] = QJsonObject {{QStringLiteral("x"), info.geometry.x()}, {QStringLiteral("y"), info.geometry.y()},
        {QStringLiteral("width"), info.geometry.width()}, {QStringLiteral("height"), info.geometry.height()},
        {QStringLiteral("scale"), info.scale}};
    return object;
}

QJsonObject bindToJson(const Config::Bind &bind)
{
    QJsonObject object;
    object[QStringLiteral("key")] = Config::bindKeyLabel(bind);
    object[QStringLiteral("action")] = actionToJson({bind.action, std::nullopt});
    object[QStringLiteral("title")] = bind.hotkeyOverlayTitle ? QJsonValue(*bind.hotkeyOverlayTitle) : QJsonValue(QJsonValue::Null);
    object[QStringLiteral("hidden")] = bind.hideFromHotkeyOverlay;
    return object;
}

namespace
{

QJsonObject gestureToJson(const QString &device, int fingers, const QString &motion, const QString &action, bool natural)
{
    return {
        {QStringLiteral("device"), device},
        {QStringLiteral("fingers"), fingers},
        {QStringLiteral("motion"), motion},
        {QStringLiteral("action"), action},
        {QStringLiteral("natural"), natural},
    };
}

void appendDeviceGestures(QJsonArray &array, const QString &device, const Config::MultiTouch &touch)
{
    if (!touch.enabled) {
        return;
    }
    if (touch.horizontalSwipe == Config::HorizontalSwipe::ScrollView) {
        array.append(gestureToJson(
            device, touch.swipeFingers, QStringLiteral("swipe-horizontal"), QStringLiteral("scroll-view"), touch.naturalSwipe));
    }
    if (touch.verticalSwipe == Config::VerticalSwipe::SwitchWorkspace) {
        array.append(gestureToJson(
            device, touch.swipeFingers, QStringLiteral("swipe-vertical"), QStringLiteral("switch-workspace"), touch.naturalSwipe));
    }
    if (touch.windowHorizontalSwipe == Config::WindowHorizontalSwipe::ConsumeOrExpel) {
        array.append(gestureToJson(device, touch.windowSwipeFingers, QStringLiteral("window-swipe-horizontal"),
            QStringLiteral("consume-or-expel"), touch.naturalSwipe));
    }
    if (touch.windowVerticalSwipe == Config::WindowVerticalSwipe::MoveToWorkspace) {
        array.append(gestureToJson(device, touch.windowSwipeFingers, QStringLiteral("window-swipe-vertical"),
            QStringLiteral("move-to-workspace"), touch.naturalSwipe));
    }
    if (touch.pinch == Config::PinchAction::ToggleOverview) {
        array.append(
            gestureToJson(device, touch.pinchFingers, QStringLiteral("pinch"), QStringLiteral("toggle-overview"), touch.naturalSwipe));
    }
}

}

QJsonArray gesturesToJson(const Config::Gestures &gestures)
{
    QJsonArray array;
    appendDeviceGestures(array, QStringLiteral("touchpad"), gestures.touchpad);
    appendDeviceGestures(array, QStringLiteral("touchscreen"), gestures.touchscreen);
    if (gestures.touchscreen.enabled && gestures.touchscreen.longPressToMove) {
        QJsonObject press = gestureToJson(QStringLiteral("touchscreen"), 1, QStringLiteral("long-press"), QStringLiteral("move-window"),
            gestures.touchscreen.naturalSwipe);
        press[QStringLiteral("hold-ms")] = gestures.touchscreen.longPressMs;
        array.append(press);
    }
    return array;
}

QJsonObject actionToJson(const ActionRequest &request)
{
    QJsonObject properties;
    for (const auto &[name, value] : request.action.properties) {
        properties[name] = value;
    }
    QJsonObject object;
    object[QStringLiteral("name")] = request.action.name;
    object[QStringLiteral("arguments")] = QJsonArray::fromStringList(request.action.arguments);
    object[QStringLiteral("properties")] = properties;
    object[QStringLiteral("id")] = optionalWindow(request.target);
    return object;
}

std::expected<ActionRequest, QString> actionFromJson(const QJsonObject &object)
{
    ActionRequest request;
    request.action.name = object.value(QStringLiteral("name")).toString();
    if (request.action.name.isEmpty()) {
        return std::unexpected(QStringLiteral("action name is missing"));
    }
    request.action.arguments = jsonStrings(object.value(QStringLiteral("arguments")));
    const QJsonObject properties = object.value(QStringLiteral("properties")).toObject();
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        request.action.properties.append({it.key(), it.value().isString() ? it.value().toString() : it.value().toVariant().toString()});
    }
    const QJsonValue id = object.value(QStringLiteral("id"));
    if (id.isDouble()) {
        request.target = static_cast<Layout::WindowId>(id.toInteger());
    }
    return request;
}

std::expected<ActionRequest, QString> actionFromArguments(const QStringList &arguments)
{
    if (arguments.isEmpty()) {
        return std::unexpected(QStringLiteral("no action given"));
    }
    ActionRequest request;
    request.action.name = arguments.first();
    for (qsizetype i = 1; i < arguments.size(); ++i) {
        const QString flag = flagName(arguments[i]);
        if (flag.isEmpty()) {
            request.action.arguments.append(arguments[i]);
            continue;
        }
        if (i + 1 >= arguments.size()) {
            return std::unexpected(QStringLiteral("missing value for --%1").arg(flag));
        }
        const QString &value = arguments[++i];
        if (flag == QLatin1String("id") || flag == QLatin1String("window-id")) {
            auto id = parseWindowId(value);
            if (!id) {
                return std::unexpected(id.error());
            }
            request.target = *id;
            continue;
        }
        request.action.properties.append({flag, value});
    }
    return request;
}

}
