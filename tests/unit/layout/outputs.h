#pragma once

#include "layout/engine/engine.h"

#include <QList>
#include <QMarginsF>
#include <QRectF>

namespace LayoutTest
{

namespace Layout = Konveyor::Layout;

inline Layout::OutputInfo makeOutput(const QString &name, QRectF geometry, double scale = 1.0, const QString &serial = {})
{
    Layout::OutputInfo info;
    info.name = name;
    info.makeModelSerial = serial;
    info.geometry = geometry;
    info.workArea = geometry;
    info.scale = scale;
    return info;
}

inline Layout::OutputInfo withPanels(Layout::OutputInfo info, QMarginsF panels)
{
    info.workArea = info.geometry.marginsRemoved(panels);
    return info;
}

inline Layout::OutputInfo withSerial(Layout::OutputInfo info, const QString &serial)
{
    info.makeModelSerial = serial;
    return info;
}

inline QList<Layout::OutputInfo> mixedOutputs()
{
    return {
        makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080), 1.0, QStringLiteral("Dell U2720Q 1111")),
        withPanels(
            makeOutput(QStringLiteral("DP-2"), QRectF(-1280, -360, 1280, 800), 1.5, QStringLiteral("Lenovo 2222")), QMarginsF(0, 0, 0, 44)),
        makeOutput(QStringLiteral("HDMI-A-1"), QRectF(1920, 200, 1536, 864), 1.25, QStringLiteral("LG 3333")),
    };
}

inline QList<Layout::OutputInfo> scaledOutputs()
{
    return {
        makeOutput(QStringLiteral("eDP-1"), QRectF(0, 0, 1280, 800), 2.0),
        makeOutput(QStringLiteral("DP-3"), QRectF(-2560, -1440, 2560, 1440), 1.0),
        makeOutput(QStringLiteral("DP-4"), QRectF(1280, 0, 1706.6666, 960), 1.5),
    };
}

}
