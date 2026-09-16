#include "values/configvalues.h"

namespace Konveyor::Settings
{

namespace
{

QVariantList sizesValue(const QList<Config::PresetSize> &sizes)
{
    QVariantList list;
    for (const Config::PresetSize &size : sizes) {
        list.append(sizeValue(size));
    }
    return list;
}

QVariantMap borderValue(const Config::Border &border)
{
    return {
        {QStringLiteral("enabled"), border.enabled},
        {QStringLiteral("width"), border.width},
        {QStringLiteral("active"), paintValue(border.active)},
        {QStringLiteral("inactive"), paintValue(border.inactive)},
        {QStringLiteral("urgent"), paintValue(border.urgent)},
    };
}

QVariantMap tabIndicatorValue(const Config::TabIndicator &indicator)
{
    static const QStringList positions {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("top"), QStringLiteral("bottom")};
    return {
        {QStringLiteral("enabled"), indicator.enabled},
        {QStringLiteral("hide-when-single-tab"), indicator.hideWhenSingleTab},
        {QStringLiteral("place-within-column"), indicator.placeWithinColumn},
        {QStringLiteral("gap"), indicator.gap},
        {QStringLiteral("width"), indicator.width},
        {QStringLiteral("length"), indicator.lengthTotalProportion},
        {QStringLiteral("position"), positions.at(static_cast<qsizetype>(indicator.position))},
        {QStringLiteral("gaps-between-tabs"), indicator.gapsBetweenTabs},
        {QStringLiteral("corner-radius"), indicator.cornerRadius},
        {QStringLiteral("active"), paintValue(indicator.active)},
        {QStringLiteral("inactive"), paintValue(indicator.inactive)},
        {QStringLiteral("urgent"), paintValue(indicator.urgent)},
    };
}

}

QVariantMap layoutValues(const Config::Layout &layout)
{
    static const QStringList centerModes {QStringLiteral("never"), QStringLiteral("always"), QStringLiteral("on-overflow")};
    return {
        {QStringLiteral("gaps"), layout.gaps},
        {QStringLiteral("center-focused-column"), centerModes.at(static_cast<qsizetype>(layout.centerFocusedColumn))},
        {QStringLiteral("new-column-position"),
            layout.newColumnPosition == Config::NewColumnPosition::Left ? QStringLiteral("left") : QStringLiteral("right")},
        {QStringLiteral("always-center-single-column"), layout.alwaysCenterSingleColumn},
        {QStringLiteral("empty-workspace-above-first"), layout.emptyWorkspaceAboveFirst},
        {QStringLiteral("default-column-display"),
            layout.defaultColumnDisplay == Config::ColumnDisplay::Tabbed ? QStringLiteral("tabbed") : QStringLiteral("normal")},
        {QStringLiteral("preset-column-widths"), sizesValue(layout.presetColumnWidths)},
        {QStringLiteral("default-column-width"), sizeValue(layout.defaultColumnWidth)},
        {QStringLiteral("preset-window-heights"), sizesValue(layout.presetWindowHeights)},
        {QStringLiteral("struts"),
            QVariantMap {
                {QStringLiteral("left"), layout.struts.left},
                {QStringLiteral("right"), layout.struts.right},
                {QStringLiteral("top"), layout.struts.top},
                {QStringLiteral("bottom"), layout.struts.bottom},
            }},
        {QStringLiteral("focus-ring"), borderValue(layout.focusRing)},
        {QStringLiteral("border"), borderValue(layout.border)},
        {QStringLiteral("tab-indicator"), tabIndicatorValue(layout.tabIndicator)},
        {QStringLiteral("insert-hint"),
            QVariantMap {
                {QStringLiteral("enabled"), layout.insertHint.enabled}, {QStringLiteral("paint"), paintValue(layout.insertHint.paint)}}},
        {QStringLiteral("background-color"), colorValue(layout.backgroundColor)},
    };
}

Config::Layout scopedLayout(const Config::Config &config, const QString &kind, const QString &name)
{
    const auto layoutOf = [&name, &config](const auto &list) {
        for (const auto &entry : list) {
            if (entry.name == name && entry.layout) {
                return *entry.layout;
            }
        }
        return config.layout;
    };
    if (kind == QLatin1String("output")) {
        return layoutOf(config.outputs);
    }
    if (kind == QLatin1String("monitor-profile")) {
        return layoutOf(config.monitorProfiles);
    }
    if (kind == QLatin1String("workspace")) {
        return layoutOf(config.workspaces);
    }
    return config.layout;
}

}
