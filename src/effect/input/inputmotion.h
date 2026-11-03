#pragma once

namespace Konveyor
{

class LeakyMeter
{
public:
    explicit LeakyMeter(double seconds);

    void add(double amount, double at);
    double total(double at) const;
    double rate(double at) const;

private:
    double m_seconds;
    double m_total = 0.0;
    double m_at = 0.0;
};

struct InputReadings
{
    float scrollSpeed = 0.0F;
    float scrollDistance = 0.0F;
    float scrollDirection = 0.0F;
    float pointerSpeed = 0.0F;
    float typingRate = 0.0F;
    bool settled = true;
};

class InputMotion
{
public:
    int scroll(double delta, double at);
    void pointer(double distance, double at);
    void key(double at);
    InputReadings read(double at) const;

private:
    LeakyMeter m_scrollRate {0.25};
    LeakyMeter m_scrollSigned {0.25};
    LeakyMeter m_scrollDistance {2.0};
    LeakyMeter m_pointerRate {0.15};
    LeakyMeter m_keyRate {1.0};
    double m_stepCarry = 0.0;
};

}
