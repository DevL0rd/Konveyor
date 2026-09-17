#pragma once

#include "config/types.h"

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

    bool touchpadSwipeBegin(int fingers, const QString &output);
    bool touchpadSwipeUpdate(QPointF delta, qint64 timestampMs);
    bool touchpadSwipeEnd();
    bool touchpadPinchBegin(int fingers);
    bool touchpadPinchUpdate(double scale);
    bool touchpadPinchEnd();

    bool touchDown(qint32 id, QPointF position, qint64 timestampMs, const QString &output);
    bool touchMotion(qint32 id, QPointF position, qint64 timestampMs);
    bool touchUp(qint32 id);
    void touchCancel();

    int touchPointCount() const { return static_cast<int>(m_points.size()); }
    bool isTouchGestureActive() const { return m_touch.active; }
    std::optional<QPointF> lastTouchPosition() const { return m_lastTouch; }
    bool isLongPress(qint64 timestampMs) const;

private:
    enum class Axis
    {
        Undecided,
        Horizontal,
        Vertical,
        Pinch,
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
        bool pinchTriggered = false;
        double startSpread = 0.0;
    };

    const Config::MultiTouch &settingsFor(GestureDevice device) const;
    void beginGesture(Swipe &gesture, GestureDevice device, const QString &output, bool allowsSwipe, bool allowsPinch);
    void feedTranslation(Swipe &gesture, QPointF delta, qint64 timestampMs);
    void feedPinch(Swipe &gesture, double scale);
    bool finishGesture(Swipe &gesture);
    QPointF centroid() const;
    double spread() const;

    Engine &m_engine;
    Config::Gestures m_config;
    Swipe m_touchpad;
    Swipe m_touch;
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
