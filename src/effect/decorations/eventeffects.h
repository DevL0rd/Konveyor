#pragma once

#include "decorations/styledborders.h"
#include "render/drive.h"
#include "render/eventlayer.h"
#include "render/signals.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QSet>

#include <memory>
#include <optional>
#include <vector>

namespace Konveyor
{

class EventEffects
{
public:
    struct Window
    {
        KWin::Window *window = nullptr;
        const Layout::WindowState *state = nullptr;
        Config::CornerRadius radius;
    };

    EventEffects(const AccentColor &accent, BorderLayerPainter &painter, StyledBorders &styles, Render::SignalBus &bus);
    ~EventEffects();

    void setReduceMotion(bool reduceMotion);
    void setOnBattery(bool onBattery);
    void sync(const QList<Window> &windows);
    void fire(Config::EventTrigger trigger, Layout::WindowId id, std::optional<QPointF> point = std::nullopt, QPointF direction = {});
    void fireOnOutput(Config::EventTrigger trigger, const QString &output);
    bool isBound(Config::EventTrigger trigger) const;
    void remove(Layout::WindowId id);
    void paint(Layout::WindowId id, const StyledBorders::PaintRequest &request);
    void scheduleFrames();
    bool isPlaying() const { return !m_plays.empty(); }

private:
    struct Known
    {
        QPointer<KWin::Window> window;
        Layout::WindowId id = 0;
        QString output;
        Layout::WorkspaceId workspace = 0;
        int column = 0;
        int tile = 0;
        bool floating = false;
        bool fullscreen = false;
        bool shown = false;
        bool focused = false;
        bool urgent = false;
        double inset = 0.0;
        double ringWidth = 0.0;
        Config::CornerRadius radius;
        Config::EventEffects effects;
    };

    struct Target
    {
        Layout::WindowId id = 0;
        QPointer<KWin::Window> window;
        std::unique_ptr<KWin::Item> carrier;
        bool waveOnly = false;
        double delay = 0.0;
        double width = 0.0;
        double inset = 0.0;
        double outerReach = 0.0;
        double innerReach = 0.0;
        Config::CornerRadius radius;
        std::vector<Render::LayerMotion> motions;
    };

    struct Play
    {
        Config::EventTrigger trigger = Config::EventTrigger::Focus;
        Config::EventMode mode = Config::EventMode::Once;
        Layout::WindowId subject = 0;
        Config::BorderStyle style;
        Render::EventScene scene;
        QPointer<KWin::Window> subjectWindow;
        QPointer<KWin::Window> fromWindow;
        QPointer<KWin::Window> toWindow;
        double startedAt = 0.0;
        double lastFired = 0.0;
        double duration = 0.0;
        std::vector<Target> targets;
    };

    struct Pair
    {
        const Known *from = nullptr;
        const Known *to = nullptr;
        std::optional<QPointF> point;
    };

    double seconds() const;
    bool holdsStill() const { return m_reduceMotion || m_onBattery; }
    const Known *known(Layout::WindowId id) const;
    void syncFocus(const std::optional<Layout::WindowId> &focused);
    void syncUrgent();
    void updateWanted();
    void dropOutdatedPlays();
    void cancelPlays(Config::EventTrigger trigger);
    void start(Config::EventTrigger trigger, const Known &subject, const Pair &pair);
    struct LayerPass
    {
        const Play &play;
        Target &target;
        const QRectF &frame;
        double opacity;
        double standing;
        double now;
        double elapsed;
    };

    static bool sharesSpace(const Known &window, const Known &subject);
    static bool reaches(
        const Known &window, const Known &subject, const Config::EventEffect &effect, const QRectF &own, const QRectF &frame);
    void addTargets(Play &play, const Known &subject, const Config::EventEffect &effect);
    void placeTarget(Target &target, const Known &window, const Play &play) const;
    void refreshTargets();
    double endsAt(const Play &play) const;
    QList<BorderLayerPainter::Layer> layersFor(const Play &play, Target &target, const QRectF &frame, double opacity);
    void appendLayer(QList<BorderLayerPainter::Layer> &layers, const LayerPass &pass, qsizetype index);
    void appendOneShot(
        QList<BorderLayerPainter::Layer> &layers, const LayerPass &pass, const Config::BorderLayer &layer, BorderLayerPainter::Layer drawn);
    void updateClaims();

    const AccentColor &m_accent;
    BorderLayerPainter &m_painter;
    StyledBorders &m_styles;
    Render::SignalBus &m_bus;
    QElapsedTimer m_clock;
    std::vector<Play> m_plays;
    std::vector<Known> m_known;
    QSet<Layout::WindowId> m_urgent;
    std::optional<Layout::WindowId> m_focused;
    QSet<Layout::WindowId> m_claimed;
    bool m_seeded = false;
    bool m_focusLost = false;
    bool m_reduceMotion = false;
    bool m_onBattery = false;
};

}
