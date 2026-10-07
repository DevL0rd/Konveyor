#include "kwin/desktopsync.h"

#include "kwin/outputregistry.h"
#include "kwin/windowregistry.h"

#include <core/output.h>
#include <virtualdesktops.h>
#include <window.h>
#include <workspace.h>

#include <QMap>
#include <QScopedValueRollback>

#include <algorithm>

namespace Konveyor
{

namespace
{

KWin::VirtualDesktopManager *manager()
{
    return KWin::VirtualDesktopManager::self();
}

KWin::VirtualDesktop *desktopAt(int index)
{
    return manager()->desktops().value(index - 1, nullptr);
}

int indexOf(KWin::VirtualDesktop *desktop)
{
    return static_cast<int>(manager()->desktops().indexOf(desktop)) + 1;
}

int maxWorkspaceIndex(const QList<Layout::WorkspaceState> &workspaces)
{
    int count = 1;
    for (const Layout::WorkspaceState &workspace : workspaces) {
        count = std::max(count, workspace.index);
    }
    return count;
}

QMap<int, QString> activeNames(const QList<Layout::WorkspaceState> &workspaces)
{
    QMap<int, QString> names;
    for (const Layout::WorkspaceState &workspace : workspaces) {
        if (!workspace.isActive) {
            continue;
        }
        QString &name = names[workspace.index];
        if (name.isEmpty() || (workspace.isFocused && !workspace.name.isEmpty())) {
            name = workspace.name;
        }
    }
    return names;
}

}

DesktopSync::DesktopSync(const WindowRegistry &windows, const OutputRegistry &outputs, QObject *parent)
    : QObject(parent)
    , m_windows(windows)
    , m_outputs(outputs)
{ }

DesktopSync::~DesktopSync()
{
    for (auto it = m_originalNames.cbegin(); it != m_originalNames.cend(); ++it) {
        if (KWin::VirtualDesktop *desktop = manager()->desktopForId(it.key())) {
            desktop->setName(it.value());
        }
    }
    restoreDesktops();
    manager()->setPerOutputVirtualDesktops(m_previousPerOutput);
    manager()->setRows(m_previousRows);
}

void DesktopSync::restoreDesktops()
{
    const QScopedValueRollback guard(m_applying, true);
    const auto isOriginal = [this](const KWin::VirtualDesktop *desktop) {
        return std::ranges::any_of(m_previousDesktops, [desktop](const auto &previous) { return previous.first == desktop->id(); });
    };
    for (qsizetype position = 0; position < m_previousDesktops.size(); ++position) {
        const QString &name = m_previousDesktops.at(position).second;
        if (KWin::VirtualDesktop *desktop = manager()->desktops().value(position, nullptr)) {
            if (!isOriginal(desktop) && desktop->name() != name) {
                desktop->setName(name);
            }
        } else {
            manager()->createVirtualDesktop(static_cast<uint>(position), name);
        }
    }
    while (manager()->desktops().size() > std::max<qsizetype>(m_previousDesktops.size(), 1)) {
        manager()->removeVirtualDesktop(manager()->desktops().constLast());
    }
}

void DesktopSync::start()
{
    m_previousPerOutput = manager()->isPerOutputVirtualDesktops();
    m_previousRows = manager()->rows();
    for (const KWin::VirtualDesktop *desktop : manager()->desktops()) {
        m_previousDesktops.append({desktop->id(), desktop->name()});
    }
    manager()->setPerOutputVirtualDesktops(true);
    connect(KWin::workspace(), &KWin::Workspace::currentDesktopChanged, this,
        [this](KWin::VirtualDesktop *previous, KWin::VirtualDesktop *current, KWin::LogicalOutput *output) {
            onCurrentDesktopChanged(previous, current, output);
        });
    connect(&m_windows, &WindowRegistry::windowAdded, this,
        [this](Layout::WindowId id, KWin::Window *window) { watchWindowDesktops(window, id); });
}

void DesktopSync::apply(const QList<Layout::WorkspaceState> &workspaces, const QList<Layout::WindowState> &windows)
{
    const QScopedValueRollback guard(m_applying, true);
    const int count = maxWorkspaceIndex(workspaces);
    growDesktops(count);
    applyWindowDesktops(windows);
    applyCurrentDesktops(workspaces);
    shrinkDesktops(count);
    applyNames(workspaces);
}

void DesktopSync::growDesktops(int count)
{
    while (static_cast<int>(manager()->count()) < count) {
        manager()->createVirtualDesktop(manager()->count());
    }
    manager()->setRows(std::max(manager()->count(), 1U));
}

void DesktopSync::shrinkDesktops(int count)
{
    while (static_cast<int>(manager()->count()) > count) {
        manager()->removeVirtualDesktop(manager()->desktops().constLast());
    }
    manager()->setRows(std::max(manager()->count(), 1U));
}

void DesktopSync::applyNames(const QList<Layout::WorkspaceState> &workspaces)
{
    const QMap<int, QString> names = activeNames(workspaces);
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        KWin::VirtualDesktop *desktop = desktopAt(it.key());
        if (!desktop || (it.value().isEmpty() && !m_originalNames.contains(desktop->id()))) {
            continue;
        }
        if (!m_originalNames.contains(desktop->id())) {
            m_originalNames.insert(desktop->id(), desktop->name());
        }
        const QString name = it.value().isEmpty() ? m_originalNames.take(desktop->id()) : it.value();
        if (desktop->name() != name) {
            desktop->setName(name);
        }
    }
}

void DesktopSync::applyCurrentDesktops(const QList<Layout::WorkspaceState> &workspaces)
{
    for (const Layout::WorkspaceState &workspace : workspaces) {
        if (!workspace.isActive) {
            continue;
        }
        KWin::LogicalOutput *output = m_outputs.outputNamed(workspace.output);
        KWin::VirtualDesktop *desktop = desktopAt(workspace.index);
        if (output && desktop && manager()->currentDesktop(output) != desktop) {
            manager()->setCurrent(desktop, output);
        }
    }
}

void DesktopSync::applyWindowDesktops(const QList<Layout::WindowState> &windows)
{
    for (const Layout::WindowState &state : windows) {
        if (state.onAllDesktops) {
            continue;
        }
        KWin::Window *window = m_windows.windowOf(state.id);
        KWin::VirtualDesktop *desktop = desktopAt(state.workspaceIndex);
        if (window && desktop && window->desktops() != QList<KWin::VirtualDesktop *> {desktop}) {
            window->setDesktops({desktop});
        }
    }
}

void DesktopSync::watchWindowDesktops(KWin::Window *window, Layout::WindowId id)
{
    connect(window, &KWin::Window::desktopsChanged, this, [this, window, id] {
        if (m_applying || m_windows.idOf(window) != id || window->desktops().size() != 1) {
            return;
        }
        Q_EMIT windowMovedToWorkspaceByUser(id, indexOf(window->desktops().constFirst()));
    });
}

void DesktopSync::onCurrentDesktopChanged(KWin::VirtualDesktop *previous, KWin::VirtualDesktop *current, KWin::LogicalOutput *output)
{
    Q_UNUSED(previous)
    if (m_applying || !current || !output) {
        return;
    }
    Q_EMIT workspaceActivatedByUser(output->name(), indexOf(current));
}

}
