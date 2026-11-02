#pragma once

#include "config/types.h"

#include <QColor>
#include <QList>
#include <QVariantList>
#include <QVariantMap>

namespace Konveyor::Settings
{

struct PreviewLayer
{
    Config::BorderLayer config;
    QList<QColor> colors;
};

QList<PreviewLayer> previewLayers(const QVariantList &layers, const QVariantMap &theme);
bool anyLayerMoves(const QList<PreviewLayer> &layers);

}
