#include "plugin/konveyoreffect_p.h"

#include <core/renderviewport.h>
#include <effect/effectwindow.h>

#include <QScopeGuard>

namespace Konveyor
{

void KonveyorEffect::prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *w, KWin::WindowPrePaintData &data)
{
    if (spillHome(w->window()) || isDragged(w->window())) {
        data.setTranslucent();
    }
    KWin::Effect::prePaintWindow(view, w, data);
}

PaintWindowResult KonveyorEffect::paintWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport,
    KWin::EffectWindow *w, int mask, const KWin::Region &deviceRegion, KWin::WindowPaintData &data)
{
    const std::optional<QRectF> home = spillHome(w->window());
    const KWin::Region clipped = home ? deviceRegion & viewport.mapToDeviceCoordinatesAligned(KWin::RectF(*home)) : deviceRegion;
    if (clipped.isEmpty()) {
        return PaintWindowResult(true);
    }
    const auto paintBorders = qScopeGuard([&] {
        if (!d->decorations.paintsBorders()) {
            return;
        }
        if (const std::optional<Layout::WindowId> id = d->windows.idOf(w->window())) {
            d->decorations.paintBorders(*id, {renderTarget, viewport, clipped, w, data});
        }
    });
    return KWin::Effect::paintWindow(renderTarget, viewport, w, mask, clipped, data);
}

void KonveyorEffect::prePaintScreen(KWin::ScreenPrePaintData &data)
{
    KWin::Effect::prePaintScreen(data);
}

void KonveyorEffect::postPaintScreen()
{
    d->decorations.scheduleBorderFrames();
    KWin::Effect::postPaintScreen();
}

}
