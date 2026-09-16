#pragma once

#include <QList>
#include <QString>

#include <optional>
#include <variant>

namespace Konveyor::Config
{

struct Proportion
{
    double value = 0.5;
    bool operator==(const Proportion &) const = default;
};

struct Fixed
{
    double value = 0;
    bool operator==(const Fixed &) const = default;
};

using PresetSize = std::variant<Proportion, Fixed>;

enum class CenterFocusedColumn
{
    Never,
    Always,
    OnOverflow
};

enum class NewColumnPosition
{
    Left,
    Right
};

enum class ColumnPosition
{
    Start,
    End
};

enum class ColumnDisplay
{
    Normal,
    Tabbed
};

struct Struts
{
    double left = 0;
    double right = 0;
    double top = 0;
    double bottom = 0;
    bool operator==(const Struts &) const = default;
};

}
