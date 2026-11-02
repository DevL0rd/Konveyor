#pragma once

#include "audio/audiosignals.h"
#include "decorations/batterymonitor.h"
#include "decorations/eventeffects.h"
#include "decorations/styledborders.h"
#include "decorations/viewsignals.h"
#include "layout/engine/engine.h"
#include "render/borderimage.h"
#include "render/signals.h"

#include <QObject>
#include <QPointer>
#include <QSet>

#include <functional>
#include <map>
#include <memory>

namespace KWin
{
class ImageItem;
class Item;
class OutlinedBorderItem;
class Window;
}

namespace Konveyor
{

class AccentColor;

class DecorationLayer : public QObject
{
    Q_OBJECT

public:
    explicit DecorationLayer(const AccentColor &accent, QObject *parent = nullptr);
    ~DecorationLayer() override;

    struct DropHintRequest
    {
        QRectF rect;
        Layout::ResolvedPaint paint;
        Config::CornerRadius radius;
        QRectF workspaceView;
        double scale = 1.0;
    };

    void update(KWin::Window *window, const Layout::WindowState &state, const QRectF &workspaceView, double scale);
    void showDropHint(Layout::WindowId carrier, KWin::Window *window, const DropHintRequest &request);
    void hideDropHint();
    void remove(Layout::WindowId id);
    void retainOnly(const QSet<Layout::WindowId> &ids);
    Config::CornerRadius radiusFor(KWin::Window *window, const Config::CornerRadius &fromRules);
    void setBorderStyles(const QList<Config::BorderStyle> &styles, bool reduceMotion);
    void setAudioBeatThreshold(float threshold) { m_audioSignals.setBeatThreshold(threshold); }
    Render::SignalBus &signalBus() { return m_signalBus; }
    BatteryMonitor &battery() { return m_battery; }
    bool paintsBorders() const { return !m_styled.isEmpty() || m_events.isPlaying(); }
    void paintBorders(Layout::WindowId id, const StyledBorders::PaintRequest &request);
    void scheduleBorderFrames();
    void syncEventEffects(const QList<Layout::WindowState> &states, const std::function<KWin::Window *(Layout::WindowId)> &windowOf);
    bool wantsViewStates() const { return m_viewSignals.isWanted(); }
    void updateViewSignals(const QList<Layout::OutputViewState> &views, const QList<Layout::WindowState> &states);
    void fireEvent(Config::EventTrigger trigger, Layout::WindowId id, std::optional<QPointF> point = std::nullopt, QPointF direction = {});
    bool isEventBound(Config::EventTrigger trigger) const { return m_events.isBound(trigger); }

private:
    struct Slot
    {
        std::unique_ptr<KWin::ImageItem> item;
        std::unique_ptr<KWin::OutlinedBorderItem> outline;
        Render::BorderSpec spec;
    };

    struct Decorations
    {
        QPointer<KWin::Window> window;
        Slot focusRing;
        Slot border;
        std::vector<Slot> tabs;
    };

    struct Placement
    {
        Render::BorderSpec spec;
        QPointF position;
        QRectF innerRect;
        double thickness = 0.0;
        int z = 1;
        bool visible = false;
        bool filled = false;
    };

    void apply(Slot &slot, KWin::Item *parent, const Placement &placement);
    static void applyOutlined(Slot &slot, KWin::Item *parent, const Placement &placement);
    static void applyImage(Slot &slot, KWin::Item *parent, const Placement &placement);
    struct OutlineRequest
    {
        QRectF frame;
        double inset = 0.0;
        Config::CornerRadius radius;
        QRectF workspaceView;
        double scale = 1.0;
    };

    struct PartRequest
    {
        StyledBorders::Part part = StyledBorders::Part::Border;
        Slot &slot;
        KWin::Window *window = nullptr;
        OutlineRequest outline;
    };

    void updatePart(const PartRequest &request, const Layout::DecorationState &decoration, const Layout::WindowState &state);
    Placement outline(const OutlineRequest &request, const Layout::DecorationState &decoration, const Layout::WindowState &state) const;
    Placement filled(const QRectF &frame, const QRectF &rect, const Config::CornerRadius &radius, const Layout::ResolvedPaint &paint,
        const QRectF &workspaceView, double scale) const;
    void fillPaint(
        Render::BorderSpec &spec, const Layout::ResolvedPaint &paint, const QRectF &globalRect, const QRectF &workspaceView) const;

    const AccentColor &m_accent;
    double m_systemRadius = 0.0;
    std::map<Layout::WindowId, Decorations> m_decorations;
    std::optional<Layout::WindowId> m_dropHintCarrier;
    Slot m_dropHint;
    BorderLayerPainter m_painter;
    Render::SignalBus m_signalBus;
    Audio::AudioSignals m_audioSignals;
    StyledBorders m_styled;
    EventEffects m_events;
    ViewSignals m_viewSignals;
    BatteryMonitor m_battery;
};

}
