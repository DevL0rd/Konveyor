#pragma once

#include <QList>
#include <QMarginsF>
#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <optional>
#include <utility>

namespace Konveyor::Layout
{

enum class OverviewSide
{
    Left,
    Right,
    Top,
    Bottom
};

// Output-local logical frames for one workspace, plus the output and card metrics.
struct OverviewGeometryInput
{
    QSizeF viewSize;
    QMarginsF reserved;
    // Gap between cards as a fraction of the output height.
    double cardGap = 0.08;
    // One entry per workspace; frames are workspace-local logical rects.
    QList<QList<QRectF>> windowFrames;
    int activeWorkspace = 0;
    // Workspace-local point to keep in view horizontally, usually the selected window.
    QPointF focus;
    double minimumScale = 0.3;
};

struct OverviewGeometry
{
    double scale = 1.0;
    double gap = 0.0;
    QPointF camera;
    QRectF contentBounds;
    QList<QRectF> cards;
    QList<QList<QRectF>> windows;
};

double overviewFitScale(const OverviewGeometryInput &input);
OverviewGeometry computeOverviewGeometry(const OverviewGeometryInput &input);
QPointF clampOverviewCamera(const QRectF &content, const QSizeF &viewSize, const QPointF &camera);
QRectF overviewViewport(const QPointF &camera, const QSizeF &viewSize);
QList<OverviewSide> overviewOffscreenSides(const QRectF &content, const QRectF &viewport);
// Smallest camera move that brings target fully into view, then clamped to content.
QPointF overviewRevealCamera(const QRectF &content, const QSizeF &viewSize, const QPointF &camera, const QRectF &target);

QPointF overviewContentPoint(const OverviewGeometry &geometry, const QPointF &outputPoint);
std::optional<int> overviewCardAt(const OverviewGeometry &geometry, const QPointF &contentPoint);
std::optional<int> overviewNearestCard(const OverviewGeometry &geometry, const QPointF &contentPoint);
std::optional<QPair<int, int>> overviewWindowAt(const OverviewGeometry &geometry, const QPointF &contentPoint);
// Maps a content-space point onto a workspace-local frame coordinate.
QPointF overviewWorkspacePoint(const OverviewGeometry &geometry, int workspace, const QPointF &contentPoint);

}
