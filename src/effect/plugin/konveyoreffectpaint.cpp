#include "plugin/konveyoreffect_p.h"

#include <core/renderviewport.h>
#include <effect/effectwindow.h>

#include <QScopeGuard>

#include <algorithm>

namespace Konveyor
{

void KonveyorEffect::prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *w, KWin::WindowPrePaintData &data)
{
    const bool overviewWindow = overviewPaintActive() && d->windows.idOf(w->window());
    if (spillHome(w->window()) || isDragged(w->window()) || overviewWindow) {
        data.setTranslucent();
        if (overviewWindow) {
            data.mask |= PAINT_WINDOW_TRANSFORMED;
        }
    }
    KWin::Effect::prePaintWindow(view, w, data);
}

PaintWindowResult KonveyorEffect::paintWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport,
    KWin::EffectWindow *w, int mask, const KWin::Region &deviceRegion, KWin::WindowPaintData &data)
{
    if (d->drawingOverviewThumbnail) {
        return KWin::Effect::paintWindow(renderTarget, viewport, w, mask, deviceRegion, data);
    }
    if (overviewPaintActive() && d->windows.idOf(w->window())) {
        return PaintWindowResult(true);
    }
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
    if (overviewPaintActive()) {
        data.mask |= PAINT_SCREEN_TRANSFORMED | PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }
    KWin::Effect::prePaintScreen(data);
}

#ifdef KONVEYOR_KWIN_PAINTSCREEN_BOOL
bool KonveyorEffect::paintScreen(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, int mask,
    const KWin::Region &deviceRegion, KWin::LogicalOutput *screen)
{
    if (!overviewPaintActive() || !screen) {
        return KWin::Effect::paintScreen(renderTarget, viewport, mask, deviceRegion, screen);
    }
    const std::optional<Layout::OverviewView> view = readEngine().overviewView(screen->name());
    if (!view) {
        return KWin::Effect::paintScreen(renderTarget, viewport, mask, deviceRegion, screen);
    }
    if (!KWin::Effect::paintScreen(renderTarget, viewport, mask, deviceRegion, screen)) {
        return false;
    }
    paintOverview(renderTarget, viewport, deviceRegion, *view, KWin::effects->clientArea(KWin::MaximizeArea, screen));
    return true;
}
#else
void KonveyorEffect::paintScreen(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, int mask,
    const KWin::Region &deviceRegion, KWin::LogicalOutput *screen)
{
    KWin::Effect::paintScreen(renderTarget, viewport, mask, deviceRegion, screen);
    if (!overviewPaintActive() || !screen) {
        return;
    }
    const std::optional<Layout::OverviewView> view = readEngine().overviewView(screen->name());
    if (!view) {
        return;
    }
    paintOverview(renderTarget, viewport, deviceRegion, *view, KWin::effects->clientArea(KWin::MaximizeArea, screen));
}
#endif

void KonveyorEffect::postPaintScreen()
{
    d->decorations.scheduleBorderFrames();
    KWin::Effect::postPaintScreen();
    if (overviewPaintActive() && d->animating) {
        KWin::effects->addRepaintFull();
    }
}

}
