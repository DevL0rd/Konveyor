#pragma once

#include "config/types.h"

#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <cstddef>
#include <optional>

namespace Konveyor::Layout
{

enum class ScrollDirection
{
    Left,
    Right
};

enum class Activation
{
    Always,
    Smart,
    Never
};

enum class ChangeKind
{
    SetFixed,
    SetProportion,
    AdjustFixed,
    AdjustProportion
};

struct PositionChange
{
    ChangeKind kind = ChangeKind::SetFixed;
    double value = 0;
    bool operator==(const PositionChange &) const = default;
};

struct SizeChange
{
    ChangeKind kind = ChangeKind::SetFixed;
    double value = 0;
    bool operator==(const SizeChange &) const = default;
};

struct ColumnWidth
{
    bool isProportion = true;
    double value = 0.5;
    static ColumnWidth proportion(double value) { return {true, value}; }
    static ColumnWidth fixed(double value) { return {false, value}; }
    static ColumnWidth fromPreset(const Config::PresetSize &preset);
    bool operator==(const ColumnWidth &) const = default;
};

struct WindowHeight
{
    enum class Kind
    {
        Auto,
        Fixed,
        Preset
    };
    Kind kind = Kind::Auto;
    double value = 1.0;
    std::size_t preset = 0;
    static WindowHeight autoWeight(double weight) { return {Kind::Auto, weight, 0}; }
    static WindowHeight fixed(double height) { return {Kind::Fixed, height, 0}; }
    static WindowHeight presetIndex(std::size_t index) { return {Kind::Preset, 0, index}; }
    bool isAuto() const { return kind == Kind::Auto; }
    bool operator==(const WindowHeight &) const = default;
};

struct PresetExtent
{
    bool isTile = true;
    double value = 0;
};

struct DropSlot
{
    enum class Kind
    {
        NewColumn,
        InColumn,
        Floating
    };
    Kind kind = Kind::NewColumn;
    std::size_t column = 0;
    std::size_t tile = 0;
    bool operator==(const DropSlot &) const = default;
};

struct DropWorkspace
{
    bool existing = true;
    quint64 id = 0;
    std::size_t newAt = 0;
    bool operator==(const DropWorkspace &) const = default;
};

bool resolveActivation(Activation activate, bool smart);

SizeChange sizeChangeFromPreset(const Config::PresetSize &preset);

}
