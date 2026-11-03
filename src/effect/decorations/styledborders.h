#pragma once

#include "decorations/borderlayerpainter.h"
#include "layout/engine/engine.h"

#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QTimer>

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace KWin
{
class EffectWindow;
class Item;
class Window;
class WindowPaintData;
}

namespace Konveyor
{

class AccentColor;

struct ShownBorder
{
    std::unique_ptr<KWin::Item> carrier;
    Config::BorderStyle style;
    double inset = 0.0;
    double width = 0.0;
    double outerReach = 0.0;
    double innerReach = 0.0;
    Config::CornerRadius radius;
    bool paintedLive = false;
    bool sleeping = false;
    std::vector<Render::LayerMotion> motions;
};

struct StyledWindow
{
    QPointer<KWin::Window> window;
    std::array<std::optional<ShownBorder>, 2> parts;
    double paintedAt = -3600.0;
};

class StyledBorders
{
public:
    enum class Part
    {
        Border,
        FocusRing
    };

    struct Placement
    {
        const Config::BorderStyle *style = nullptr;
        double inset = 0.0;
        double width = 0.0;
        Config::CornerRadius radius;
    };

    struct PaintRequest
    {
        const KWin::RenderTarget &renderTarget;
        const KWin::RenderViewport &viewport;
        const KWin::Region &deviceRegion;
        KWin::EffectWindow *window = nullptr;
        const KWin::WindowPaintData &data;
    };

    StyledBorders(const AccentColor &accent, BorderLayerPainter &painter, Render::SignalBus &signalBus);
    ~StyledBorders();

    void setStyles(const QList<Config::BorderStyle> &styles, bool reduceMotion);
    void setOnBattery(bool onBattery);
    const Config::BorderStyle *styleNamed(const QString &name);

    void place(Layout::WindowId id, KWin::Window *window, Part part, const Placement &placement);
    void hide(Layout::WindowId id, Part part);
    void remove(Layout::WindowId id);
    void retainOnly(const QSet<Layout::WindowId> &ids);

    bool isEmpty() const { return m_windows.empty(); }
    void paint(Layout::WindowId id, const PaintRequest &request);
    void scheduleFrames();

private:
    double seconds() const;
    bool holdsStill() const { return m_reduceMotion || m_onBattery; }
    void repaint(StyledWindow &parts, ShownBorder &shown);
    void updateReach(ShownBorder &shown, const Placement &placement);
    struct LayerPass
    {
        ShownBorder &shown;
        const Render::SignalFrame &frameSignals;
        Render::EventScene scene;
        QRectF own;
        double opacity;
        double now;
        bool still;
        bool live;
    };

    QList<BorderLayerPainter::Layer> layersFor(Layout::WindowId id, ShownBorder &shown, QSizeF frame, double opacity, bool &live);
    void appendLayer(QList<BorderLayerPainter::Layer> &layers, LayerPass &pass, std::size_t index);
    static double layerClock(const Config::BorderLayer &layer, const Render::LayerMotion &motion, bool reactive, const LayerPass &pass);
    static std::optional<Render::EventLayerFrame> oneShotFor(
        const Config::BorderLayer &layer, const Config::BorderLayer &config, const Render::LayerMotion &motion, const LayerPass &pass);
    Render::SignalSet wantedFor(const StyledWindow &parts, double now) const;
    void refreshWanted();
    void wake(Layout::WindowId id);

    const AccentColor &m_accent;
    BorderLayerPainter &m_painter;
    Render::SignalBus &m_signalBus;
    QTimer m_wantedExpiry;
    QHash<QString, Config::BorderStyle> m_styles;
    std::map<Layout::WindowId, StyledWindow> m_windows;
    QElapsedTimer m_clock;
    bool m_reduceMotion = false;
    bool m_onBattery = false;
};

QList<QColor> resolvedColors(const AccentColor &accent, const QList<Config::Paint> &paints);
BorderLayerPainter::Target borderPaintTarget(const StyledBorders::PaintRequest &request);
void placeBorderCarrier(std::unique_ptr<KWin::Item> &carrier, KWin::Window *window, double outerReach, double innerReach);

}
