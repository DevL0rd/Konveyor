#include "kwin/windowapplier.h"

#include "kwin/windowregistry.h"

#include "config/log.h"
#include "layout/common/geometry.h"

#include <KDecoration3/Decoration>
#include <core/output.h>
#include <inputmethod.h>
#include <main.h>
#include <scene/borderradius.h>
#include <window.h>
#include <workspace.h>

#include <wayland_server.h>
#include <xdgshellwindow.h>

#include <QScopedValueRollback>

#include <algorithm>
#include <utility>

namespace Konveyor
{

namespace
{

bool nearlyEqual(const QRectF &a, const QRectF &b)
{
    const auto close = [](double x, double y) { return std::abs(x - y) < 0.5; };
    return close(a.x(), b.x()) && close(a.y(), b.y()) && close(a.width(), b.width()) && close(a.height(), b.height());
}

bool isSameSize(const QSizeF &a, const QSizeF &b)
{
    return std::abs(a.width() - b.width()) < 1.0 && std::abs(a.height() - b.height()) < 1.0;
}

bool isUserManipulated(KWin::Window *window)
{
    return window->isInteractiveMove() || window->isInteractiveResize();
}

#ifdef KONVEYOR_KWIN_CONFINES_COMMITS
using UnrestrictedFlag = bool KWin::XdgToplevelWindow::*;

template<UnrestrictedFlag Flag> struct UnrestrictedFlagAccess
{
    friend UnrestrictedFlag unrestrictedFlag() { return Flag; }
};

UnrestrictedFlag unrestrictedFlag();
template struct UnrestrictedFlagAccess<&KWin::XdgToplevelWindow::m_isUnrestricted>;

void setExemptFromConfinement(KWin::Window *window, bool exempt)
{
    if (KWin::XdgToplevelWindow *toplevel = KWin::waylandServer()->findXdgToplevelWindow(window->surface())) {
        toplevel->*unrestrictedFlag() = exempt;
    }
}
#else
void setExemptFromConfinement(KWin::Window *, bool) { }
#endif

using NextStates = KWin::XdgToplevelInterface::States KWin::XdgToplevelWindow::*;
using ConfigureScheduler = void (KWin::Window::*)();

template<NextStates Flag> struct NextStatesAccess
{
    friend NextStates nextStates() { return Flag; }
};

template<ConfigureScheduler Function> struct ConfigureSchedulerAccess
{
    friend ConfigureScheduler configureScheduler() { return Function; }
};

NextStates nextStates();
ConfigureScheduler configureScheduler();
template struct NextStatesAccess<&KWin::XdgToplevelWindow::m_nextStates>;
template struct ConfigureSchedulerAccess<&KWin::Window::doSetActive>;

constexpr KWin::XdgToplevelInterface::States TiledEdges {KWin::XdgToplevelInterface::State::TiledLeft
    | KWin::XdgToplevelInterface::State::TiledRight | KWin::XdgToplevelInterface::State::TiledTop
    | KWin::XdgToplevelInterface::State::TiledBottom};

void setStrictSize(KWin::Window *window, bool strict)
{
    KWin::XdgToplevelWindow *toplevel = KWin::waylandServer()->findXdgToplevelWindow(window->surface());
    if (!toplevel) {
        return;
    }
    KWin::XdgToplevelInterface::States &states = toplevel->*nextStates();
    const KWin::XdgToplevelInterface::States wanted = strict ? (states | TiledEdges) : (states & ~TiledEdges);
    if (wanted != states) {
        states = wanted;
        (toplevel->*configureScheduler())();
    }
}

}

WindowApplier::WindowApplier(const WindowRegistry &windows)
    : m_windows(windows)
{ }

WindowApplier::~WindowApplier()
{
    for (const QPointer<KWin::Window> &window : std::as_const(m_parked)) {
        if (window) {
            setExemptFromConfinement(window, false);
        }
    }
}

void WindowApplier::apply(const QList<Layout::WindowState> &states)
{
    const QScopedValueRollback guard(m_applying, true);
    for (const Layout::WindowState &state : states) {
        KWin::Window *window = m_windows.windowOf(state.id);
        if (!window) {
            continue;
        }
        applyOpacity(window, state);
        logTargetChange(window, state);
        if (isUserManipulated(window)) {
            continue;
        }
        const QRectF frame = placedFrame(state);
        setParked(state.id, window, frame != frameFor(state));
        const auto previous = m_appliedFrames.constFind(state.id);
        const std::optional<QSizeF> requestedSize = previous != m_appliedFrames.constEnd() ? std::optional(previous->size()) : std::nullopt;
        if (!frame.isEmpty()) {
            m_appliedFrames.insert(state.id, frame);
            QList<QSizeF> &inFlight = m_inFlightSizes[state.id];
            if (!requestedSize || *requestedSize != frame.size()) {
                inFlight.removeAll(frame.size());
                inFlight.append(frame.size());
            }
        }
        applySizingMode(window, state);
        setStrictSize(window, keepsStrictSize(window, state, frame));
        applyGeometry(window, frame, requestedSize, !state.isFloating,
            state.isForceResizable || window->isResizable() || m_strictUntilSettled.contains(state.id),
            state.requestedSizingMode == Layout::WindowMode::Fullscreen);
        applyBorderRadius(window, state);
    }
    applyStacking(states);
}

bool WindowApplier::keepsStrictSize(KWin::Window *window, const Layout::WindowState &state, const QRectF &frame)
{
    const bool tiled = !state.isFloating && state.requestedSizingMode != Layout::WindowMode::Fullscreen && !window->isRequestedFullScreen();
    if (tiled && state.isForceResizable && !window->isResizable()) {
        m_strictUntilSettled.insert(state.id);
        return true;
    }
    if (!m_strictUntilSettled.contains(state.id)) {
        return false;
    }
    if (!tiled || isSameSize(window->frameGeometry().size(), frame.size())) {
        m_strictUntilSettled.remove(state.id);
        return false;
    }
    return true;
}

bool WindowApplier::consumeInFlightSize(Layout::WindowId id, const QSizeF &size)
{
    QList<QSizeF> &inFlight = m_inFlightSizes[id];
    const auto match = std::ranges::find_if(inFlight, [&size](const QSizeF &requested) { return isSameSize(requested, size); });
    if (match == inFlight.end()) {
        return false;
    }
    inFlight.erase(inFlight.begin(), match + 1);
    return true;
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
    return isSameSize(applied->size(), size);
}

bool WindowApplier::isEchoOfAppliedFrame(Layout::WindowId id, const QRectF &frame) const
{
    const auto applied = m_appliedFrames.constFind(id);
    return applied != m_appliedFrames.constEnd() && nearlyEqual(*applied, frame);
}

void WindowApplier::applyOpacity(KWin::Window *window, const Layout::WindowState &state)
{
    const auto overridden = m_overriddenOpacity.constFind(state.id);
    if (state.ruleOpacity >= 1.0) {
        if (overridden != m_overriddenOpacity.constEnd()) {
            window->setOpacity(*overridden);
            m_overriddenOpacity.erase(overridden);
        }
        return;
    }
    if (overridden == m_overriddenOpacity.constEnd()) {
        m_overriddenOpacity.insert(state.id, window->opacity());
    }
    if (!qFuzzyCompare(window->opacity(), state.ruleOpacity)) {
        window->setOpacity(state.ruleOpacity);
    }
}

void WindowApplier::forget(Layout::WindowId id)
{
    m_overriddenOpacity.remove(id);
    m_appliedFrames.remove(id);
    m_inFlightSizes.remove(id);
    m_strictUntilSettled.remove(id);
    m_loggedTargets.remove(id);
    if (const QPointer<KWin::Window> window = m_parked.take(id)) {
        setExemptFromConfinement(window, false);
    }
}

void WindowApplier::setParked(Layout::WindowId id, KWin::Window *window, bool parked)
{
    if (parked == m_parked.contains(id)) {
        return;
    }
    if (parked) {
        m_parked.insert(id, window);
    } else {
        m_parked.remove(id);
    }
    setExemptFromConfinement(window, parked);
}

void WindowApplier::logTargetChange(KWin::Window *window, const Layout::WindowState &state)
{
    const LoggedTarget target {state.targetFrame, state.output, state.workspace, state.columnIndex, state.isFloating, state.sizingMode,
        state.requestedSizingMode, state.isWindowedFullscreen, window->isFullScreen()};
    const auto previous = m_loggedTargets.constFind(state.id);
    if (previous != m_loggedTargets.constEnd() && *previous == target) {
        return;
    }
    m_loggedTargets.insert(state.id, target);
    qCInfo(lcKonveyor).nospace() << "konveyor: target for window " << state.id << " (" << window->resourceClass()
                                 << ") frame=" << target.frame << " output=" << target.output << " workspace=" << target.workspace
                                 << " column=" << target.column << " floating=" << target.floating
                                 << " sizing=" << static_cast<int>(target.sizing) << " requested=" << static_cast<int>(target.requested)
                                 << " windowedFullscreen=" << target.windowedFullscreen << " kwinFullscreen=" << target.kwinFullscreen
                                 << " current=" << window->frameGeometry() << " active=" << window->isActive();
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

void WindowApplier::applyGeometry(KWin::Window *window, const QRectF &frame, const std::optional<QSizeF> &requestedSize, bool tiled,
    bool allowResize, bool fullscreenPresentation) const
{
    if (frame.isEmpty()) {
        return;
    }
    if (fullscreenPresentation || window->isFullScreen() || window->isRequestedFullScreen()) {
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
        qCInfo(lcKonveyor) << "konveyor: setting" << window->resourceClass() << "fullscreen =" << fullscreen
                           << "requested mode =" << static_cast<int>(state.requestedSizingMode)
                           << "windowed fullscreen =" << state.isWindowedFullscreen;
        window->setFullScreen(fullscreen);
    }
    if (window->requestedMaximizeMode() != KWin::MaximizeRestore && window->isMaximizable()) {
        qCInfo(lcKonveyor) << "konveyor: restoring" << window->resourceClass() << "from KWin maximize";
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
