#include "config/loader.h"
#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

const QStringList &triggerNames()
{
    static const QStringList names {QStringLiteral("focus"), QStringLiteral("unfocus"), QStringLiteral("click"), QStringLiteral("scroll"),
        QStringLiteral("keypress"), QStringLiteral("urgent"), QStringLiteral("beat"), QStringLiteral("view")};
    return names;
}

const QStringList &reachNames()
{
    static const QStringList names {QStringLiteral("self"), QStringLiteral("neighbors"), QStringLiteral("all")};
    return names;
}

const QStringList &modeNames()
{
    static const QStringList names {QStringLiteral("once"), QStringLiteral("while")};
    return names;
}

EventMode defaultMode(EventTrigger trigger)
{
    return trigger == EventTrigger::Scroll || trigger == EventTrigger::View ? EventMode::While : EventMode::Once;
}

void decodeOn(const Kdl::Node &node, EventEffects &effects)
{
    expectNoChildren(node);
    expectArgumentLimit(node, 2);
    requiredArgument(node, QStringLiteral("event"));
    const auto trigger = static_cast<EventTrigger>(toKeyword(node.arguments.first(), triggerNames()));
    if (node.arguments.size() < 2) {
        fail(node, QStringLiteral("an event needs a border style name or `none`"));
    }
    EventEffect effect;
    effect.style = toText(node.arguments.at(1));
    effect.mode = defaultMode(trigger);
    ValueTable table;
    table.insert(QStringLiteral("reach"),
        [&effect](const Kdl::Value &value) { effect.reach = static_cast<EventReach>(toKeyword(value, reachNames())); });
    table.insert(QStringLiteral("mode"),
        [&effect](const Kdl::Value &value) { effect.mode = static_cast<EventMode>(toKeyword(value, modeNames())); });
    table.insert(QStringLiteral("duration"), [&effect](const Kdl::Value &value) { effect.duration = toPositiveNumber(value, 600); });
    decodeProperties(node, table);
    effects.of(trigger) = effect;
}

}

EventEffects decodeEventEffects(const Kdl::Node &node)
{
    expectNoProperties(node);
    expectNoArguments(node);
    EventEffects effects;
    NodeTable table;
    table.insert(QStringLiteral("on"), [&effects](const Kdl::Node &child) { decodeOn(child, effects); });
    decodeChildren(node, table, {QStringLiteral("on")});
    return effects;
}

void mergeEventEffects(EventEffects &base, const EventEffects &part)
{
    for (std::size_t index = 0; index < kEventTriggerCount; ++index) {
        if (part.slots_[index]) {
            base.slots_[index] = part.slots_[index];
        }
    }
}

}
