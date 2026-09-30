#pragma once

#include "fuzzwindows.h"

#include <QMap>
#include <QSet>

#include <map>

namespace LayoutTest
{

struct FuzzFailure
{
    qsizetype index = -1;
    QString error;
    bool failed() const { return index >= 0; }
};

inline QString checkStates(const QList<Layout::WindowState> &states)
{
    QSet<Layout::WindowId> seen;
    for (const Layout::WindowState &state : states) {
        const bool valid = !seen.contains(state.id) && state.workspaceIndex >= 1 && !state.output.isEmpty() && state.renderAlpha >= 0.0
            && state.renderAlpha <= 1.0 && state.targetFrame.width() >= 0.0 && state.targetFrame.height() >= 0.0;
        if (!valid) {
            return QStringLiteral("invalid window state for id %1").arg(state.id);
        }
        seen.insert(state.id);
    }
    return {};
}

class FuzzRun
{
public:
    FuzzRun(quint32 arrangement, bool animated, std::map<QString, int> *successes = nullptr)
        : m_fixture(instantConfig(), QRectF(0, 0, 1920, 1080), closingHooks())
        , m_pool(arrangement % 2 == 0 ? mixedOutputs() : scaledOutputs())
        , m_animated(animated)
        , m_successes(successes)
    {
        m_fixture.removeOutput(QStringLiteral("DP-1"));
        plug(m_pool.first());
    }

    ~FuzzRun() { m_fixture.stopReporting(); }

    FuzzRun(const FuzzRun &) = delete;
    FuzzRun &operator=(const FuzzRun &) = delete;

    FuzzFailure run(const QList<Event> &events)
    {
        for (qsizetype i = 0; i < events.size(); ++i) {
            apply(events.at(i));
            QString error = m_fixture.invariants();
            error = error.isEmpty() ? checkStates(m_fixture.engine().windowStates()) : error;
            if (!error.isEmpty()) {
                return {i, error};
            }
        }
        return {};
    }

private:
    Layout::Hooks closingHooks()
    {
        Layout::Hooks hooks;
        hooks.closeWindow = [this](Layout::WindowId id) { m_closing.append(id); };
        return hooks;
    }

    Layout::WindowId windowAt(quint32 pick) const { return m_windows.at(static_cast<qsizetype>(pick % m_windows.size())); }
    QString outputAt(quint32 pick) const { return m_present.keys().at(static_cast<qsizetype>(pick % m_present.size())); }

    void plug(const Layout::OutputInfo &output)
    {
        m_present.insert(output.name, output);
        m_fixture.addOutput(output);
    }

    void unplug(const QString &name)
    {
        m_present.remove(name);
        m_fixture.removeOutput(name);
    }

    void apply(const Event &event)
    {
        const bool needsWindow = event.step != Step::AddWindow && event.step != Step::Act && event.step != Step::Reload
            && event.step != Step::AddOutput && event.step != Step::Advance && event.step != Step::LayoutFocus
            && event.step != Step::DataDrag && event.step != Step::RemoveOutput && event.step != Step::UpdateOutput;
        const bool needsOutput = event.step == Step::FocusOutput || event.step == Step::Swipe || event.step == Step::WorkspaceSwipe
            || event.step == Step::UpdateOutput || event.step == Step::DataDrag || event.step == Step::Drag;
        if ((needsWindow && m_windows.isEmpty()) || (needsOutput && m_present.isEmpty())) {
            return;
        }
        applyLayoutEvent(event) || applyOutputEvent(event) || applyWindowEvent(event) || applyGestureEvent(event);
        m_fixture.settle();
    }

    bool applyLayoutEvent(const Event &event)
    {
        switch (event.step) {
        case Step::Act:
            act(event.action);
            return true;
        case Step::Reload:
            m_fixture.setConfig(randomConfig(event.a, m_animated));
            return true;
        case Step::Advance:
            if (event.a % 5 == 0) {
                m_fixture.advance(61'000);
            } else {
                m_fixture.advanceInSteps(event.a % 200);
            }
            return true;
        case Step::LayoutFocus:
            m_fixture.engine().setLayoutFocused(event.a % 2 == 0);
            return true;
        default:
            return false;
        }
    }

    bool applyOutputEvent(const Event &event)
    {
        switch (event.step) {
        case Step::AddOutput: {
            const Layout::OutputInfo &output = m_pool.at(static_cast<qsizetype>(event.a % m_pool.size()));
            if (!m_present.contains(output.name)) {
                plug(output);
            }
            return true;
        }
        case Step::RemoveOutput:
            if (m_present.size() > 1 || (!m_present.isEmpty() && event.b % 8 == 0)) {
                unplug(outputAt(event.a));
            }
            return true;
        case Step::UpdateOutput:
            updateOutput(outputAt(event.a), event.b, event.c);
            return true;
        case Step::FocusOutput:
            m_fixture.engine().focusOutput(outputAt(event.a));
            return true;
        default:
            return false;
        }
    }

    void updateOutput(const QString &name, quint32 variant, quint32 value)
    {
        Layout::OutputInfo output = m_present.value(name);
        switch (variant % 5) {
        case 0:
            output.scale = std::initializer_list<double> {1.0, 1.25, 1.5, 2.0}.begin()[value % 4];
            break;
        case 1:
            output = withPanels(output, value % 2 == 0 ? QMarginsF(0, 0, 0, 44) : QMarginsF(48, 30, 0, 0));
            break;
        case 2:
            output.geometry.setSize(output.geometry.size() * (value % 2 == 0 ? 0.75 : 1.25));
            output.workArea = output.geometry;
            break;
        case 3:
            unplug(name);
            plug(withSerial(output, QStringLiteral("replacement %1").arg(value % 3)));
            return;
        default:
            unplug(name);
            plug(output);
            return;
        }
        m_present.insert(name, output);
        m_fixture.engine().updateOutput(output);
    }

    void act(Config::Action action)
    {
        for (auto &[key, value] : action.properties) {
            if (value == WindowPlaceholder) {
                value
                    = m_windows.isEmpty() ? QStringLiteral("999999") : QString::number(windowAt(static_cast<quint32>(action.name.size())));
            }
        }
        const Layout::ActionResult result = m_fixture.engine().perform(action);
        if (result.ok && m_successes) {
            (*m_successes)[action.name] += 1;
        }
        for (const Layout::WindowId id : std::exchange(m_closing, {})) {
            closeWindow(id);
        }
    }

    void closeWindow(Layout::WindowId id)
    {
        if (m_windows.removeAll(id) > 0) {
            m_properties.remove(id);
            m_fixture.remove(id);
        }
    }

    void addWindow(const Event &event)
    {
        if (m_windows.size() >= 14) {
            closeWindow(windowAt(event.a));
            return;
        }
        const std::optional<Layout::WindowId> parent
            = (event.a >> 8) % 8 == 0 && !m_windows.isEmpty() ? std::optional(windowAt(event.a >> 12)) : std::nullopt;
        const Layout::WindowProperties properties = fuzzWindowProperties(event, parent);
        const auto policy = std::initializer_list<Layout::ActivationPolicy> {Layout::ActivationPolicy::Focus,
            Layout::ActivationPolicy::Smart, Layout::ActivationPolicy::NoFocus}
                                .begin()[(event.c >> 12) % 3];
        const QString preferred = (event.c >> 16) % 4 == 0 ? outputNames().at((event.c >> 20) % 6) : QString();
        const Layout::WindowId id = m_fixture.addClient(properties, fuzzClient(properties, event.b >> 12), policy, preferred);
        if (m_fixture.engine().hasWindow(id)) {
            m_windows.append(id);
            m_properties.insert(id, properties);
        }
    }

    bool applyWindowEvent(const Event &event)
    {
        Layout::Engine &engine = m_fixture.engine();
        switch (event.step) {
        case Step::AddWindow:
            addWindow(event);
            return true;
        case Step::RemoveWindow:
            closeWindow(windowAt(event.a));
            return true;
        case Step::Fullscreen:
            engine.setWindowFullscreen(windowAt(event.a), event.b % 2 == 0);
            return true;
        case Step::FillWidth:
            engine.toggleWindowFillWidth(windowAt(event.a));
            return true;
        case Step::Urgent:
            engine.setWindowUrgent(windowAt(event.a), event.b % 2 == 0);
            return true;
        case Step::Activate:
            engine.activateWindow(windowAt(event.a));
            return true;
        case Step::FloatingFrame:
            engine.setFloatingFrame(windowAt(event.a),
                QRectF(double(event.a % 4000) - 1500, double(event.b % 2000) - 500, 10 + event.c % 1500, 10 + (event.c >> 12) % 1000));
            return true;
        case Step::UpdateProperties: {
            const Layout::WindowId id = windowAt(event.a);
            const Layout::WindowProperties properties = changedProperties(m_properties.value(id), event.b);
            m_properties.insert(id, properties);
            engine.updateWindowProperties(id, properties);
            return true;
        }
        default:
            return false;
        }
    }

    bool applyGestureEvent(const Event &event)
    {
        Layout::Engine &engine = m_fixture.engine();
        const double delta = double(event.c % 600) - 300;
        switch (event.step) {
        case Step::Swipe:
        case Step::WorkspaceSwipe: {
            const bool touchpad = event.b % 2 == 0;
            const bool workspace = event.step == Step::WorkspaceSwipe;
            workspace ? engine.beginWorkspaceSwipe(outputAt(event.a), touchpad) : engine.beginSwipe(outputAt(event.a), touchpad);
            for (int i = 0; i < 3; ++i) {
                m_timestamp += 16;
                workspace ? engine.updateWorkspaceSwipe(delta, m_timestamp, touchpad) : engine.updateSwipe(delta, m_timestamp, touchpad);
            }
            workspace ? engine.endWorkspaceSwipe(touchpad) : engine.endSwipe(touchpad);
            return true;
        }
        case Step::Drag: {
            const Layout::WindowId id = windowAt(event.a);
            const QString output = outputAt(event.b);
            const QPointF target = m_present.value(output).geometry.center() + QPointF(delta, delta / 3.0);
            if (engine.beginWindowDrag(id, m_fixture.frame(id).center())) {
                engine.updateWindowDrag(m_fixture.frame(id).center() + QPointF(40, 10), output);
                engine.updateWindowDrag(target, output);
                if (event.c % 4 == 0) {
                    engine.toggleWindowDragFloating();
                }
                engine.endWindowDrag();
            }
            return true;
        }
        case Step::Resize: {
            const quint8 edges = std::initializer_list<quint8> {8, 4, 2, 1, 10, 5}.begin()[event.b % 6];
            if (engine.beginResize(windowAt(event.a), edges)) {
                engine.updateResize(QPointF(delta, delta / 2.0));
                engine.endResize();
            }
            return true;
        }
        case Step::DataDrag: {
            const QString output = outputAt(event.a);
            engine.beginDataDrag();
            m_timestamp += 16;
            engine.dataDragEdgeScroll(output, m_present.value(output).geometry.topLeft() + QPointF(2, 300), m_timestamp);
            m_timestamp += 400;
            engine.dataDragEdgeScroll(output, m_present.value(output).geometry.topLeft() + QPointF(2, 300), m_timestamp);
            engine.endDataDrag();
            return true;
        }
        default:
            return false;
        }
    }

    Fixture m_fixture;
    QList<Layout::OutputInfo> m_pool;
    bool m_animated = false;
    std::map<QString, int> *m_successes = nullptr;
    QMap<QString, Layout::OutputInfo> m_present;
    QList<Layout::WindowId> m_windows;
    QHash<Layout::WindowId, Layout::WindowProperties> m_properties;
    QList<Layout::WindowId> m_closing;
    qint64 m_timestamp = 0;
};

}
