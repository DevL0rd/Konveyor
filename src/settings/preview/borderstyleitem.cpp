#include "preview/borderstyleitem.h"

#include "preview/borderstyleclock.h"
#include "preview/borderstylematerial.h"
#include "render/borderlayer.h"
#include "render/drive.h"

#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGRendererInterface>
#include <QSGTransformNode>

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace Konveyor::Settings
{

namespace
{

constexpr float kDimmedOpacity = 0.25f;

const QSGGeometry::AttributeSet &vertexAttributes()
{
    static const std::array<QSGGeometry::Attribute, 2> attributes {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
        QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType, false),
    };
    static const QSGGeometry::AttributeSet set {2, sizeof(Render::BorderVertex), attributes.data()};
    return set;
}

QSGGeometryNode *newLayerNode()
{
    auto *node = new QSGGeometryNode;
    node->setGeometry(new QSGGeometry(vertexAttributes(), 0));
    node->geometry()->setDrawingMode(QSGGeometry::DrawTriangles);
    node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    return node;
}

void setVertices(QSGGeometryNode *node, const std::vector<Render::BorderVertex> &vertices)
{
    QSGGeometry *geometry = node->geometry();
    geometry->allocate(static_cast<int>(vertices.size()));
    if (!vertices.empty()) {
        std::memcpy(geometry->vertexData(), vertices.data(), vertices.size() * sizeof(Render::BorderVertex));
    }
    node->markDirty(QSGNode::DirtyGeometry);
}

BorderStyleShaderKind shaderFor(Config::BorderLayerKind kind)
{
    if (kind == Config::BorderLayerKind::Particles) {
        return BorderStyleShaderKind::Particle;
    }
    return Render::reactiveShaderKind(kind) >= 0 ? BorderStyleShaderKind::Reactive : BorderStyleShaderKind::Band;
}

BorderStyleMaterial *materialFor(QSGGeometryNode *node, BorderStyleShaderKind shader)
{
    auto *material = static_cast<BorderStyleMaterial *>(node->material());
    if (!material || material->shader() != shader) {
        material = new BorderStyleMaterial(shader);
        node->setMaterial(material);
    }
    return material;
}

void fillColors(BorderStyleUniforms &uniforms, const QList<QColor> &colors)
{
    const qsizetype count = std::min<qsizetype>(colors.size(), Config::kBorderLayerMaxColors);
    for (qsizetype i = 0; i < count; ++i) {
        const QColor &color = colors[i];
        uniforms.colors[static_cast<std::size_t>(i)] = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
    }
    uniforms.colorCount = static_cast<std::int32_t>(count);
}

void fillShape(BorderStyleUniforms &uniforms, const Render::BorderLayerShape &shape, double pixels)
{
    const Config::CornerRadius &radius = shape.radius;
    uniforms.halfSize = {static_cast<float>(shape.size.width() / 2.0), static_cast<float>(shape.size.height() / 2.0)};
    uniforms.radii = {static_cast<float>(radius.topLeft), static_cast<float>(radius.topRight), static_cast<float>(radius.bottomRight),
        static_cast<float>(radius.bottomLeft)};
    uniforms.pixels = static_cast<float>(pixels);
    uniforms.inset = static_cast<float>(shape.inset);
    uniforms.bandWidth = static_cast<float>(shape.width);
}

std::vector<Render::BorderVertex> layerVertices(
    const Config::BorderLayer &config, const Render::BorderLayerShape &shape, const LayerFrame &frame)
{
    if (frame.hidden) {
        return {};
    }
    if (config.kind != Config::BorderLayerKind::Particles) {
        return Render::bandVertices(shape, Render::layerOuterReach(config, shape.width), frame.pixels);
    }
    if (Render::layerBursts(config)) {
        return Render::particleVertices(
            Render::burstsAt(config, shape, frame.motion ? frame.motion->bursts : Render::LayerBursts {}, frame.seconds), frame.pixels);
    }
    return Render::particleVertices(Render::particlesAt(config, shape, frame.motion ? frame.motion->clock : frame.seconds), frame.pixels);
}

void fillReactive(BorderStyleUniforms &uniforms, const Render::LayerMotion &motion)
{
    const QVector4D reactive = Render::motionUniforms(motion);
    uniforms.reactive = {reactive.x(), reactive.y(), reactive.z(), reactive.w()};
    uniforms.audioBands = motion.bands;
    uniforms.audioWave = motion.wave;
}

void fillLayerNode(QSGGeometryNode *node, const PreviewLayer &layer, const Render::BorderLayerShape &shape, const LayerFrame &frame)
{
    const Config::BorderLayer &config = frame.motion ? frame.motion->resolved : layer.config;
    BorderStyleMaterial *material = materialFor(node, shaderFor(config.kind));
    BorderStyleUniforms &uniforms = material->uniforms;
    material->blend = config.blend;
    fillColors(uniforms, layer.colors);
    uniforms.colorMix = Render::colorMixUniform(frame.motion);
    uniforms.additive = Render::additiveUniform(config.blend);
    fillShape(uniforms, shape, frame.pixels);
    uniforms.layerOpacity = frame.opacity;
    uniforms.phase = static_cast<float>(frame.phase);
    const int reactiveKind = Render::reactiveShaderKind(config.kind);
    uniforms.kind = reactiveKind >= 0 ? reactiveKind : Render::bandShaderKind(config.kind);
    if (frame.motion) {
        fillReactive(uniforms, *frame.motion);
    }
    const QVector4D params = Render::isOneShotLayer(config.kind) ? frame.params : Render::layerParams(config);
    uniforms.params = {params.x(), params.y(), params.z(), params.w()};
    uniforms.front = {frame.front.x(), frame.front.y(), frame.front.z(), frame.front.w()};
    node->markDirty(QSGNode::DirtyMaterial);
    setVertices(node, layerVertices(config, shape, frame));
}

double layerPhaseAt(const Config::BorderLayer &layer, double seconds, bool reduceMotion)
{
    return reduceMotion ? 0.0 : Render::layerPhase(layer, seconds);
}

void trimChildren(QSGNode *root, int count)
{
    while (root->childCount() > count) {
        QSGNode *last = root->lastChild();
        root->removeChildNode(last);
        delete last;
    }
}

}

BorderStyleItem::BorderStyleItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_clock(BorderStyleClock::shared())
{
    setFlag(ItemHasContents);
    setFlag(ItemObservesViewport);
    connect(this, &BorderStyleItem::layersChanged, this, &BorderStyleItem::parseLayers);
    connect(this, &BorderStyleItem::appearanceChanged, this, [this] {
        updateAnimating();
        update();
    });
}

BorderStyleItem::~BorderStyleItem()
{
    m_clock->setAnimating(this, false);
}

void BorderStyleItem::parseLayers()
{
    m_parsed = previewLayers(m_layerValues, m_theme);
    updateAnimating();
    update();
}

void BorderStyleItem::updateAnimating()
{
    m_clock->setAnimating(this, window() && isVisible() && !m_reduceMotion && anyLayerMoves(m_parsed));
}

Render::EventScene BorderStyleItem::eventScene() const
{
    Render::EventScene scene {m_eventSubject, std::nullopt, std::nullopt, std::nullopt, m_eventFarthest};
    if (!m_eventFrom.isEmpty() && !m_eventTo.isEmpty()) {
        scene.from = m_eventFrom;
        scene.to = m_eventTo;
    }
    return scene;
}

double BorderStyleItem::eventDuration() const
{
    Config::BorderStyle style;
    for (const PreviewLayer &layer : m_parsed) {
        style.layers.append(layer.config);
    }
    return Render::styleSeconds(style, eventScene(), m_eventLength > 0.0 ? std::optional(m_eventLength) : std::nullopt);
}

double BorderStyleItem::outerReach() const
{
    double reach = m_inset;
    for (const PreviewLayer &layer : m_parsed) {
        reach = std::max(reach, m_inset + Render::layerOuterReach(layer.config, m_baseWidth));
    }
    return reach * m_zoom;
}

bool BorderStyleItem::reachesViewport()
{
    const QQuickItem *viewport = viewportItem();
    if (!viewport) {
        return true;
    }
    const double reach = outerReach();
    const QRectF drawn = QRectF(0, 0, width(), height()).adjusted(-reach, -reach, reach, reach);
    return drawn.intersects(viewport->mapRectToItem(this, viewport->clipRect()));
}

void BorderStyleItem::advance()
{
    const QWindow *widgetWindow = QQuickRenderControl::renderWindowFor(window());
    const bool shown = widgetWindow ? widgetWindow->isVisible() : window()->isVisible();
    if (shown && reachesViewport()) {
        update();
    }
}

void BorderStyleItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    update();
}

void BorderStyleItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange || change == ItemVisibleHasChanged) {
        updateAnimating();
    }
    if (change == ItemDevicePixelRatioHasChanged) {
        update();
    }
}

LayerFrame BorderStyleItem::frameFor(int index, double ratio, double now, const QSizeF &frameSize)
{
    const PreviewLayer &layer = m_parsed[index];
    const bool reactive = Render::isReactive(layer.config);
    Render::LayerMotion &motion = m_motions[static_cast<std::size_t>(index)];
    if (reactive && m_reduceMotion) {
        Render::poseMotion(motion, layer.config, now, m_baseWidth);
    } else if (reactive) {
        Render::advanceMotion(motion, layer.config, Render::simulatedSignals(now), now, m_baseWidth);
    }
    const Config::BorderLayer &config = reactive ? motion.resolved : layer.config;
    const float emphasis = m_emphasis >= 0 && m_emphasis != index ? kDimmedOpacity : 1.f;
    LayerFrame frame {m_zoom * ratio, reactive && !m_reduceMotion ? motion.phase : layerPhaseAt(config, now, m_reduceMotion), now,
        static_cast<float>(config.opacity) * emphasis, false, {}, {}, reactive ? &motion : nullptr};
    if (Render::isOneShotLayer(layer.config.kind)) {
        applyOneShot(frame, config, motion, now, frameSize);
    } else if (m_eventTime >= 0.0) {
        frame.opacity *= static_cast<float>(Render::standingEnvelope(m_eventTime, eventDuration()));
    }
    return frame;
}

void BorderStyleItem::applyOneShot(
    LayerFrame &frame, const Config::BorderLayer &config, const Render::LayerMotion &motion, double now, const QSizeF &frameSize) const
{
    const QRectF own = m_eventTime >= 0.0 ? m_eventFrame : QRectF(QPointF(), frameSize);
    Render::EventScene ownScene;
    ownScene.subject = own;
    const auto shown = m_eventTime >= 0.0 ? Render::oneShotFrame(config, eventScene(), own, m_eventTime)
        : !config.fireOn.isEmpty()        ? Render::oneShotFrame(config, ownScene, own, now - motion.lastFire)
        : m_reduceMotion                  ? std::nullopt
                                          : Render::restingFrame(config, ownScene, own, now, m_rippleRest);
    frame.hidden = !shown;
    if (shown) {
        frame.phase = shown->phase;
        frame.params = shown->params;
        frame.front = shown->front;
    }
}

QSGNode *BorderStyleItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!QSGRendererInterface::isApiRhiBased(window()->rendererInterface()->graphicsApi())) {
        delete oldNode;
        return nullptr;
    }
    auto *root = static_cast<QSGTransformNode *>(oldNode);
    if (!root) {
        root = new QSGTransformNode;
    }
    const double ratio = window()->effectiveDevicePixelRatio();
    QMatrix4x4 matrix;
    matrix.scale(static_cast<float>(1.0 / ratio));
    root->setMatrix(matrix);

    const double now = m_reduceMotion ? Render::kFrozenParticleSeconds : m_clock->seconds();
    const Config::CornerRadius radius {m_radii.x(), m_radii.y(), m_radii.z(), m_radii.w()};
    const QSizeF frameSize(width() / m_zoom, height() / m_zoom);
    trimChildren(root, static_cast<int>(m_parsed.size()));
    m_motions.resize(m_parsed.size());
    QSGNode *child = root->firstChild();
    for (int index = 0; index < static_cast<int>(m_parsed.size()); ++index) {
        const PreviewLayer &layer = m_parsed[index];
        auto *node = child ? static_cast<QSGGeometryNode *>(child) : newLayerNode();
        const LayerFrame frame = frameFor(index, ratio, now, frameSize);
        const Config::BorderLayer &config
            = Render::isReactive(layer.config) ? m_motions[static_cast<std::size_t>(index)].resolved : layer.config;
        fillLayerNode(node, layer, {frameSize, radius, m_inset, Render::layerWidth(config, m_baseWidth)}, frame);
        if (child) {
            child = child->nextSibling();
        } else {
            root->appendChildNode(node);
        }
    }
    return root;
}

}
