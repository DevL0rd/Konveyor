#include "anim/easing.h"

#include <cmath>

namespace Konveyor::Anim
{

namespace
{

qreal exponentialOut(qreal x)
{
    return 1.0 - std::pow(2.0, -10.0 * x);
}

QEasingCurve qtCurveFor(Config::EasingCurve kind)
{
    switch (kind) {
    case Config::EasingCurve::Linear:
    case Config::EasingCurve::CubicBezier:
        return QEasingCurve(QEasingCurve::Linear);
    case Config::EasingCurve::EaseOutQuad:
        return QEasingCurve(QEasingCurve::OutQuad);
    case Config::EasingCurve::EaseOutCubic:
        return QEasingCurve(QEasingCurve::OutCubic);
    case Config::EasingCurve::EaseOutExpo: {
        QEasingCurve curve;
        curve.setCustomType(exponentialOut);
        return curve;
    }
    }
    Q_UNREACHABLE();
}

}

Curve::Curve(Config::EasingCurve kind)
    : Curve(kind, qtCurveFor(kind))
{ }

Curve::Curve(Config::EasingCurve kind, QEasingCurve curve)
    : m_kind(kind)
    , m_curve(std::move(curve))
{ }

Curve Curve::fromConfig(const Config::EasingParams &params)
{
    if (params.curve != Config::EasingCurve::CubicBezier) {
        return Curve(params.curve);
    }
    QEasingCurve bezier(QEasingCurve::BezierSpline);
    bezier.addCubicBezierSegment(QPointF(params.x1, params.y1), QPointF(params.x2, params.y2), QPointF(1.0, 1.0));
    return Curve(params.curve, std::move(bezier));
}

Config::EasingCurve Curve::kind() const
{
    return m_kind;
}

double Curve::y(double x) const
{
    if (m_kind == Config::EasingCurve::EaseOutExpo) {
        return exponentialOut(x);
    }
    return m_curve.valueForProgress(x);
}

}
