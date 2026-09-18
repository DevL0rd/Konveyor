#include "layout/tile/window.h"

#include "layout/common/geometry.h"

#include <cmath>

namespace Konveyor::Layout
{

LayoutWindow::LayoutWindow(WindowId id, const WindowProperties &properties)
    : m_id(id)
    , m_properties(properties)
    , m_size(clampedNonNegative(QSizeF(std::round(properties.frameSize.width()), std::round(properties.frameSize.height()))))
    , m_isUrgent(properties.isUrgent)
{ }

bool LayoutWindow::setRules(const EffectiveWindowRules &rules)
{
    m_needsRuleRecompute = false;
    if (rules == m_rules) {
        return false;
    }
    if (!rules.opacity || *rules.opacity >= 1.0) {
        m_ignoreOpacityRule = false;
    }
    m_rules = rules;
    return true;
}

QSize LayoutWindow::minSize() const
{
    return m_rules.limitMinSize(nonNegativeSize(m_properties.minSize));
}

QSize LayoutWindow::maxSize() const
{
    return m_rules.limitMaxSize(nonNegativeSize(m_properties.maxSize));
}

QSize LayoutWindow::tiledMinSize() const
{
    return m_rules.limitMinSize(QSize(0, 0));
}

QSize LayoutWindow::tiledMaxSize() const
{
    return m_rules.limitMaxSize(QSize(0, 0));
}

std::optional<QSize> LayoutWindow::pendingSize() const
{
    std::optional<QSize> current;
    if (m_sizingMode == WindowMode::Normal) {
        current = roundedSize(m_size);
    }
    if (m_oneShotResize == OneShotResize::FollowWindow || !m_uncommittedRequest || !m_requestedSize) {
        return current;
    }
    if (m_requestedMode != WindowMode::Normal) {
        return std::nullopt;
    }
    QSize size = *m_requestedSize;
    if ((size.width() == 0 || size.height() == 0) && !current) {
        return std::nullopt;
    }
    if (size.width() == 0) {
        size.setWidth(current->width());
    }
    if (size.height() == 0) {
        size.setHeight(current->height());
    }
    return size;
}

void LayoutWindow::requestWindowedFullscreen(bool value)
{
    m_windowedFullscreenRequested = value;
    m_uncommittedRequest = true;
}

void LayoutWindow::requestSize(QSize size, WindowMode mode, bool animate)
{
    if (mode == WindowMode::Fullscreen) {
        m_windowedFullscreenRequested = false;
    }
    const bool changed = m_requestedSize != size;
    if (changed || m_requestedMode != mode) {
        m_uncommittedRequest = true;
    }
    if (changed && animate) {
        m_animateNextResize = true;
    }
    m_requestedSize = size;
    m_requestedMode = mode;
    m_oneShotResize = OneShotResize::None;
}

void LayoutWindow::requestSizeUntilCommit(QSize size, bool animate)
{
    const bool changed = m_requestedSize != size;
    if (changed || m_requestedMode != WindowMode::Normal) {
        m_uncommittedRequest = true;
    }
    if (changed && animate) {
        m_animateNextResize = true;
    }
    m_requestedSize = size;
    m_requestedMode = WindowMode::Normal;
    m_oneShotResize = m_uncommittedRequest ? OneShotResize::AwaitingCommit : OneShotResize::FollowWindow;
}

std::optional<QSizeF> LayoutWindow::takeResizeStartSize()
{
    return std::exchange(m_resizeStartSize, std::nullopt);
}

bool LayoutWindow::commit(QSizeF frameSize)
{
    const QSizeF size = clampedNonNegative(QSizeF(std::round(frameSize.width()), std::round(frameSize.height())));
    const bool sizeChanged = size != m_size;
    if (sizeChanged && m_animateNextResize) {
        m_resizeStartSize = m_size;
    }
    const bool modeChanged = m_sizingMode != m_requestedMode || m_windowedFullscreen != m_windowedFullscreenRequested;
    m_size = size;
    m_sizingMode = m_requestedMode;
    m_windowedFullscreen = m_windowedFullscreenRequested;
    m_animateNextResize = false;
    m_uncommittedRequest = false;
    if (m_oneShotResize == OneShotResize::AwaitingCommit) {
        m_oneShotResize = OneShotResize::FollowWindow;
    }
    if (m_resizeEndPending) {
        m_resizeEdges.reset();
        m_resizeEndPending = false;
    }
    return sizeChanged || modeChanged;
}

bool LayoutWindow::isChildOf(const LayoutWindow &parent) const
{
    return m_properties.parent == parent.id();
}

void LayoutWindow::setFocused(bool focused)
{
    if (m_isFocused == focused) {
        return;
    }
    m_isFocused = focused;
    m_isUrgent = false;
    m_needsRuleRecompute = true;
}

void LayoutWindow::setUrgent(bool urgent)
{
    if (m_isFocused && urgent) {
        return;
    }
    m_needsRuleRecompute |= m_isUrgent != urgent;
    m_isUrgent = urgent;
}

void LayoutWindow::setActivated(bool active)
{
    m_needsRuleRecompute |= m_isActivated != active;
    m_isActivated = active;
}

void LayoutWindow::setActiveInColumn(bool active)
{
    m_needsRuleRecompute |= m_isActiveInColumn != active;
    m_isActiveInColumn = active;
}

void LayoutWindow::setFloating(bool floating)
{
    m_needsRuleRecompute |= m_isFloating != floating;
    m_isFloating = floating;
}

void LayoutWindow::setInteractiveResize(std::optional<quint8> edges)
{
    if (edges) {
        m_resizeEdges = edges;
        m_resizeEndPending = false;
        return;
    }
    if (m_resizeEdges) {
        m_resizeEndPending = true;
    }
}

void LayoutWindow::cancelInteractiveResize()
{
    m_resizeEdges.reset();
    m_resizeEndPending = false;
}

}
