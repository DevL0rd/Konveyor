#include "plugin/konveyoreffect_p.h"

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
    const WindowVisibilityHold hold = readEngine().isOverviewOpen() ? WindowVisibilityHold::AllWindows
        : readEngine().isWorkspaceTransitioning()                   ? WindowVisibilityHold::VisibleWindows
                                                                    : WindowVisibilityHold::None;
    d->visibility.sync(states, hold);
}

void KonveyorEffect::connectOverviewSync()
{
    connect(KWin::effects, &KWin::EffectsHandler::activeFullScreenEffectChanged, this, [this] {
        if (d->slideSuppression) {
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
