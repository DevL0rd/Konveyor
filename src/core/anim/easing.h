#pragma once

#include "config/types.h"

#include <QEasingCurve>

namespace Konveyor::Anim
{

class Curve
{
public:
    explicit Curve(Config::EasingCurve kind);

    static Curve fromConfig(const Config::EasingParams &params);

    Config::EasingCurve kind() const;
    double y(double x) const;

private:
    Curve(Config::EasingCurve kind, QEasingCurve curve);

    Config::EasingCurve m_kind;
    QEasingCurve m_curve;
};

}
