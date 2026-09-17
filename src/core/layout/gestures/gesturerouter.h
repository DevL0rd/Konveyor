#pragma once

#include "config/types.h"
#include "layout/gestures/taptracker.h"

#include <QHash>
#include <QPointF>
#include <QSet>
#include <QString>

#include <optional>

namespace Konveyor::Layout
{

class Engine;

enum class GestureDevice
{
    Touchpad,
    Touchscreen
};

class GestureRouter
{
public:
    explicit GestureRouter(Engine &engine);

    void setConfig(const Config::Gestures &gestures);
    const Config::Gestures &config() const { return m_config; }

    bool touchpadSwipeBegin(int fingers, const QString &output);
    bool touchpadSwipeUpdate(QPointF delta, qint64 timestampMs);
    bool touchpadSwipeEnd();
    bool touchpadPinchBegin(int fingers);
    bool touchpadPinchUpdate(double scale);
    bool touchpadPinchEnd();
    void touchpadContactDown(qint32 slot, QPointF millimeters, qint64 timestampMs);
    void touchpadContactMotion(qint32 slot, QPointF millimeters);
    bool touchpadContactUp(qint32 slot, qint64 timestampMs);
    void touchpadPhysicalClick();
    void touchpadContactsReset();
    bool takesTouchpadTapButton() const;

    bool touchDown(qint32 id, QPointF position, qint64 timestampMs, const QString &output);
    bool touchMotion(qint32 id, QPointF position, qint64 timestampMs);
    bool touchUp(qint32 id, qint64 timestampMs);
    void touchCancel();
    bool resetTouches();

    int touchPointCount() const { return static_cast<int>(m_points.size()); }
    bool isTouchGestureActive() const { return m_touch.active; }
    std::optional<QPointF> lastTouchPosition() const { return m_lastTouch; }
    bool isLongPress(qint64 timestampMs) const;
    bool hasFirstTouchMoved() const { return m_firstTouchMoved; }

private:
    enum class Axis
    {
        Undecided,
        Horizontal,
        Vertical,
        Pinch,
        WindowHorizontal,
        WindowVertical,
        Ignored
    };

    struct Swipe
    {
        bool active = false;
        GestureDevice device = GestureDevice::Touchpad;
        QString output;
        Axis axis = Axis::Undecided;
        QPointF pending;
        bool allowsSwipe = false;
        bool allowsPinch = false;
        bool allowsWindowSwipe = false;
        double travel = 0.0;
        bool pinchTriggered = false;
        double startSpread = 0.0;
    };

    const Config::MultiTouch &settingsFor(GestureDevice device) const;
    struct Allowed
    {
        bool swipe = false;
        bool pinch = false;
        bool windowSwipe = false;
        bool tap = false;
    };

    static Allowed allowedFor(const Config::MultiTouch &settings, int fingers);
    void beginGesture(Swipe &gesture, GestureDevice device, const QString &output, Allowed allowed);
    bool decideAxis(Swipe &gesture, QPointF delta);
    void focusWindowUnderFingers(const Swipe &gesture);
    void feedWindowSwipe(Swipe &gesture, double delta);
    void feedTranslation(Swipe &gesture, QPointF delta, qint64 timestampMs);
    void feedPinch(Swipe &gesture, double scale);
    bool finishGesture(Swipe &gesture);
    QPointF centroid() const;
    double spread() const;
    bool performTap(const Config::MultiTouch &settings, int fingers, std::optional<QPointF> position);

    Engine &m_engine;
    Config::Gestures m_config;
    Swipe m_touchpad;
    Swipe m_touch;
    TapTracker m_touchpadTaps;
    TapTracker m_touchTaps;
    QHash<qint32, QPointF> m_points;
    QSet<qint32> m_gestureIds;
    QPointF m_lastCentroid;
    QPointF m_firstTouchStart;
    std::optional<QPointF> m_lastTouch;
    qint64 m_firstTouchMs = 0;
    qint64 m_firstMoveMs = 0;
    bool m_firstTouchMoved = false;
};

}
