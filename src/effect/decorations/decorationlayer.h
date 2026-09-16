#pragma once

#include "layout/engine/engine.h"
#include "render/borderimage.h"

#include <QObject>
#include <QPointer>
#include <QSet>

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

    void update(KWin::Window *window, const Layout::WindowState &state, const QRectF &workspaceView, double scale);
    void remove(Layout::WindowId id);
    void retainOnly(const QSet<Layout::WindowId> &ids);
    Config::CornerRadius radiusFor(KWin::Window *window, const Config::CornerRadius &fromRules);

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
        bool visible = false;
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

    Placement outline(const OutlineRequest &request, const Layout::DecorationState &decoration, const Layout::WindowState &state) const;
    Placement tab(
        const QRectF &frame, const QRectF &rect, const Layout::ResolvedPaint &paint, const QRectF &workspaceView, double scale) const;
    void fillPaint(
        Render::BorderSpec &spec, const Layout::ResolvedPaint &paint, const QRectF &globalRect, const QRectF &workspaceView) const;

    const AccentColor &m_accent;
    double m_systemRadius = 0.0;
    std::map<Layout::WindowId, Decorations> m_decorations;
};

}
