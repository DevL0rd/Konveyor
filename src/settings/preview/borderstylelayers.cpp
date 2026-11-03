#include "preview/borderstylelayers.h"

#include "render/borderlayer.h"
#include "values/configvalues.h"

#include <algorithm>

namespace Konveyor::Settings
{

namespace
{

QList<QColor> resolvedColors(const QVariantList &paints, const QVariantMap &theme)
{
    QList<QColor> colors;
    colors.reserve(paints.size());
    for (const QVariant &value : paints) {
        const QVariantMap paint = value.toMap();
        const QString source = paint.value(QStringLiteral("source")).toString();
        colors.append((source == QStringLiteral("color") ? paint.value(QStringLiteral("color")) : theme.value(source)).value<QColor>());
    }
    if (colors.isEmpty()) {
        colors.append(theme.value(QStringLiteral("accent")).value<QColor>());
    }
    return colors;
}

}

QList<PreviewLayer> previewLayers(const QVariantList &layers, const QVariantMap &theme)
{
    QList<PreviewLayer> parsed;
    parsed.reserve(layers.size());
    for (const QVariant &value : layers) {
        const QVariantMap map = value.toMap();
        parsed.append({borderLayerWithoutColors(map), resolvedColors(map.value(QStringLiteral("colors")).toList(), theme)});
    }
    return parsed;
}

bool anyLayerMoves(const QList<PreviewLayer> &layers)
{
    return std::ranges::any_of(layers, [](const PreviewLayer &layer) { return Render::layerRunsContinuously(layer.config); });
}

}
