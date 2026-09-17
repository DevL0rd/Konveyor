#pragma once

#include <QHash>
#include <QPointF>

#include <optional>

namespace Konveyor::Layout
{

class TapTracker
{
public:
    static constexpr qint64 MaxTapMs = 250;

    explicit TapTracker(double maxTravel);

    void down(qint32 id, QPointF position, qint64 timestampMs);
    void motion(qint32 id, QPointF position);
    std::optional<int> up(qint32 id, qint64 timestampMs);
    void cancel();
    void invalidate();
    void press();
    int fingers() const { return m_fingers; }
    bool pressed() const { return m_pressed; }
    QPointF centroid() const { return m_centroid; }

private:
    double m_maxTravel;
    QHash<qint32, QPointF> m_starts;
    qint64 m_startMs = 0;
    int m_fingers = 0;
    bool m_valid = false;
    bool m_pressed = false;
    QPointF m_centroid;
};

}
