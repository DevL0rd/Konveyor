#include "plugin/konveyoreffect_p.h"

#include <workspace.h>

#include <QMetaObject>

namespace Konveyor
{

namespace
{

KWin::Effect *kdeOverview()
{
    return KWin::effects->findEffect(QStringLiteral("overview"));
}

}

void KonveyorEffect::showKdeOverview(bool open)
{
    if (readEngine().isOverviewEnabled()) {
        if (open) {
            if (KWin::workspace()) {
                for (KWin::Window *window : KWin::workspace()->windows()) {
                    if (window && !window->isDeleted() && (window->isPopupWindow() || window->isMenu())) {
                        window->popupDone();
                    }
                }
            }
            if (KWin::effects && d->kontrolPanelWindows.isEmpty()) {
                KWin::effects->startMouseInterception(this, Qt::ArrowCursor);
            }
        } else {
            if (KWin::effects && KWin::effects->isMouseInterception()) {
                KWin::effects->stopMouseInterception(this);
            }
        }
        updateWindowVisibility(readEngine().windowStates());
        scheduleFlush();
        KWin::effects->addRepaintFull();
        return;
    }
    KWin::Effect *overview = kdeOverview();
    if (!overview) {
        qWarning() << "konveyor: KDE's Overview effect is not loaded, so the overview can't" << (open ? "open" : "close");
        return;
    }
    if (open) {
        updateWindowVisibility(readEngine().windowStates());
    }
    QMetaObject::invokeMethod(overview, open ? "activate" : "deactivate");
    if (!open) {
        updateWindowVisibility(readEngine().windowStates());
    }
}

void KonveyorEffect::updateWindowVisibility(const QList<Layout::WindowState> &states)
{
    const WindowVisibilityHold hold = (readEngine().isOverviewOpen() || overviewPaintActive()) ? WindowVisibilityHold::AllWindows
        : readEngine().isWorkspaceTransitioning()                                            ? WindowVisibilityHold::VisibleWindows
                                                                                             : WindowVisibilityHold::None;
    d->visibility.sync(states, hold);
}

void KonveyorEffect::connectOverviewSync()
{
    connect(KWin::effects, &KWin::EffectsHandler::activeFullScreenEffectChanged, this, [this] {
        if (d->slideSuppression || readEngine().isOverviewEnabled()) {
            return;
        }
        const KWin::Effect *overview = kdeOverview();
        const bool shown = overview && KWin::effects->activeFullScreenEffect() == overview;
        if (readEngine().isOverviewOpen() != shown) {
            changeEngine().setOverviewOpen(shown);
        }
    });
}

}
