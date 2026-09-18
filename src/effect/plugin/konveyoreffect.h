#pragma once

#include "config/types.h"
#include "layout/engine/engine.h"

#include <effect/effect.h>

#include <QJsonDocument>

#include <memory>
#include <optional>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class AccentColor;
class ConfigManager;
class DBusService;
class DecorationLayer;
class DesktopSync;
class InputFilter;
class OutputRegistry;
class ShortcutManager;
class WindowApplier;
class WindowRegistry;

class KonveyorEffect : public KWin::Effect
{
    Q_OBJECT

public:
    KonveyorEffect();
    ~KonveyorEffect() override;

    bool isActive() const override;
    bool borderActivated(KWin::ElectricBorder border) override;
    void prePaintScreen(KWin::ScreenPrePaintData &data) override;
    void postPaintScreen() override;
    void prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *w, KWin::WindowPrePaintData &data) override;
    void paintWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, KWin::EffectWindow *w, int mask,
        const KWin::Region &deviceRegion, KWin::WindowPaintData &data) override;
    bool blocksDirectScanout() const override;

private:
    void installInputFilter();
    void startDBusService();
    void connectRegistries();
    void connectWindowLifecycle();
    void connectWindowState();
    void handleWindowSizeCommitted(Layout::WindowId id, const QSizeF &size);
    void connectOutputs();
    void connectDragAndDrop();
    QJsonDocument windowsJson() const;
    QJsonDocument workspacesJson() const;
    QJsonDocument outputsJson() const;
    QJsonDocument focusedWindowJson() const;
    QJsonDocument focusedOutputJson() const;
    QJsonDocument bindsJson() const;
    QJsonDocument lastBindJson() const;
    void applyHotCorners(const Config::HotCorners &corners);
    void handlePointerMotion(const QPointF &position, qint64 timestampMs);
    QPointF interactionPoint() const;
    bool routeGesture(bool consumed);
    bool handleTouchDown(qint32 id, const QPointF &position, qint64 timestampMs);
    bool handleTouchMotion(qint32 id, const QPointF &position, qint64 timestampMs);
    bool handleTouchUp(qint32 id, qint64 timestampMs);
    bool holdsToDecide(Layout::WindowId id, int phase);
    void decidePendingTouchMove(qint64 timestampMs);
    void showKdeOverview(bool open);
    void connectOverviewSync();
    void followActiveWindow();
    void applyFocusRequest();
    void focusWindowUnderPointer(const QPointF &position);
    void warpPointerTo(Layout::WindowId id);
    bool isFloatingWindow(Layout::WindowId id) const;
    bool adoptFloatingGeometry(Layout::WindowId id, KWin::Window *window);
    void handleTitlebarDrag(Layout::WindowId id, KWin::Window *window, int phase);
    void endTitlebarDrag();
    void handleWindowMove(Layout::WindowId id, KWin::Window *window, int phase);
    void handleWindowResize(Layout::WindowId id, KWin::Window *window, int phase);
    void connectDesktopSync();
    void applyConfig(const Config::Config &config);
    void onWindowAdded(Layout::WindowId id, KWin::Window *window);
    void onInteractive(Layout::WindowId id, bool isMove, int phase);
    void flush();
    void stepAnimations();
    Layout::Engine &changeEngine();
    const Layout::Engine &readEngine() const;
    void acknowledgeSettledModeChanges(const QList<Layout::WindowState> &states);
    void updateDecorations(const QList<Layout::WindowState> &states);
    void updateHomeOutputs(const QList<Layout::WindowState> &states);
    std::optional<QRectF> spillHome(KWin::Window *window) const;
    bool hasSpill() const;
    void scheduleFlush();
    Layout::Hooks makeHooks();
    Layout::ActionResult performAction(const Config::Action &action, std::optional<Layout::WindowId> target = std::nullopt);
    Layout::ActionResult toggleForceResizable(std::optional<Layout::WindowId> target);
    QString performActionJson(const QString &json);

    struct Private;
    std::unique_ptr<Private> d;
};

}
