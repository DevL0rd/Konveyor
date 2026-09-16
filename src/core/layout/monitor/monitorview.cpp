#include "layout/monitor/monitor.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

double Monitor::visibleWorkspacePosition() const
{
    if (m_transition) {
        return m_transition->currentIndex();
    }
    return static_cast<double>(m_activeWorkspaceIndex);
}

double Monitor::workspaceHeightWithGap() const
{
    const double gap = snapToPixelsAtLeastOne(m_area.scale, m_area.viewSize.height() * 0.1);
    return ceilToPixels(m_area.scale, m_area.viewSize.height()) + gap;
}

std::vector<double> Monitor::workspaceRenderOffsets() const
{
    const double height = workspaceHeightWithGap();
    const double firstY = snapToPixels(m_area.scale, -visibleWorkspacePosition() * height);
    std::vector<double> offsets;
    offsets.reserve(m_workspaces.size() + 1);
    for (std::size_t idx = 0; idx <= m_workspaces.size(); ++idx) {
        offsets.push_back(snapToPixels(m_area.scale, firstY + static_cast<double>(idx) * height));
    }
    return offsets;
}

double Monitor::transitionProgress() const
{
    return visibleWorkspacePosition();
}

Monitor::InsertTarget Monitor::insertTargetAt(QPointF pos) const
{
    const std::vector<double> offsets = workspaceRenderOffsets();
    const double height = ceilToPixels(m_area.scale, m_area.viewSize.height());
    if (pos.y() < offsets[0]) {
        return {DropWorkspace {false, 0, 0}, 0.0};
    }
    for (std::size_t idx = 0; idx < m_workspaces.size(); ++idx) {
        if (idx > 0 && pos.y() < offsets[idx]) {
            return {DropWorkspace {false, 0, idx}, 0.0};
        }
        if (pos.y() < offsets[idx] + height) {
            return {DropWorkspace {true, m_workspaces[idx].id(), 0}, offsets[idx]};
        }
    }
    return {DropWorkspace {false, 0, m_workspaces.size()}, 0.0};
}

}
