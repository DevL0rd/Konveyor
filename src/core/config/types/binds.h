#pragma once

#include "config/types/windowrules.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace Konveyor::Config
{

enum class SizeChangeKind
{
    SetFixed,
    SetProportion,
    AdjustFixed,
    AdjustProportion
};

struct SizeChange
{
    SizeChangeKind kind = SizeChangeKind::SetProportion;
    double value = 0;
    bool operator==(const SizeChange &) const = default;
};

enum class WorkspaceReferenceKind
{
    Index,
    Name,
    Id
};

struct WorkspaceReference
{
    WorkspaceReferenceKind kind = WorkspaceReferenceKind::Index;
    quint64 index = 1;
    QString name;
    bool operator==(const WorkspaceReference &) const = default;
};

struct Action
{
    QString name;
    QStringList arguments;
    QList<std::pair<QString, QString>> properties;
    bool operator==(const Action &) const = default;
};

enum class BindTrigger
{
    Key,
    MouseButton,
    Wheel,
    TouchpadScroll
};

enum class BindModifier : quint8
{
    Ctrl = 0x01,
    Shift = 0x02,
    Alt = 0x04,
    Super = 0x08,
    IsoLevel3Shift = 0x10,
    IsoLevel5Shift = 0x20,
    Mod = 0x40,
};
Q_DECLARE_FLAGS(BindModifiers, BindModifier)

enum class MouseButton
{
    Left,
    Right,
    Middle,
    Back,
    Forward
};

enum class ScrollDirection
{
    Down,
    Up,
    Left,
    Right
};

struct Bind
{
    QString keyText;
    Qt::KeyboardModifiers modifiers;
    BindTrigger trigger = BindTrigger::Key;
    int key = 0;
    Action action;
    bool repeat = true;
    std::optional<int> cooldownMs;
    std::optional<QString> hotkeyOverlayTitle;
    bool hideFromHotkeyOverlay = false;
    quint32 keysym = 0;
    MouseButton mouseButton = MouseButton::Left;
    ScrollDirection scrollDirection = ScrollDirection::Down;
    BindModifiers keyModifiers;
    BindModifiers resolvedModifiers;
    bool operator==(const Bind &) const = default;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Konveyor::Config::BindModifiers)
