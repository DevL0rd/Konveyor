#pragma once

#include "config/types/sizing.h"

#include <QColor>
#include <QList>
#include <QPointF>
#include <QString>

#include <optional>

namespace Konveyor::Config
{

enum class GradientRelativeTo
{
    Window,
    WorkspaceView
};

enum class GradientInterpolation
{
    Srgb,
    SrgbLinear,
    Oklab,
    Oklch
};

enum class HueInterpolation
{
    Shorter,
    Longer,
    Increasing,
    Decreasing
};

struct Gradient
{
    QColor from;
    QColor to;
    double angle = 180;
    GradientRelativeTo relativeTo = GradientRelativeTo::Window;
    GradientInterpolation interpolation = GradientInterpolation::Srgb;
    HueInterpolation hue = HueInterpolation::Shorter;
    bool operator==(const Gradient &) const = default;
};

enum class ColorSource
{
    Explicit,
    SystemAccent,
    SystemFocus,
    SystemHover,
    SystemWindow,
    SystemWindowText,
    SystemInactiveText
};

struct Paint
{
    ColorSource source = ColorSource::Explicit;
    QColor color;
    std::optional<Gradient> gradient;
    bool operator==(const Paint &) const = default;
};

struct Border
{
    bool enabled = false;
    double width = 4;
    Paint active;
    Paint inactive;
    Paint urgent;
    bool operator==(const Border &) const = default;
};

struct BorderRule
{
    std::optional<bool> enabled;
    std::optional<double> width;
    std::optional<Paint> active;
    std::optional<Paint> inactive;
    std::optional<Paint> urgent;
    bool operator==(const BorderRule &) const = default;
};

struct Shadow
{
    bool enabled = false;
    double softness = 30;
    double spread = 5;
    QPointF offset {0, 5};
    bool drawBehindWindow = false;
    QColor color {0, 0, 0, 0x77};
    std::optional<QColor> inactiveColor;
    bool operator==(const Shadow &) const = default;
};

enum class TabIndicatorPosition
{
    Left,
    Right,
    Top,
    Bottom
};

struct ShadowRule
{
    std::optional<bool> enabled;
    std::optional<QPointF> offset;
    std::optional<double> softness;
    std::optional<double> spread;
    std::optional<bool> drawBehindWindow;
    std::optional<QColor> color;
    std::optional<QColor> inactiveColor;
    bool operator==(const ShadowRule &) const = default;
};

struct TabIndicatorRule
{
    std::optional<Paint> active;
    std::optional<Paint> inactive;
    std::optional<Paint> urgent;
    bool operator==(const TabIndicatorRule &) const = default;
};

struct TabIndicator
{
    bool enabled = true;
    bool hideWhenSingleTab = false;
    bool placeWithinColumn = false;
    double gap = 5;
    double width = 4;
    double lengthTotalProportion = 0.5;
    TabIndicatorPosition position = TabIndicatorPosition::Left;
    double gapsBetweenTabs = 0;
    double cornerRadius = 0;
    std::optional<Paint> active;
    std::optional<Paint> inactive;
    std::optional<Paint> urgent;
    bool operator==(const TabIndicator &) const = default;
};

struct InsertHint
{
    bool enabled = true;
    Paint paint;
    bool operator==(const InsertHint &) const = default;
};

struct TabIndicatorPart
{
    std::optional<bool> enabled;
    std::optional<bool> hideWhenSingleTab;
    std::optional<bool> placeWithinColumn;
    std::optional<double> gap;
    std::optional<double> width;
    std::optional<double> lengthTotalProportion;
    std::optional<TabIndicatorPosition> position;
    std::optional<double> gapsBetweenTabs;
    std::optional<double> cornerRadius;
    TabIndicatorRule colors;
    bool operator==(const TabIndicatorPart &) const = default;
};

struct InsertHintPart
{
    std::optional<bool> enabled;
    std::optional<Paint> paint;
    bool operator==(const InsertHintPart &) const = default;
};

}
