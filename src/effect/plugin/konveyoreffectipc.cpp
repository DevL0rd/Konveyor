#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

QJsonDocument KonveyorEffect::windowsJson() const
{
    QJsonArray array;
    const QList<Layout::WindowState> states = readEngine().windowStates();
    for (const Layout::WindowState &state : states) {
        if (KWin::Window *window = d->windows.windowOf(state.id)) {
            array.append(Ipc::windowToJson({state.id, window->caption(), window->resourceClass(), window->pid()}, state));
        }
    }
    return QJsonDocument(array);
}

QJsonDocument KonveyorEffect::workspacesJson() const
{
    QJsonArray array;
    const QList<Layout::WorkspaceState> states = readEngine().workspaceStates();
    for (const Layout::WorkspaceState &state : states) {
        array.append(Ipc::workspaceToJson(state));
    }
    return QJsonDocument(array);
}

QJsonDocument KonveyorEffect::outputsJson() const
{
    QJsonArray array;
    const QList<Layout::OutputInfo> infos = d->outputs.outputs();
    for (const Layout::OutputInfo &info : infos) {
        array.append(Ipc::outputToJson(info));
    }
    return QJsonDocument(array);
}

QJsonDocument KonveyorEffect::focusedWindowJson() const
{
    const std::optional<Layout::WindowId> id = readEngine().focusedWindow();
    KWin::Window *window = id ? d->windows.windowOf(*id) : nullptr;
    const std::optional<Layout::WindowState> state = id ? readEngine().windowState(*id) : std::nullopt;
    if (!window || !state) {
        return QJsonDocument();
    }
    return QJsonDocument(Ipc::windowToJson({*id, window->caption(), window->resourceClass(), window->pid()}, *state));
}

QJsonDocument KonveyorEffect::focusedOutputJson() const
{
    QJsonArray array;
    const std::optional<QString> name = readEngine().focusedOutput();
    const QList<Layout::OutputInfo> infos = d->outputs.outputs();
    for (const Layout::OutputInfo &info : infos) {
        if (name && info.name == *name) {
            array.append(Ipc::outputToJson(info));
        }
    }
    return QJsonDocument(array);
}

QJsonDocument KonveyorEffect::bindsJson() const
{
    QJsonArray array;
    const QList<Config::Bind> binds = d->shortcuts.binds();
    for (const Config::Bind &bind : binds) {
        array.append(Ipc::bindToJson(bind));
    }
    return QJsonDocument(array);
}

QString KonveyorEffect::performActionJson(const QString &json)
{
    const auto request = Ipc::actionFromJson(QJsonDocument::fromJson(json.toUtf8()).object());
    if (!request) {
        return request.error();
    }
    const Layout::ActionResult result = changeEngine().perform(request->action, request->target);
    return result.ok ? QString() : result.error;
}

QJsonDocument KonveyorEffect::lastBindJson() const
{
    QJsonObject object;
    object[QStringLiteral("count")] = static_cast<double>(d->bindCount);
    object[QStringLiteral("key")] = d->lastBindKey;
    object[QStringLiteral("action")] = d->lastBindAction;
    return QJsonDocument(object);
}

}
