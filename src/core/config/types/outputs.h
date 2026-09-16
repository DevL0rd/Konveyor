#pragma once

#include "config/types/input.h"

#include <QList>
#include <QRegularExpression>
#include <QString>

#include <optional>

namespace Konveyor::Config
{

struct OutputConfig
{
    QString name;
    std::optional<Layout> layout;
    std::optional<HotCorners> hotCorners;
    std::optional<LayoutPart> layoutPart;
    bool operator==(const OutputConfig &) const = default;
};

struct MonitorMatch
{
    std::optional<QRegularExpression> name;
    std::optional<double> aspectRatioAbove;
    std::optional<double> aspectRatioBelow;
    std::optional<double> widthAbove;
    std::optional<double> widthBelow;
    std::optional<double> heightAbove;
    std::optional<double> heightBelow;
    bool operator==(const MonitorMatch &) const = default;
};

struct MonitorProfile
{
    QString name;
    QList<MonitorMatch> matches;
    std::optional<Layout> layout;
    std::optional<LayoutPart> layoutPart;
    bool operator==(const MonitorProfile &) const = default;
};

struct NamedWorkspace
{
    QString name;
    std::optional<QString> openOnOutput;
    std::optional<Layout> layout;
    std::optional<LayoutPart> layoutPart;
    bool operator==(const NamedWorkspace &) const = default;
};

}
