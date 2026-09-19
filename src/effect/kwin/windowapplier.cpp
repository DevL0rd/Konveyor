#include "kwin/windowapplier.h"

#include "kwin/windowregistry.h"

#include "layout/common/geometry.h"

#include <KDecoration3/Decoration>
#include <core/output.h>
#include <inputmethod.h>
#include <main.h>
#include <scene/borderradius.h>
#include <window.h>
#include <workspace.h>

#include <QScopedValueRollback>

#include <algorithm>

namespace Konveyor
{

namespace
{

bool nearlyEqual(const QRectF &a, const QRectF &b)
{
    const auto close = [](double x, double y) { return std::abs(x - y) < 0.5; };
    return close(a.x(), b.x()) && close(a.y(), b.y()) && close(a.width(), b.width()) && close(a.height(), b.height());
}

bool isUserManipulated(KWin::Window *window)
{
    return window->isInteractiveMove() || window->isInteractiveResize();
}

}

WindowApplier::WindowApplier(const WindowRegistry &windows)
    : m_windows(windows)
{ }

void WindowApplier::apply(const QList<Layout::WindowState> &states)
{
    const QScopedValueRollback guard(m_applying, true);
    for (const Layout::WindowState &state : states) {
        KWin::Window *window = m_windows.windowOf(state.id);
        if (!window || isUserManipulated(window)) {
            continue;
        }
        const QRectF frame = placedFrame(state);
        const auto previous = m_appliedFrames.constFind(state.id);
        const std::optional<QSizeF> requestedSize = previous != m_appliedFrames.constEnd() ? std::optional(previous->size()) : std::nullopt;
        if (!frame.isEmpty()) {
            m_appliedFrames.insert(state.id, frame);
        }
        applySizingMode(window, state);
        applyGeometry(window, frame, requestedSize, !state.isFloating, state.isForceResizable || window->isResizable());
        applyBorderRadius(window, state);
        if (!qFuzzyCompare(window->opacity(), state.ruleOpacity)) {
            window->setOpacity(state.ruleOpacity);
        }
    }
    applyStacking(states);
}

bool WindowApplier::isApplying() const
{
    return m_applying;
}

bool WindowApplier::isEchoOfAppliedSize(Layout::WindowId id, const QSizeF &size) const
{
    const auto applied = m_appliedFrames.constFind(id);
    if (applied == m_appliedFrames.constEnd()) {
        return false;
    }
    return std::abs(applied->width() - size.width()) < 1.0 && std::abs(applied->height() - size.height()) < 1.0;
}

bool WindowApplier::isEchoOfAppliedFrame(Layout::WindowId id, const QRectF &frame) const
{
    const auto applied = m_appliedFrames.constFind(id);
    return applied != m_appliedFrames.constEnd() && nearlyEqual(*applied, frame);
}

void WindowApplier::forget(Layout::WindowId id)
{
    m_appliedFrames.remove(id);
}

QRectF WindowApplier::frameFor(const Layout::WindowState &state)
{
    if (state.targetFrame.isEmpty()) {
        return state.renderFrame;
    }
    const bool expanded = state.sizingMode != Layout::WindowMode::Normal || state.requestedSizingMode != Layout::WindowMode::Normal;
    if (state.renderFrame.isEmpty() || expanded) {
        return state.targetFrame;
    }
    return QRectF(state.renderFrame.topLeft(), state.targetFrame.size());
}

QRectF WindowApplier::placedFrame(const Layout::WindowState &state)
{
    const QRectF frame = frameFor(state);
    const KWin::LogicalOutput *home = KWin::workspace()->findOutput(state.output);
    if (frame.isEmpty() || !home) {
        return frame;
    }
    const QRectF homeRect = home->geometryF();
    if (frame.intersects(homeRect)) {
        return frame;
    }
    QList<QRectF> outputs;
    for (const KWin::LogicalOutput *output : KWin::workspace()->outputs()) {
        outputs.append(output->geometryF());
    }
    return Layout::parkedFrame(frame, homeRect, outputs);
}

void WindowApplier::applyGeometry(
    KWin::Window *window, const QRectF &frame, const std::optional<QSizeF> &requestedSize, bool tiled, bool allowResize) const
{
    if (frame.isEmpty()) {
        return;
    }
    if (window->isFullScreen() || window->isRequestedFullScreen()) {
        if (!window->isActive()) {
            window->updateLayer();
        }
        return;
    }
    const Layout::GeometryUpdate update = Layout::geometryUpdateFor(window->moveResizeGeometry(), requestedSize, frame, allowResize);
    if (update == Layout::GeometryUpdate::None) {
        return;
    }
    const KWin::InputMethod *inputMethod = KWin::kwinApp()->inputMethod();
    if (tiled && inputMethod && inputMethod->activeWindow() == window) {
        window->setVirtualKeyboardGeometry(KWin::RectF());
    }
    if (update == Layout::GeometryUpdate::Move) {
        window->move(frame.topLeft());
    } else {
        window->moveResize(frame);
    }
}

void WindowApplier::applyBorderRadius(KWin::Window *window, const Layout::WindowState &state)
{
    const Config::CornerRadius &radius = state.cornerRadius;
    const KWin::BorderRadius wanted = state.clipToGeometry
        ? KWin::BorderRadius(radius.topLeft, radius.topRight, radius.bottomRight, radius.bottomLeft)
        : window->isDecorated() ? KWin::BorderRadius::from(window->decoration()->borderRadius())
                                : KWin::BorderRadius();
    if (window->borderRadius() != wanted) {
        window->setBorderRadius(wanted);
    }
}

void WindowApplier::applySizingMode(KWin::Window *window, const Layout::WindowState &state) const
{
    const bool fullscreen = state.requestedSizingMode == Layout::WindowMode::Fullscreen && !state.isWindowedFullscreen;
    if (window->isFullScreen() != fullscreen && window->isFullScreenable()) {
        window->setFullScreen(fullscreen);
    }
    if (window->requestedMaximizeMode() != KWin::MaximizeRestore && window->isMaximizable()) {
        window->maximize(KWin::MaximizeRestore);
    }
    if (window->requestedTile()) {
        window->setTileCompatibility(nullptr);
    }
}

void WindowApplier::applyStacking(const QList<Layout::WindowState> &states)
{
    QList<const Layout::WindowState *> ordered;
    for (const Layout::WindowState &state : states) {
        ordered.append(&state);
    }
    std::ranges::stable_sort(ordered, {}, &Layout::WindowState::stackingIndex);
    qsizetype lowerGroupTop = -1;
    qsizetype currentGroupTop = -1;
    std::optional<int> currentIndex;
    for (const Layout::WindowState *state : ordered) {
        KWin::Window *window = m_windows.windowOf(state->id);
        if (!window) {
            continue;
        }
        if (currentIndex != state->stackingIndex) {
            lowerGroupTop = std::max(lowerGroupTop, currentGroupTop);
            currentIndex = state->stackingIndex;
        }
        if (KWin::workspace()->stackingOrder().indexOf(window) < lowerGroupTop) {
            KWin::workspace()->raiseWindow(window);
        }
        currentGroupTop = std::max(currentGroupTop, KWin::workspace()->stackingOrder().indexOf(window));
    }
}

}
