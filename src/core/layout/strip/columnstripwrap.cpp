#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

namespace Konveyor::Layout
{

double ColumnStrip::wrapPeriod() const
{
    if (!m_options->layout.wrapColumns || m_columns.size() < 2) {
        return 0.0;
    }
    const double period = columnOffset(m_columns.size());
    return period + m_options->layout.gaps > m_area.workingArea.width() ? period : 0.0;
}

double ColumnStrip::viewCenter(double scrollPosition) const
{
    return scrollPosition + m_area.workingArea.x() + m_area.workingArea.width() / 2.0;
}

double ColumnStrip::columnXNear(std::size_t idx, double scrollPosition) const
{
    const double colX = columnOffset(idx);
    const double mid = colX + m_columns[idx].width() / 2.0;
    return colX + (nearestPeriodCopy(mid, viewCenter(scrollPosition), wrapPeriod()) - mid);
}

double ColumnStrip::wrappedSlide(std::size_t idx, double linearSlide) const
{
    const double period = wrapPeriod();
    if (period <= 0.0) {
        return linearSlide;
    }
    const double center = viewCenter(scrollPosition());
    const double to = columnOffset(idx) + m_columns[idx].width() / 2.0;
    return nearestPeriodCopy(to + linearSlide, center, period) - nearestPeriodCopy(to, center, period);
}

std::optional<std::size_t> ColumnStrip::neighbourIndex(std::size_t idx, bool forward) const
{
    const std::size_t count = m_columns.size();
    const bool wraps = wrapPeriod() > 0.0;
    if (forward) {
        if (idx + 1 < count) {
            return idx + 1;
        }
        return wraps ? std::optional<std::size_t>(0) : std::nullopt;
    }
    if (idx > 0) {
        return idx - 1;
    }
    return wraps ? std::optional<std::size_t>(count - 1) : std::nullopt;
}

}
