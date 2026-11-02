#pragma once

#include "config/types.h"
#include "render/borderlayer.h"
#include "render/drive.h"
#include "render/eventlayer.h"

#include <QColor>
#include <QList>
#include <QMatrix4x4>

#include <core/region.h>

#include <memory>
#include <optional>

namespace KWin
{
class GLShader;
class RenderTarget;
class RenderViewport;
}

namespace Konveyor
{

class BorderLayerPainter
{
public:
    BorderLayerPainter();
    ~BorderLayerPainter();

    struct Target
    {
        const KWin::RenderTarget &renderTarget;
        const KWin::RenderViewport &viewport;
        const KWin::Region &deviceRegion;
        QMatrix4x4 transform;
    };

    struct Layer
    {
        const Config::BorderLayer *config = nullptr;
        QList<QColor> colors;
        double phase = 0.0;
        double opacity = 1.0;
        double seconds = 0.0;
        const Render::LayerMotion *motion = nullptr;
        std::optional<Render::EventLayerFrame> event;
    };

    bool load();
    bool loadReactive();
    void draw(const Target &target, const Render::BorderLayerShape &shape, const QList<Layer> &layers);

private:
    void drawBand(const Target &target, const KWin::Region &scissor, const Render::BorderLayerShape &shape, const Layer &layer);
    void drawReactive(const Target &target, const KWin::Region &scissor, const Render::BorderLayerShape &shape, const Layer &layer);
    void drawParticles(const Target &target, const KWin::Region &scissor, const Render::BorderLayerShape &shape, const Layer &layer);

    std::unique_ptr<KWin::GLShader> m_band;
    std::unique_ptr<KWin::GLShader> m_particle;
    std::unique_ptr<KWin::GLShader> m_reactive;
    bool m_failed = false;
    bool m_reactiveFailed = false;
};

}
