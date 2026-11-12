#pragma once

#include "config/types.h"

#include <QList>
#include <QRectF>
#include <QString>

#include <optional>

namespace Konveyor::Layout
{

enum class HoverSide
{
    Left,
    Right
};

struct HoverOutput
{
    QRectF geometry;
    QRectF workArea;
};

struct EdgeHoverTarget
{
    QString output;
    HoverSide side = HoverSide::Left;
    bool operator==(const EdgeHoverTarget &) const = default;
};

std::optional<HoverSide> hoverSideAt(QPointF pointer, const HoverOutput &output, const QList<QRectF> &outputs,
    Config::EdgeSwitchMonitorEdges monitorEdges, double cornerSize);

class EdgeHoverTracker
{
public:
    enum class Change
    {
        None,
        Entered,
        Left
    };

    Change move(const std::optional<EdgeHoverTarget> &at);
    std::optional<EdgeHoverTarget> take();
    void reset();

private:
    std::optional<EdgeHoverTarget> m_at;
    bool m_spent = false;
};

}
