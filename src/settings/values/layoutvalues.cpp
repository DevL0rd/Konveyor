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

QVariantMap eventEffectsValue(const Config::EventEffects &effects)
{
    static const QStringList events {QStringLiteral("focus"), QStringLiteral("unfocus"), QStringLiteral("click"), QStringLiteral("scroll"),
        QStringLiteral("keypress"), QStringLiteral("urgent"), QStringLiteral("beat"), QStringLiteral("view")};
    static const QStringList reaches {QStringLiteral("self"), QStringLiteral("neighbors"), QStringLiteral("all")};
    QVariantMap values;
    for (std::size_t index = 0; index < Config::kEventTriggerCount; ++index) {
        const std::optional<Config::EventEffect> &effect = effects.slots_[index];
        if (effect) {
            values.insert(events.at(static_cast<qsizetype>(index)),
                QVariantMap {
                    {QStringLiteral("style"), effect->style},
                    {QStringLiteral("reach"), reaches.at(static_cast<qsizetype>(effect->reach))},
                    {QStringLiteral("mode"), effect->mode == Config::EventMode::While ? QStringLiteral("while") : QStringLiteral("once")},
                    {QStringLiteral("duration"), effect->duration.value_or(0.0)},
                });
        }
    }
    return values;
}

QVariantMap borderValue(const Config::Border &border)
{
    return {
        {QStringLiteral("enabled"), border.enabled},
        {QStringLiteral("width"), border.width},
        {QStringLiteral("active"), paintValue(border.active)},
        {QStringLiteral("inactive"), paintValue(border.inactive)},
        {QStringLiteral("urgent"), paintValue(border.urgent)},
        {QStringLiteral("active-style"), border.activeStyle},
        {QStringLiteral("inactive-style"), border.inactiveStyle},
        {QStringLiteral("urgent-style"), border.urgentStyle},
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
    static const QStringList groupModes {QStringLiteral("off"), QStringLiteral("beside"), QStringLiteral("stack")};
    return {
        {QStringLiteral("gaps"), layout.gaps},
        {QStringLiteral("center-focused-column"), centerModes.at(static_cast<qsizetype>(layout.centerFocusedColumn))},
        {QStringLiteral("new-column-position"),
            layout.newColumnPosition == Config::NewColumnPosition::Left ? QStringLiteral("left") : QStringLiteral("right")},
        {QStringLiteral("always-center-single-column"), layout.alwaysCenterSingleColumn},
        {QStringLiteral("wrap-columns"), layout.wrapColumns},
        {QStringLiteral("always-expand-single-column"), layout.alwaysExpandSingleColumn},
        {QStringLiteral("empty-workspace-above-first"), layout.emptyWorkspaceAboveFirst},
        {QStringLiteral("fixed-workspace-count"), layout.fixedWorkspaceCount},
        {QStringLiteral("horizontal-desktops"), layout.horizontalDesktops},
        {QStringLiteral("wrap-workspaces"), layout.wrapWorkspaces},
        {QStringLiteral("remember-window-sizes"), layout.rememberWindowSizes},
        {QStringLiteral("remember-window-positions"), layout.rememberWindowPositions},
        {QStringLiteral("group-app-windows"), groupModes.at(static_cast<qsizetype>(layout.groupAppWindows))},
        {QStringLiteral("max-rows-per-column"), layout.maxRowsPerColumn},
        {QStringLiteral("new-window-placement"),
            layout.newWindowPlacement == Config::NewWindowPlacement::Stack ? QStringLiteral("stack") : QStringLiteral("column")},
        {QStringLiteral("float-child-windows"), layout.floatChildWindows},
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
        {QStringLiteral("focus-ring-transition"),
            layout.focusRingTransition == Config::FocusRingTransition::Crossfade
                ? QStringLiteral("crossfade")
                : (layout.focusRingTransition == Config::FocusRingTransition::Delayed ? QStringLiteral("delayed")
                                                                                      : QStringLiteral("ride"))},
        {QStringLiteral("border"), borderValue(layout.border)},
        {QStringLiteral("event-effects"), eventEffectsValue(layout.eventEffects)},
        {QStringLiteral("tab-indicator"), tabIndicatorValue(layout.tabIndicator)},
        {QStringLiteral("insert-hint"),
            QVariantMap {
                {QStringLiteral("enabled"), layout.insertHint.enabled}, {QStringLiteral("paint"), paintValue(layout.insertHint.paint)}}},
        {QStringLiteral("background-color"), colorValue(layout.backgroundColor)},
    };
}

}
