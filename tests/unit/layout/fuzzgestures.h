#pragma once

#include "fuzzwindows.h"

#include <initializer_list>

namespace LayoutTest
{

struct GestureTarget
{
    std::optional<Layout::WindowId> window;
    QString output;
    QRectF outputGeometry;
};

inline bool gestureNeedsWindow(const Event &event)
{
    const Phase phase = gesturePhase(event);
    return (event.step == Step::Drag || event.step == Step::Resize) && (phase == Phase::Begin || phase == Phase::Whole);
}

class GestureDriver
{
public:
    explicit GestureDriver(Fixture &fixture)
        : m_fixture(fixture)
    { }

    bool holdsDrag() const { return m_holdingDrag; }

    bool apply(const Event &event, const GestureTarget &target)
    {
        switch (event.step) {
        case Step::Swipe:
            swipe(event, target);
            return true;
        case Step::WorkspaceSwipe:
            workspaceSwipe(event, target);
            return true;
        case Step::Drag:
            drag(event, target);
            return true;
        case Step::Resize:
            resize(event, target);
            return true;
        case Step::DataDrag:
            dataDrag(event, target);
            return true;
        default:
            return false;
        }
    }

private:
    Layout::Engine &engine() { return m_fixture.engine(); }

    static double delta(const Event &event) { return double(event.c % 600) - 300; }

    static bool runs(const Event &event, Phase phase)
    {
        const Phase actual = gesturePhase(event);
        return actual == phase || actual == Phase::Whole;
    }

    qint64 nextTimestamp(qint64 step)
    {
        m_timestamp += step;
        return m_timestamp;
    }

    void swipe(const Event &event, const GestureTarget &target)
    {
        const bool touchpad = event.b % 2 == 0;
        if (runs(event, Phase::Begin)) {
            engine().beginSwipe(target.output, touchpad);
        }
        if (runs(event, Phase::Update)) {
            for (int i = 0; i < 3; ++i) {
                engine().updateSwipe(delta(event), nextTimestamp(16), touchpad);
            }
        }
        if (runs(event, Phase::End)) {
            engine().endSwipe(touchpad, event.b % 5 == 0 ? target.window : std::nullopt);
        }
    }

    void workspaceSwipe(const Event &event, const GestureTarget &target)
    {
        const bool touchpad = event.b % 2 == 0;
        if (runs(event, Phase::Begin)) {
            engine().beginWorkspaceSwipe(target.output, touchpad);
        }
        if (runs(event, Phase::Update)) {
            for (int i = 0; i < 3; ++i) {
                engine().updateWorkspaceSwipe(delta(event), nextTimestamp(16), touchpad);
            }
        }
        if (runs(event, Phase::End)) {
            engine().endWorkspaceSwipe(event.b % 3 == 0 ? std::optional(touchpad) : std::nullopt);
        }
    }

    void drag(const Event &event, const GestureTarget &target)
    {
        const QPointF point = target.outputGeometry.center() + QPointF(delta(event), delta(event) / 3.0);
        if (runs(event, Phase::Begin) && target.window) {
            m_holdingDrag = engine().beginWindowDrag(*target.window, m_fixture.frame(*target.window).center()) || m_holdingDrag;
        }
        if (runs(event, Phase::Update)) {
            engine().updateWindowDrag(point + QPointF(40, 10), target.output);
            engine().updateWindowDrag(point, target.output);
            if (event.b % 4 == 0) {
                engine().toggleWindowDragFloating();
            }
        }
        if (runs(event, Phase::End)) {
            engine().endWindowDrag();
            m_holdingDrag = false;
        }
    }

    void resize(const Event &event, const GestureTarget &target)
    {
        if (runs(event, Phase::Begin) && target.window) {
            const quint8 edges = std::initializer_list<quint8> {8, 4, 2, 1, 10, 5}.begin()[event.b % 6];
            engine().beginResize(*target.window, edges);
        }
        if (runs(event, Phase::Update)) {
            engine().updateResize(QPointF(delta(event), delta(event) / 2.0));
        }
        if (runs(event, Phase::End)) {
            engine().endResize();
        }
    }

    void dataDrag(const Event &event, const GestureTarget &target)
    {
        if (runs(event, Phase::Begin)) {
            engine().beginDataDrag();
        }
        if (runs(event, Phase::Update)) {
            const QPointF edge = target.outputGeometry.topLeft() + QPointF(event.b % 2 == 0 ? 2 : target.outputGeometry.width() - 2, 300);
            engine().dataDragEdgeScroll(target.output, edge, nextTimestamp(16));
            engine().dataDragEdgeScroll(target.output, edge, nextTimestamp(400));
        }
        if (runs(event, Phase::End)) {
            engine().endDataDrag();
        }
    }

    Fixture &m_fixture;
    qint64 m_timestamp = 0;
    bool m_holdingDrag = false;
};

}
