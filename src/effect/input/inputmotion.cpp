#include "input/inputmotion.h"

#include <cmath>

namespace Konveyor
{

namespace
{

constexpr double kNotch = 15.0;
constexpr double kNormalScrollRate = 5.0 * kNotch;
constexpr double kHalfScrollDistance = 10.0 * kNotch;
constexpr double kNormalPointerRate = 800.0;
constexpr double kNormalKeyRate = 5.0;
constexpr double kDirectionFloor = 1.0;
constexpr float kSettled = 0.001F;

float saturated(double amount, double half)
{
    return static_cast<float>(amount / (amount + half));
}

}

LeakyMeter::LeakyMeter(double seconds)
    : m_seconds(seconds)
{ }

void LeakyMeter::add(double amount, double at)
{
    m_total = total(at) + amount;
    m_at = at;
}

double LeakyMeter::total(double at) const
{
    return m_total * std::exp(-(at - m_at) / m_seconds);
}

double LeakyMeter::rate(double at) const
{
    return total(at) / m_seconds;
}

int InputMotion::scroll(double delta, double at)
{
    const double size = std::abs(delta);
    m_scrollRate.add(size, at);
    m_scrollSigned.add(delta, at);
    m_scrollDistance.add(size, at);
    if (m_stepCarry * delta < 0.0) {
        m_stepCarry = 0.0;
    }
    m_stepCarry += delta;
    const double steps = std::trunc(m_stepCarry / kNotch);
    m_stepCarry -= steps * kNotch;
    return static_cast<int>(std::abs(steps));
}

void InputMotion::pointer(double distance, double at)
{
    m_pointerRate.add(distance, at);
}

void InputMotion::key(double at)
{
    m_keyRate.add(1.0, at);
}

InputReadings InputMotion::read(double at) const
{
    const double scrollRate = m_scrollRate.rate(at);
    InputReadings readings;
    readings.scrollSpeed = saturated(scrollRate, kNormalScrollRate);
    readings.scrollDistance = saturated(m_scrollDistance.total(at), kHalfScrollDistance);
    readings.scrollDirection = static_cast<float>(m_scrollSigned.rate(at) / (scrollRate + kDirectionFloor));
    readings.pointerSpeed = saturated(m_pointerRate.rate(at), kNormalPointerRate);
    readings.typingRate = saturated(m_keyRate.rate(at), kNormalKeyRate);
    readings.settled = readings.scrollSpeed < kSettled && readings.scrollDistance < kSettled && readings.pointerSpeed < kSettled
        && readings.typingRate < kSettled;
    return readings;
}

}
