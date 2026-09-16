#include "config/color.h"
#include "config/loader.h"
#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

QList<PresetSize> defaultPresets()
{
    return {Proportion {1.0 / 3.0}, Proportion {0.5}, Proportion {2.0 / 3.0}};
}

void mergeTabIndicator(TabIndicator &base, const TabIndicatorPart &part)
{
    if (part.enabled) {
        base.enabled = *part.enabled;
    }
    if (part.hideWhenSingleTab) {
        base.hideWhenSingleTab = *part.hideWhenSingleTab;
    }
    if (part.placeWithinColumn) {
        base.placeWithinColumn = *part.placeWithinColumn;
    }
    if (part.gap) {
        base.gap = *part.gap;
    }
    if (part.width) {
        base.width = *part.width;
    }
    if (part.lengthTotalProportion) {
        base.lengthTotalProportion = *part.lengthTotalProportion;
    }
    if (part.position) {
        base.position = *part.position;
    }
    if (part.gapsBetweenTabs) {
        base.gapsBetweenTabs = *part.gapsBetweenTabs;
    }
    if (part.cornerRadius) {
        base.cornerRadius = *part.cornerRadius;
    }
    mergeOptionalPaint(base.active, part.colors.active);
    mergeOptionalPaint(base.inactive, part.colors.inactive);
    mergeOptionalPaint(base.urgent, part.colors.urgent);
}

void mergeInsertHint(InsertHint &base, const InsertHintPart &part)
{
    if (part.enabled) {
        base.enabled = *part.enabled;
    }
    mergePaint(base.paint, part.paint);
}

void mergeLayoutSizing(Layout &base, const LayoutPart &part)
{
    if (part.gaps) {
        base.gaps = *part.gaps;
    }
    if (part.centerFocusedColumn) {
        base.centerFocusedColumn = *part.centerFocusedColumn;
    }
    if (part.newColumnPosition) {
        base.newColumnPosition = *part.newColumnPosition;
    }
    if (part.alwaysCenterSingleColumn) {
        base.alwaysCenterSingleColumn = *part.alwaysCenterSingleColumn;
    }
    if (part.emptyWorkspaceAboveFirst) {
        base.emptyWorkspaceAboveFirst = *part.emptyWorkspaceAboveFirst;
    }
    if (part.defaultColumnDisplay) {
        base.defaultColumnDisplay = *part.defaultColumnDisplay;
    }
    if (part.presetColumnWidths) {
        base.presetColumnWidths = *part.presetColumnWidths;
    }
    if (part.presetWindowHeights) {
        base.presetWindowHeights = *part.presetWindowHeights;
    }
    if (part.defaultColumnWidth) {
        base.defaultColumnWidth = *part.defaultColumnWidth;
    }
    if (part.struts) {
        base.struts = *part.struts;
    }
}

}

Layout mergedLayout(Layout base, const LayoutPart &part)
{
    mergeLayoutSizing(base, part);
    if (part.backgroundColor) {
        base.backgroundColor = *part.backgroundColor;
    }
    if (part.focusRing) {
        mergeBorder(base.focusRing, *part.focusRing);
    }
    if (part.border) {
        mergeBorder(base.border, *part.border);
    }
    if (part.shadow) {
        mergeShadow(base.shadow, *part.shadow);
    }
    if (part.tabIndicator) {
        mergeTabIndicator(base.tabIndicator, *part.tabIndicator);
    }
    if (part.insertHint) {
        mergeInsertHint(base.insertHint, *part.insertHint);
    }
    if (base.presetColumnWidths.isEmpty()) {
        base.presetColumnWidths = defaultPresets();
    }
    if (base.presetWindowHeights.isEmpty()) {
        base.presetWindowHeights = defaultPresets();
    }
    return base;
}

void mergeBorder(Border &base, const BorderRule &part)
{
    if (part.enabled) {
        base.enabled = *part.enabled;
    }
    if (part.width) {
        base.width = *part.width;
    }
    mergePaint(base.active, part.active);
    mergePaint(base.inactive, part.inactive);
    mergePaint(base.urgent, part.urgent);
}

void mergeShadow(Shadow &base, const ShadowRule &part)
{
    if (part.enabled) {
        base.enabled = *part.enabled;
    }
    if (part.offset) {
        base.offset = *part.offset;
    }
    if (part.softness) {
        base.softness = *part.softness;
    }
    if (part.spread) {
        base.spread = *part.spread;
    }
    if (part.drawBehindWindow) {
        base.drawBehindWindow = *part.drawBehindWindow;
    }
    if (part.color) {
        base.color = *part.color;
    }
    if (part.inactiveColor) {
        base.inactiveColor = *part.inactiveColor;
    }
}

}
