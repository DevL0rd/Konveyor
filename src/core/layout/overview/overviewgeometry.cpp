#include "layout/overview/overviewgeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Konveyor::Layout
{

namespace
{

constexpr double kMarginRatio = 0.03;
constexpr double kMinimumScaleFloor = 0.05;
constexpr double kMaximumScale = 1.0;

double viewHeightOf(const OverviewGeometryInput &input)
{
    return std::max(1.0, input.viewSize.height());
}

double cardGapOf(const OverviewGeometryInput &input)
{
    return std::clamp(input.cardGap, 0.0, 0.5) * viewHeightOf(input);
}

double usableHeightOf(const OverviewGeometryInput &input)
{
    const double height = viewHeightOf(input);
    return std::max(1.0, height - input.reserved.top() - input.reserved.bottom() - 2.0 * height * kMarginRatio);
}

void includeRect(QRectF &bounds, const QRectF &rect)
{
    bounds = bounds.isNull() ? rect : bounds.united(rect);
}

}

double overviewFitScale(const OverviewGeometryInput &input)
{
    const auto count = static_cast<double>(std::max<qsizetype>(1, input.windowFrames.size()));
    const double gap = cardGapOf(input);
    const double stack = count * viewHeightOf(input) + (count - 1.0) * gap;
    const double fit = usableHeightOf(input) / stack;
    const double floorScale = std::clamp(input.minimumScale, kMinimumScaleFloor, kMaximumScale);
    return std::clamp(fit, floorScale, kMaximumScale);
}

OverviewGeometry computeOverviewGeometry(const OverviewGeometryInput &input)
{
    OverviewGeometry geometry;
    geometry.scale = overviewFitScale(input);
    geometry.gap = cardGapOf(input);

    const double cardWidth = std::max(1.0, input.viewSize.width()) * geometry.scale;
    const double cardHeight = viewHeightOf(input) * geometry.scale;
    double top = 0.0;
    for (const QList<QRectF> &frames : input.windowFrames) {
        const QRectF card(0.0, top, cardWidth, cardHeight);
        geometry.cards.append(card);
        QList<QRectF> windows;
        windows.reserve(frames.size());
        for (const QRectF &frame : frames) {
            windows.append(QRectF(frame.x() * geometry.scale, top + frame.y() * geometry.scale, frame.width() * geometry.scale,
                frame.height() * geometry.scale));
        }
        geometry.windows.append(std::move(windows));
        top += cardHeight + geometry.gap;
    }

    QRectF content;
    for (const QRectF &card : std::as_const(geometry.cards)) {
        includeRect(content, card);
    }
    for (const QList<QRectF> &windows : std::as_const(geometry.windows)) {
        for (const QRectF &window : windows) {
            includeRect(content, window);
        }
    }
    const double padding = viewHeightOf(input) * kMarginRatio;
    geometry.contentBounds = content.adjusted(-padding, -padding, padding, padding);

    const int active = std::clamp(input.activeWorkspace, 0, std::max(0, int(geometry.cards.size()) - 1));
    QPointF desired = geometry.contentBounds.center() - QPointF(input.viewSize.width() * 0.5, input.viewSize.height() * 0.5);
    if (!geometry.cards.isEmpty()) {
        desired.setY(geometry.cards[active].center().y() - input.viewSize.height() * 0.5);
    }
    if (!input.focus.isNull()) {
        const double focusY
            = geometry.cards.isEmpty() ? input.focus.y() * geometry.scale : geometry.cards[active].top() + input.focus.y() * geometry.scale;
        desired = QPointF(input.focus.x() * geometry.scale - input.viewSize.width() * 0.5, focusY - input.viewSize.height() * 0.5);
    }
    geometry.camera = clampOverviewCamera(geometry.contentBounds, input.viewSize, desired);
    return geometry;
}

QPointF clampOverviewCamera(const QRectF &content, const QSizeF &viewSize, const QPointF &camera)
{
    const auto axis = [](double position, double start, double contentSize, double view) {
        if (contentSize <= view) {
            return start + (contentSize - view) / 2.0;
        }
        return std::clamp(position, start, start + contentSize - view);
    };
    return QPointF(axis(camera.x(), content.left(), content.width(), viewSize.width()),
        axis(camera.y(), content.top(), content.height(), viewSize.height()));
}

QRectF overviewViewport(const QPointF &camera, const QSizeF &viewSize)
{
    return QRectF(camera, viewSize);
}

QPointF overviewRevealCamera(const QRectF &content, const QSizeF &viewSize, const QPointF &camera, const QRectF &target)
{
    const QRectF visible = overviewViewport(camera, viewSize);
    const double marginX = viewSize.width() * 0.08;
    const double marginY = viewSize.height() * 0.08;
    QPointF moved = camera;
    if (target.left() < visible.left() + marginX) {
        moved.rx() -= visible.left() + marginX - target.left();
    } else if (target.right() > visible.right() - marginX) {
        moved.rx() += target.right() - (visible.right() - marginX);
    }
    if (target.top() < visible.top() + marginY) {
        moved.ry() -= visible.top() + marginY - target.top();
    } else if (target.bottom() > visible.bottom() - marginY) {
        moved.ry() += target.bottom() - (visible.bottom() - marginY);
    }
    return clampOverviewCamera(content, viewSize, moved);
}

QList<OverviewSide> overviewOffscreenSides(const QRectF &content, const QRectF &viewport)
{
    QList<OverviewSide> sides;
    if (content.left() < viewport.left() - 0.5) {
        sides.append(OverviewSide::Left);
    }
    if (content.right() > viewport.right() + 0.5) {
        sides.append(OverviewSide::Right);
    }
    if (content.top() < viewport.top() - 0.5) {
        sides.append(OverviewSide::Top);
    }
    if (content.bottom() > viewport.bottom() + 0.5) {
        sides.append(OverviewSide::Bottom);
    }
    return sides;
}

QPointF overviewContentPoint(const OverviewGeometry &geometry, const QPointF &outputPoint)
{
    return outputPoint + geometry.camera;
}

std::optional<int> overviewCardAt(const OverviewGeometry &geometry, const QPointF &contentPoint)
{
    for (qsizetype index = 0; index < geometry.cards.size(); ++index) {
        if (geometry.cards.at(index).contains(contentPoint)) {
            return static_cast<int>(index);
        }
    }
    return std::nullopt;
}

std::optional<int> overviewNearestCard(const OverviewGeometry &geometry, const QPointF &contentPoint)
{
    if (geometry.cards.isEmpty()) {
        return std::nullopt;
    }
    qsizetype best = 0;
    double bestDistance = std::numeric_limits<double>::max();
    for (qsizetype index = 0; index < geometry.cards.size(); ++index) {
        const QRectF &card = geometry.cards.at(index);
        double distance = 0.0;
        if (contentPoint.y() < card.top()) {
            distance = card.top() - contentPoint.y();
        } else if (contentPoint.y() > card.bottom()) {
            distance = contentPoint.y() - card.bottom();
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            best = index;
        }
    }
    return static_cast<int>(best);
}

std::optional<QPair<int, int>> overviewWindowAt(const OverviewGeometry &geometry, const QPointF &contentPoint)
{
    for (qsizetype workspace = geometry.windows.size(); workspace > 0; --workspace) {
        const QList<QRectF> &windows = geometry.windows.at(workspace - 1);
        for (qsizetype window = windows.size(); window > 0; --window) {
            if (windows.at(window - 1).contains(contentPoint)) {
                return QPair<int, int>(static_cast<int>(workspace - 1), static_cast<int>(window - 1));
            }
        }
    }
    return std::nullopt;
}

QPointF overviewWorkspacePoint(const OverviewGeometry &geometry, int workspace, const QPointF &contentPoint)
{
    if (workspace < 0 || workspace >= geometry.cards.size() || geometry.scale <= 0.0) {
        return contentPoint;
    }
    return (contentPoint - geometry.cards.at(workspace).topLeft()) / geometry.scale;
}

}
