#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>

#include <functional>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class WindowRegistry : public QObject
{
    Q_OBJECT

public:
    explicit WindowRegistry(QObject *parent = nullptr);

    void start();
    void setWantsWindow(std::function<bool(const Layout::WindowProperties &)> wantsWindow);
    void reevaluate();

    static bool isManageable(KWin::Window *window);
    Layout::WindowProperties propertiesOf(KWin::Window *window) const;

    std::optional<Layout::WindowId> idOf(KWin::Window *window) const;
    KWin::Window *windowOf(Layout::WindowId id) const;

Q_SIGNALS:
    void windowAdded(Layout::WindowId id, KWin::Window *window);
    void windowRemoved(Layout::WindowId id);
    void propertiesChanged(Layout::WindowId id);
    void sizeCommitted(Layout::WindowId id, const QSizeF &size);
    void fullscreenRequested(Layout::WindowId id, bool fullscreen);
    void maximizeRequested(Layout::WindowId id, bool maximized);
    void urgencyChanged(Layout::WindowId id, bool urgent);
    void appearanceChanged(Layout::WindowId id);
    void activeWindowChanged();
    void interactiveStarted(Layout::WindowId id, bool isMove);
    void interactiveStepped(Layout::WindowId id, bool isMove);
    void interactiveFinished(Layout::WindowId id, bool isMove);

private:
    void observe(KWin::Window *window);
    void forget(KWin::Window *window);
    void refresh(KWin::Window *window);
    void add(KWin::Window *window);
    void remove(KWin::Window *window);
    void connectObserved(KWin::Window *window);
    void connectWindow(KWin::Window *window, Layout::WindowId id);
    void connectInteractiveSignals(KWin::Window *window, Layout::WindowId id);

    QSet<KWin::Window *> m_observed;
    QHash<KWin::Window *, Layout::WindowId> m_ids;
    QHash<Layout::WindowId, QPointer<KWin::Window>> m_windows;
    Layout::WindowId m_nextId = 1;
    std::function<bool(const Layout::WindowProperties &)> m_wantsWindow;
    bool m_adopting = false;
};

}
