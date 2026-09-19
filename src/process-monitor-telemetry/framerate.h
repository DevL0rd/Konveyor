#pragma once

#include <QList>

#include <optional>

namespace ProcessMonitor
{

struct FrameRate
{
    int fps = 0;
    double frametime = 0;
    int fpsLow = 0;
};

class FrameRateTracker
{
public:
    std::optional<double> addFrame(qint64 timestampNs);
    std::optional<FrameRate> rateAt(qint64 timestampNs) const;

private:
    QList<qint64> m_frames;
};

}
