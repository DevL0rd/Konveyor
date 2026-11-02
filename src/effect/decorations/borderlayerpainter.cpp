#include "decorations/borderlayerpainter.h"

#include "config/log.h"
#include "decorations/bordershaders.h"

#include <core/rendertarget.h>
#include <core/renderviewport.h>
#include <effect/effecthandler.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>
#include <opengl/glvertexbuffer.h>
#include <scene/itemgeometry.h>

#include <QVector2D>
#include <QVector4D>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace Konveyor
{

namespace
{

constexpr std::array<const char *, Config::kBorderLayerMaxColors> kColorUniforms {
    "colors[0]", "colors[1]", "colors[2]", "colors[3]", "colors[4]", "colors[5]", "colors[6]", "colors[7]"};

void applyBlend(Config::BorderBlend blend)
{
    switch (blend) {
    case Config::BorderBlend::Normal:
    case Config::BorderBlend::Add:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case Config::BorderBlend::Screen:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        break;
    case Config::BorderBlend::Multiply:
        glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

KWin::Region scissorRegion(const BorderLayerPainter::Target &target)
{
    const KWin::RenderTarget &renderTarget = target.renderTarget;
    const QPoint offset = target.viewport.renderOffset();
    const QSize bufferOffset = renderTarget.transform().map(QSize(offset.x(), offset.y()));
    const KWin::Rect bounds(QPoint(bufferOffset.width(), bufferOffset.height()), renderTarget.size() - 2 * bufferOffset);
    return KWin::Region(bounds)
        & target.viewport.transform().map(target.deviceRegion & renderTarget.transformedRect(), renderTarget.transformedSize());
}

void uploadAndRender(const std::vector<Render::BorderVertex> &vertices, const KWin::Region &scissor)
{
    static constexpr std::array<KWin::GLVertexAttrib, 2> layout {
        KWin::GLVertexAttrib {KWin::VA_Position, 2, GL_FLOAT, offsetof(Render::BorderVertex, position)},
        KWin::GLVertexAttrib {KWin::VA_TexCoord, 4, GL_FLOAT, offsetof(Render::BorderVertex, attributes)},
    };
    KWin::GLVertexBuffer *vbo = KWin::GLVertexBuffer::streamingBuffer();
    vbo->reset();
    vbo->setAttribLayout(std::span<const KWin::GLVertexAttrib>(layout), sizeof(Render::BorderVertex));
    vbo->setData(vertices.data(), vertices.size() * sizeof(Render::BorderVertex));
    vbo->setVertexCount(static_cast<int>(vertices.size()));
    vbo->render(scissor, GL_TRIANGLES, true);
}

void setCommonUniforms(KWin::GLShader &shader, const BorderLayerPainter::Layer &layer, const BorderLayerPainter::Target &target,
    const Render::BorderLayerShape &shape)
{
    const Config::CornerRadius &radius = shape.radius;
    shader.setUniform("halfSize", QVector2D(shape.size.width() / 2.0, shape.size.height() / 2.0));
    shader.setUniform("radii", QVector4D(radius.topLeft, radius.topRight, radius.bottomRight, radius.bottomLeft));
    shader.setUniform("pixels", static_cast<float>(target.viewport.scale()));
    const qsizetype count = std::min<qsizetype>(layer.colors.size(), Config::kBorderLayerMaxColors);
    for (qsizetype i = 0; i < count; ++i) {
        const QColor &color = layer.colors[i];
        shader.setUniform(
            kColorUniforms[static_cast<std::size_t>(i)], QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
    }
    shader.setUniform("colorCount", static_cast<int>(count));
    shader.setUniform("colorMix", Render::colorMixUniform(layer.motion));
    shader.setUniform("additive", Render::additiveUniform(layer.config->blend));
    shader.setUniform("layerOpacity", static_cast<float>(layer.opacity));
    shader.setUniform(KWin::GLShader::Mat4Uniform::ModelViewProjectionMatrix, target.viewport.projectionMatrix() * target.transform);
    shader.setColorspaceUniforms(KWin::ColorDescription::sRGB, target.renderTarget.colorDescription(), KWin::RenderingIntent::Perceptual);
}

void setReactiveUniforms(KWin::GLShader &shader, const Render::LayerMotion &motion)
{
    static constexpr int kBandVectors = static_cast<int>(sizeof(motion.bands) / sizeof(QVector4D));
    static constexpr int kWaveVectors = static_cast<int>(sizeof(motion.wave) / sizeof(QVector4D));
    shader.setUniform("reactive", Render::motionUniforms(motion));
    glUniform4fv(shader.uniformLocation("audioBands"), kBandVectors, motion.bands.data());
    glUniform4fv(shader.uniformLocation("audioWave"), kWaveVectors, motion.wave.data());
}

}

BorderLayerPainter::BorderLayerPainter() = default;

BorderLayerPainter::~BorderLayerPainter()
{
    if (m_band || m_particle || m_reactive) {
        KWin::effects->makeOpenGLContextCurrent();
    }
}

bool BorderLayerPainter::load()
{
    if (m_band && m_particle) {
        return true;
    }
    if (m_failed || !KWin::effects->isOpenGLCompositing() || !KWin::effects->makeOpenGLContextCurrent()) {
        return false;
    }
    KWin::ShaderManager *manager = KWin::ShaderManager::instance();
    m_band = manager->generateCustomShader({}, BorderShaders::vertex(), BorderShaders::bandFragment());
    m_particle = manager->generateCustomShader({}, BorderShaders::vertex(), BorderShaders::particleFragment());
    m_failed = !m_band || !m_particle;
    if (m_failed) {
        qCWarning(lcKonveyor) << "animated border shaders failed to build, using plain borders";
    }
    return !m_failed;
}

bool BorderLayerPainter::loadReactive()
{
    if (m_reactive) {
        return true;
    }
    if (m_reactiveFailed || !KWin::effects->makeOpenGLContextCurrent()) {
        return false;
    }
    m_reactive = KWin::ShaderManager::instance()->generateCustomShader({}, BorderShaders::vertex(), BorderShaders::reactiveFragment());
    m_reactiveFailed = !m_reactive;
    if (m_reactiveFailed) {
        qCWarning(lcKonveyor) << "reactive border shader failed to build, skipping those layers";
    }
    return !m_reactiveFailed;
}

void BorderLayerPainter::draw(const Target &target, const Render::BorderLayerShape &shape, const QList<Layer> &layers)
{
    const KWin::Region scissor = scissorRegion(target);
    if (scissor.isEmpty() || !load()) {
        return;
    }
    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    for (const Layer &layer : layers) {
        applyBlend(layer.config->blend);
        Render::BorderLayerShape layerShape = shape;
        layerShape.width = Render::layerWidth(*layer.config, shape.width);
        const Config::BorderLayerKind kind = layer.config->kind;
        if (kind == Config::BorderLayerKind::Particles) {
            drawParticles(target, scissor, layerShape, layer);
        } else if (Render::reactiveShaderKind(kind) >= 0) {
            drawReactive(target, scissor, layerShape, layer);
        } else {
            drawBand(target, scissor, layerShape, layer);
        }
    }
    glDisable(GL_SCISSOR_TEST);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

void BorderLayerPainter::drawBand(
    const Target &target, const KWin::Region &scissor, const Render::BorderLayerShape &shape, const Layer &layer)
{
    const double scale = target.viewport.scale();
    const KWin::ShaderBinder binder(m_band.get());
    KWin::GLShader &shader = *m_band;
    setCommonUniforms(shader, layer, target, shape);
    shader.setUniform("inset", static_cast<float>(shape.inset));
    shader.setUniform("bandWidth", static_cast<float>(shape.width));
    shader.setUniform("phase", static_cast<float>(layer.event ? layer.event->phase : layer.phase));
    shader.setUniform("kind", Render::bandShaderKind(layer.config->kind));
    shader.setUniform("params", layer.event ? layer.event->params : Render::layerParams(*layer.config));
    shader.setUniform("front", layer.event ? layer.event->front : QVector4D());
    uploadAndRender(Render::bandVertices(shape, Render::layerOuterReach(*layer.config, shape.width), scale), scissor);
}

void BorderLayerPainter::drawReactive(
    const Target &target, const KWin::Region &scissor, const Render::BorderLayerShape &shape, const Layer &layer)
{
    if (!loadReactive()) {
        return;
    }
    const double scale = target.viewport.scale();
    const KWin::ShaderBinder binder(m_reactive.get());
    KWin::GLShader &shader = *m_reactive;
    setCommonUniforms(shader, layer, target, shape);
    Render::LayerMotion silent;
    if (!layer.motion) {
        silent.resolved = *layer.config;
    }
    setReactiveUniforms(shader, layer.motion ? *layer.motion : silent);
    shader.setUniform("inset", static_cast<float>(shape.inset));
    shader.setUniform("bandWidth", static_cast<float>(shape.width));
    shader.setUniform("phase", static_cast<float>(layer.phase));
    shader.setUniform("kind", Render::reactiveShaderKind(layer.config->kind));
    shader.setUniform("params", Render::layerParams(*layer.config));
    uploadAndRender(Render::bandVertices(shape, Render::layerOuterReach(*layer.config, shape.width), scale), scissor);
}

void BorderLayerPainter::drawParticles(
    const Target &target, const KWin::Region &scissor, const Render::BorderLayerShape &shape, const Layer &layer)
{
    const std::vector<Render::BorderParticle> particles = Render::layerBursts(*layer.config)
        ? Render::burstsAt(*layer.config, shape, layer.motion ? layer.motion->bursts : Render::LayerBursts {}, layer.seconds)
        : Render::particlesAt(*layer.config, shape, layer.motion ? layer.motion->clock : layer.seconds);
    if (particles.empty()) {
        return;
    }
    const KWin::ShaderBinder binder(m_particle.get());
    setCommonUniforms(*m_particle, layer, target, shape);
    uploadAndRender(Render::particleVertices(particles, target.viewport.scale()), scissor);
}

}
