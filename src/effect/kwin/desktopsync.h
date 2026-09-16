#pragma once

#include "layout/engine/engine.h"

#include <QObject>

namespace KWin
{
class LogicalOutput;
class VirtualDesktop;
class Window;
}

namespace Konveyor
{

class OutputRegistry;
class WindowRegistry;

class DesktopSync : public QObject
{
    Q_OBJECT

public:
    DesktopSync(const WindowRegistry &windows, const OutputRegistry &outputs, QObject *parent = nullptr);
    ~DesktopSync() override;

    void start();
    void apply(const QList<Layout::WorkspaceState> &workspaces, const QList<Layout::WindowState> &windows);

Q_SIGNALS:
    void workspaceActivatedByUser(const QString &output, int index);
    void windowMovedToWorkspaceByUser(Layout::WindowId id, int index);

private:
    void growDesktops(int count);
    void shrinkDesktops(int count);
    void applyNames(const QList<Layout::WorkspaceState> &workspaces);
    void applyCurrentDesktops(const QList<Layout::WorkspaceState> &workspaces);
    void applyWindowDesktops(const QList<Layout::WindowState> &windows);
    void watchWindowDesktops(KWin::Window *window, Layout::WindowId id);
    void onCurrentDesktopChanged(KWin::VirtualDesktop *previous, KWin::VirtualDesktop *current, KWin::LogicalOutput *output);

    const WindowRegistry &m_windows;
    const OutputRegistry &m_outputs;
    bool m_applying = false;
    bool m_previousPerOutput = false;
    uint m_previousRows = 1;
};

}
