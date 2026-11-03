#pragma once

#include "config/types/borderstyle.h"

#include <QTest>

#include <cmath>

namespace Konveyor::Render::Testing
{

inline void compareClose(double actual, double expected, double tolerance = 1e-5)
{
    QVERIFY2(std::abs(actual - expected) < tolerance, qPrintable(QStringLiteral("%1 vs %2").arg(actual).arg(expected)));
}

inline Config::BorderLayer layerOf(Config::BorderLayerKind kind)
{
    Config::BorderLayer layer;
    layer.kind = kind;
    Config::applyKindDefaults(layer);
    return layer;
}

inline Config::LayerDrive driveOf(const QString &setting, Config::Signal signal, double amount = 1.0, double smooth = 0.0,
    Config::DriveCurve curve = Config::DriveCurve::Linear)
{
    return {setting, signal, amount, smooth, curve};
}

}
