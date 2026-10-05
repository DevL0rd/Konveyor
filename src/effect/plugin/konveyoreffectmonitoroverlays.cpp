#include "plugin/konveyoreffect_p.h"

#include <functional>

namespace Konveyor
{

namespace
{

void attachMonitorWindow(KWin::Window *window, KWin::Window *target)
{
    if (!window || !target || window == target) {
        return;
    }
    if (KWin::Window *previous = window->transientFor(); previous && previous != target) {
        previous->removeTransient(window);
    }
    if (!target->transients().contains(window)) {
        target->addTransient(window);
    }
    if (window->transientFor() != target) {
        window->setTransientFor(target);
    }
    window->setSkipTaskbar(true);
    window->setSkipPager(true);
    window->setSkipSwitcher(true);
}

void detachMonitorWindow(KWin::Window *window)
{
    if (window) {
        if (KWin::Window *target = window->transientFor()) {
            target->removeTransient(window);
        }
    }
}

using MonitorWindowMap = QHash<Layout::WindowId, QHash<int, QPointer<KWin::Window>>>;

bool removeMonitorWindow(MonitorWindowMap &windows, KWin::Window *window, const std::function<void(Layout::WindowId)> &place)
{
    bool removed = false;
    for (auto target = windows.begin(); target != windows.end();) {
        auto &slotMap = target.value();
        for (auto slot = slotMap.begin(); slot != slotMap.end();) {
            if (slot.value() == window) {
                slot = slotMap.erase(slot);
                removed = true;
            } else {
                ++slot;
            }
        }
        if (slotMap.isEmpty()) {
            target = windows.erase(target);
        } else {
            const Layout::WindowId id = target.key();
            ++target;
            place(id);
        }
    }
    return removed;
}

}

void KonveyorEffect::observeMonitorOverlay(KWin::Window *window)
{
    static const QRegularExpression pattern(QStringLiteral("^Konveyor Monitor (Overlay|Panel) (\\d+) ([0-2])$"));
    if (!window) {
        return;
    }
    const QRegularExpressionMatch match = pattern.match(window->caption());
    if (match.hasMatch()) {
        const bool panel = match.captured(1) == QLatin1String("Panel");
        const Layout::WindowId target = match.captured(2).toULongLong();
        const int slot = match.captured(3).toInt();
        const auto &windows = panel ? d->monitorPanels : d->monitorOverlays;
        if (windows.value(target).value(slot) == window) {
            return;
        }
    }
    forgetMonitorOverlay(window);
    if (!match.hasMatch()) {
        return;
    }
    const bool panel = match.captured(1) == QLatin1String("Panel");
    const Layout::WindowId target = match.captured(2).toULongLong();
    const int slot = match.captured(3).toInt();
    KWin::Window *targetWindow = d->windows.windowOf(target);
    if (!targetWindow) {
        return;
    }
    attachMonitorWindow(window, targetWindow);
    if (panel) {
        d->monitorPanels[target].insert(slot, window);
        connect(window, &KWin::Window::frameGeometryChanged, this, [this, target] { placeMonitorPanels(target); });
        placeMonitorPanels(target);
    } else {
        d->monitorOverlays[target].insert(slot, window);
        connect(window, &KWin::Window::frameGeometryChanged, this, [this, target] { placeMonitorOverlays(target); });
        placeMonitorOverlays(target);
    }
}

void KonveyorEffect::forgetMonitorOverlay(KWin::Window *window)
{
    bool forgotten = removeMonitorWindow(d->monitorOverlays, window, [this](Layout::WindowId id) { placeMonitorOverlays(id); });
    forgotten = removeMonitorWindow(d->monitorPanels, window, [this](Layout::WindowId id) { placeMonitorPanels(id); }) || forgotten;
    if (forgotten) {
        detachMonitorWindow(window);
    }
}

void KonveyorEffect::placeMonitorOverlays(Layout::WindowId id)
{
    if (d->placingMonitorOverlays.contains(id)) {
        return;
    }
    KWin::Window *target = d->windows.windowOf(id);
    const auto overlays = d->monitorOverlays.value(id);
    if (!target || overlays.isEmpty()) {
        return;
    }
    d->placingMonitorOverlays.insert(id);
    qreal width = 0;
    for (int slot = 0; slot < 3; ++slot) {
        if (KWin::Window *overlay = overlays.value(slot)) {
            width += overlay->frameGeometry().width();
        }
    }
    qreal x = target->frameGeometry().x() + (target->frameGeometry().width() - width) / 2;
    const qreal y = target->frameGeometry().y();
    for (int slot = 0; slot < 3; ++slot) {
        if (KWin::Window *overlay = overlays.value(slot)) {
            const QPointF position(qRound(x), qRound(y));
            if (overlay->frameGeometry().topLeft() != position) {
                overlay->move(position);
            }
            x += overlay->frameGeometry().width();
        }
    }
    d->placingMonitorOverlays.remove(id);
}

void KonveyorEffect::placeMonitorPanels(Layout::WindowId id)
{
    if (d->placingMonitorPanels.contains(id)) {
        return;
    }
    KWin::Window *target = d->windows.windowOf(id);
    const auto panels = d->monitorPanels.value(id);
    if (!target || panels.isEmpty()) {
        return;
    }
    d->placingMonitorPanels.insert(id);
    for (KWin::Window *panel : panels) {
        if (!panel) {
            continue;
        }
        const QPointF position(qRound(target->frameGeometry().x() + (target->frameGeometry().width() - panel->frameGeometry().width()) / 2),
            qRound(target->frameGeometry().y() + (target->frameGeometry().height() - panel->frameGeometry().height()) / 2));
        if (panel->frameGeometry().topLeft() != position) {
            panel->move(position);
        }
    }
    d->placingMonitorPanels.remove(id);
}

}
