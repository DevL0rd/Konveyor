#pragma once

#include <QString>

#include <array>
#include <cstddef>
#include <optional>

namespace Konveyor::Config
{

enum class EventTrigger
{
    Focus,
    Unfocus,
    Click,
    Scroll,
    Keypress,
    Urgent,
    Beat,
    View,
    Count
};

inline constexpr std::size_t kEventTriggerCount = static_cast<std::size_t>(EventTrigger::Count);

enum class EventReach
{
    Self,
    Neighbors,
    All
};

enum class EventMode
{
    Once,
    While
};

struct EventEffect
{
    QString style;
    EventReach reach = EventReach::Self;
    EventMode mode = EventMode::Once;
    std::optional<double> duration;
    bool operator==(const EventEffect &) const = default;
};

struct EventEffects
{
    std::array<std::optional<EventEffect>, kEventTriggerCount> slots_;

    const std::optional<EventEffect> &of(EventTrigger trigger) const { return slots_[static_cast<std::size_t>(trigger)]; }
    std::optional<EventEffect> &of(EventTrigger trigger) { return slots_[static_cast<std::size_t>(trigger)]; }
    bool operator==(const EventEffects &) const = default;
};

}
