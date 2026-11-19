#include "plugin/konveyoreffect_p.h"

#include <core/renderviewport.h>
#include <effect/effectwindow.h>

#include <QScopeGuard>

#include <algorithm>

namespace Konveyor
{

namespace
{

constexpr unsigned kOverviewScreenMask = static_cast<unsigned>(KWin::Effect::PAINT_SCREEN_TRANSFORMED)
    | static_cast<unsigned>(KWin::Effect::PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS);
constexpr unsigned kAllWindowsMask
    = static_cast<unsigned>(KWin::Effect::PAINT_WINDOW_OPAQUE) | static_cast<unsigned>(KWin::Effect::PAINT_WINDOW_TRANSLUCENT);

}

void KonveyorEffect::prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *w, KWin::WindowPrePaintData &data)
{
    const bool overviewWindow = overviewPaintActive() && d->windows.idOf(w->window());
    if (spillHome(w->window()) || isDragged(w->window()) || overviewWindow) {
        data.setTranslucent();
        if (overviewWindow) {
            data.setTransformed();
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
    if (overviewPaintActive() && !d->drawingKontrolPanel && w->window()->resourceClass() == QLatin1String("konveyor-kontrol-panel")) {
        return PaintWindowResult(true);
    }
    if (overviewPaintActive() && (w->isPopupWindow() || w->isMenu())) {
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
        data.mask = static_cast<int>(static_cast<unsigned>(data.mask) | kOverviewScreenMask);
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
    paintOverviewPass(renderTarget, viewport, deviceRegion, screen, *view);
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
    paintOverviewPass(renderTarget, viewport, deviceRegion, screen, *view);
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

void KonveyorEffect::paintOverviewPass(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport,
    const KWin::Region &deviceRegion, KWin::LogicalOutput *screen, const Layout::OverviewView &view)
{
    paintOverview(renderTarget, viewport, deviceRegion, view, KWin::effects->clientArea(KWin::MaximizeArea, screen));
    if (d->config.config().overview.allowKontrolPanel && !d->kontrolPanelWindows.isEmpty()) {
        d->drawingKontrolPanel = true;
        const auto resetDrawing = qScopeGuard([this] { d->drawingKontrolPanel = false; });
        for (KWin::Window *panelWindow : std::as_const(d->kontrolPanelWindows)) {
            if (KWin::EffectWindow *effectWindow = KWin::effects->findWindow(panelWindow->internalId())) {
                KWin::WindowPaintData data;
#ifdef KONVEYOR_KWIN_PAINTSCREEN_BOOL
                const bool ok = KWin::effects->drawWindow(
                    renderTarget, viewport, effectWindow, static_cast<int>(kAllWindowsMask), deviceRegion, data);
                Q_UNUSED(ok);
#else
                KWin::effects->drawWindow(renderTarget, viewport, effectWindow, static_cast<int>(kAllWindowsMask), deviceRegion, data);
#endif
            }
        }
    }
    paintOverviewCursor(renderTarget, viewport, deviceRegion, view);
}
}
