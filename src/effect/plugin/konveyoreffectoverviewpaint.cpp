#include "plugin/konveyoreffect_p.h"

#include "layout/overview/overviewpaint.h"

#include <core/colorspace.h>
#include <core/rendertarget.h>
#include <core/renderviewport.h>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>
#include <opengl/glvertexbuffer.h>
#include <window.h>

#include <QColor>
#include <QList>
#include <QVector2D>

#include <algorithm>

namespace Konveyor
{

namespace
{

constexpr unsigned kThumbnailMask = static_cast<unsigned>(KWin::Effect::PAINT_WINDOW_OPAQUE)
    | static_cast<unsigned>(KWin::Effect::PAINT_WINDOW_TRANSLUCENT) | static_cast<unsigned>(KWin::Effect::PAINT_WINDOW_TRANSFORMED);
constexpr double kEdgeAlpha = 0.6;
const QColor kCardFillColor(24, 26, 32);

KWin::Region deviceClip(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, const KWin::Region &deviceRegion,
    const QRectF &logicalArea)
{
    const QPoint offset = viewport.renderOffset();
    const QSize bufferOffset = renderTarget.transform().map(QSize(offset.x(), offset.y()));
    const KWin::Rect bounds(QPoint(bufferOffset.width(), bufferOffset.height()), renderTarget.size() - 2 * bufferOffset);
    const KWin::Region area = viewport.mapToDeviceCoordinatesAligned(KWin::RectF(logicalArea));
    return KWin::Region(bounds)
        & viewport.transform().map((deviceRegion & area) & renderTarget.transformedRect(), renderTarget.transformedSize());
}

void appendRect(QList<QVector2D> &vertices, const QRectF &rect, double scale)
{
    const auto vertex = [scale](const QPointF &point) { return QVector2D(point.x() * scale, point.y() * scale); };
    const QVector2D topLeft = vertex(rect.topLeft());
    const QVector2D topRight = vertex(rect.topRight());
    const QVector2D bottomRight = vertex(rect.bottomRight());
    const QVector2D bottomLeft = vertex(rect.bottomLeft());
    vertices << topLeft << topRight << bottomRight << topLeft << bottomRight << bottomLeft;
}

QColor withAlpha(QColor color, double alpha)
{
    color.setAlphaF(std::clamp(alpha, 0.0, 1.0));
    return color;
}

void fillRects(const QList<QRectF> &rects, const QColor &color, const KWin::RenderTarget &renderTarget,
    const KWin::RenderViewport &viewport, const KWin::Region &scissor)
{
    if (rects.isEmpty() || color.alphaF() <= 0.0 || scissor.isEmpty()) {
        return;
    }
    QList<QVector2D> vertices;
    vertices.reserve(rects.size() * 6);
    for (const QRectF &rect : rects) {
        appendRect(vertices, rect, viewport.scale());
    }
    KWin::ShaderBinder binder(KWin::ShaderTrait::UniformColor | KWin::ShaderTrait::TransformColorspace);
    KWin::GLShader *shader = binder.shader();
    shader->setUniform(KWin::GLShader::Mat4Uniform::ModelViewProjectionMatrix, viewport.projectionMatrix());
    shader->setColorspaceUniforms(KWin::ColorDescription::sRGB, renderTarget.colorDescription(), KWin::RenderingIntent::Perceptual);
    shader->setUniform(KWin::GLShader::ColorUniform::Color, color);
    KWin::GLVertexBuffer *buffer = KWin::GLVertexBuffer::streamingBuffer();
    buffer->reset();
    buffer->setVertices(vertices);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    buffer->render(scissor, GL_TRIANGLES, true);
    glDisable(GL_BLEND);
}

void appendOutline(QList<QRectF> &rects, const QRectF &rect, double thickness)
{
    if (rect.isEmpty() || thickness <= 0.0) {
        return;
    }
    const double inner = thickness;
    rects.append(QRectF(rect.left(), rect.top(), rect.width(), inner));
    rects.append(QRectF(rect.left(), rect.bottom() - inner, rect.width(), inner));
    rects.append(QRectF(rect.left(), rect.top() + inner, inner, std::max(0.0, rect.height() - 2.0 * inner)));
    rects.append(QRectF(rect.right() - inner, rect.top() + inner, inner, std::max(0.0, rect.height() - 2.0 * inner)));
}

void drawThumbnail(const WindowRegistry &windows, bool &drawingThumbnail, const Layout::OverviewPaintWindow &painted,
    const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, const KWin::Region &scissor, const QRectF &workArea)
{
    if (painted.opacity <= 0.0 || !painted.rect.intersects(workArea)) {
        return;
    }
    KWin::Window *window = windows.windowOf(painted.id);
    KWin::EffectWindow *effectWindow = window ? KWin::effects->findWindow(window->internalId()) : nullptr;
    if (!effectWindow) {
        return;
    }
    KWin::WindowPaintData data;
    data.multiplyOpacity(painted.opacity);
    KWin::Rect logicalRegion;
    KWin::Effect::setPositionTransformations(
        data, logicalRegion, effectWindow, KWin::Rect(painted.rect.toAlignedRect()), Qt::KeepAspectRatio);
    logicalRegion &= KWin::Rect(workArea.toAlignedRect());
    if (logicalRegion.isEmpty()) {
        return;
    }
    const KWin::Region deviceRegion = viewport.mapToDeviceCoordinatesAligned(KWin::Region(logicalRegion)) & scissor;
    if (deviceRegion.isEmpty()) {
        return;
    }
    drawingThumbnail = true;
#ifdef KONVEYOR_KWIN_PAINTSCREEN_BOOL
    const bool paintedOk
        = KWin::effects->drawWindow(renderTarget, viewport, effectWindow, static_cast<int>(kThumbnailMask), deviceRegion, data);
    Q_UNUSED(paintedOk);
#else
    KWin::effects->drawWindow(renderTarget, viewport, effectWindow, static_cast<int>(kThumbnailMask), deviceRegion, data);
#endif
    drawingThumbnail = false;
}

void appendChevronWing(QList<QVector2D> &vertices, const QPointF &from, const QPointF &to, double thickness, double scale)
{
    const QPointF delta = to - from;
    const double length = std::hypot(delta.x(), delta.y());
    if (length <= 0.0) {
        return;
    }
    const QPointF normal(-delta.y() / length, delta.x() / length);
    const QPointF offset = normal * (thickness * 0.5);
    const auto vertex = [scale](const QPointF &p) { return QVector2D(p.x() * scale, p.y() * scale); };
    const QVector2D v0 = vertex(from + offset);
    const QVector2D v1 = vertex(to + offset);
    const QVector2D v2 = vertex(to - offset);
    const QVector2D v3 = vertex(from - offset);
    vertices << v0 << v1 << v2 << v0 << v2 << v3;
}

void appendChevron(QList<QVector2D> &vertices, const Layout::OverviewPaintEdge &edge, double scale)
{
    const QPointF center = edge.rect.center();
    const double span = edge.rect.width() * 0.45;
    const double thickness = std::clamp(span * 0.22, 2.5, 4.0);
    QPointF apex;
    QPointF wing1;
    QPointF wing2;
    switch (edge.side) {
    case Layout::OverviewSide::Left:
        apex = center - QPointF(span * 0.4, 0.0);
        wing1 = center + QPointF(span * 0.4, -span);
        wing2 = center + QPointF(span * 0.4, span);
        break;
    case Layout::OverviewSide::Right:
        apex = center + QPointF(span * 0.4, 0.0);
        wing1 = center - QPointF(span * 0.4, span);
        wing2 = center - QPointF(span * 0.4, -span);
        break;
    case Layout::OverviewSide::Top:
        apex = center - QPointF(0.0, span * 0.4);
        wing1 = center + QPointF(-span, span * 0.4);
        wing2 = center + QPointF(span, span * 0.4);
        break;
    case Layout::OverviewSide::Bottom:
        apex = center + QPointF(0.0, span * 0.4);
        wing1 = center + QPointF(-span, -span * 0.4);
        wing2 = center + QPointF(span, -span * 0.4);
        break;
    }
    appendChevronWing(vertices, apex, wing1, thickness, scale);
    appendChevronWing(vertices, apex, wing2, thickness, scale);
}

void fillChevrons(const QList<Layout::OverviewPaintEdge> &edges, const QColor &color, const KWin::RenderTarget &renderTarget,
    const KWin::RenderViewport &viewport, const KWin::Region &scissor)
{
    if (edges.isEmpty() || color.alphaF() <= 0.0 || scissor.isEmpty()) {
        return;
    }
    QList<QVector2D> vertices;
    vertices.reserve(edges.size() * 12);
    for (const Layout::OverviewPaintEdge &edge : edges) {
        appendChevron(vertices, edge, viewport.scale());
    }
    KWin::ShaderBinder binder(KWin::ShaderTrait::UniformColor | KWin::ShaderTrait::TransformColorspace);
    KWin::GLShader *shader = binder.shader();
    shader->setUniform(KWin::GLShader::Mat4Uniform::ModelViewProjectionMatrix, viewport.projectionMatrix());
    shader->setColorspaceUniforms(KWin::ColorDescription::sRGB, renderTarget.colorDescription(), KWin::RenderingIntent::Perceptual);
    shader->setUniform(KWin::GLShader::ColorUniform::Color, color);
    KWin::GLVertexBuffer *buffer = KWin::GLVertexBuffer::streamingBuffer();
    buffer->reset();
    buffer->setVertices(vertices);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    buffer->render(scissor, GL_TRIANGLES, true);
    glDisable(GL_BLEND);
}

QList<QRectF> selectedOutlineRects(const Layout::OverviewPaintView &paint, double thickness)
{
    QList<QRectF> rects;
    for (const Layout::OverviewPaintWindow &window : paint.windows) {
        if (window.selected) {
            appendOutline(rects, Layout::overviewOutlineRect(window.rect, thickness), thickness);
        }
    }
    return rects;
}

QColor resolveFocusRingColor(const Config::Border &focusRing, const AccentColor &accent)
{
    const Config::Paint &paint = focusRing.active;
    if (paint.source != Config::ColorSource::Explicit) {
        return accent.colorFor(paint.source);
    }
    if (paint.color.isValid()) {
        return paint.color;
    }
    if (paint.gradient) {
        return paint.gradient->from;
    }
    return accent.colorFor(Config::ColorSource::SystemAccent);
}

}

bool KonveyorEffect::overviewPaintActive() const
{
    const Layout::Engine &engine = readEngine();
    return engine.isOverviewEnabled() && engine.overviewProgress() > 0.0;
}

void KonveyorEffect::paintOverview(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport,
    const KWin::Region &deviceRegion, const Layout::OverviewView &view, const QRectF &workArea)
{
    const Layout::OverviewPaintView paint
        = Layout::overviewPaintView(view, {}, readEngine().overviewProgress(), d->config.config().overview.offscreenIndicators);
    const KWin::Region scissor = deviceClip(renderTarget, viewport, deviceRegion, workArea);
    if (paint.blend <= 0.0 || scissor.isEmpty()) {
        return;
    }

    const double scale = view.outputGeometry.height();
    const Config::Border &focusRing = d->config.config().layout.focusRing;
    const double ringWidth = focusRing.enabled ? focusRing.width : 3.0;
    const double windowThickness = std::max(2.0, std::max(ringWidth, scale * 0.0035));
    const QColor focusRingColor = resolveFocusRingColor(focusRing, d->accent);

    fillRects({workArea}, withAlpha(Qt::black, paint.scrimOpacity * Layout::kOverviewScrimAlpha), renderTarget, viewport, scissor);
    for (const Layout::OverviewPaintCard &card : paint.cards) {
        if (!card.rect.intersects(workArea)) {
            continue;
        }
        if (card.hovered || card.dragged) {
            fillRects({card.rect}, withAlpha(kCardFillColor, card.opacity), renderTarget, viewport, scissor);
        }
    }
    if (paint.dropHintRect && paint.dropHintRect->intersects(workArea)) {
        fillRects({*paint.dropHintRect}, withAlpha(focusRingColor, 0.22 * paint.blend), renderTarget, viewport, scissor);
        QList<QRectF> hintOutlines;
        appendOutline(hintOutlines, Layout::overviewOutlineRect(*paint.dropHintRect, windowThickness), windowThickness);
        fillRects(hintOutlines, withAlpha(focusRingColor, 0.85 * paint.blend), renderTarget, viewport, scissor);
    }
    for (const Layout::OverviewPaintWindow &window : paint.windows) {
        drawThumbnail(d->windows, d->drawingOverviewThumbnail, window, renderTarget, viewport, scissor, workArea);
    }
    fillRects(selectedOutlineRects(paint, windowThickness), withAlpha(focusRingColor, Layout::kOverviewOutlineAlpha * paint.blend),
        renderTarget, viewport, scissor);
    fillChevrons(paint.edges, withAlpha(Qt::white, kEdgeAlpha * paint.blend), renderTarget, viewport, scissor);
}

}
