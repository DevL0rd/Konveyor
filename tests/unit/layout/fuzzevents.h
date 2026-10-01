#pragma once

#include "fuzzconfig.h"
#include "layout/engine/engineprivate.h"

#include <algorithm>

namespace LayoutTest
{

enum class Step
{
    AddWindow,
    RemoveWindow,
    Act,
    Reload,
    AddOutput,
    RemoveOutput,
    UpdateOutput,
    Advance,
    Fullscreen,
    FillWidth,
    Urgent,
    Activate,
    LayoutFocus,
    FloatingFrame,
    FocusOutput,
    UpdateProperties,
    Swipe,
    WorkspaceSwipe,
    Drag,
    Resize,
    DataDrag,
    FocusWorkspace,
};

struct Event
{
    Step step = Step::Act;
    quint32 a = 0;
    quint32 b = 0;
    quint32 c = 0;
    Config::Action action;
    qsizetype index = 0;
};

enum class Phase
{
    Begin,
    Update,
    End,
    Whole,
};

inline Phase gesturePhase(const Event &event)
{
    return static_cast<Phase>((event.c >> 24) % 4);
}

inline bool isGesture(Step step)
{
    return step == Step::Swipe || step == Step::WorkspaceSwipe || step == Step::Drag || step == Step::Resize || step == Step::DataDrag;
}

inline const QString WindowPlaceholder = QStringLiteral("#window");

inline QStringList actionNames()
{
    QStringList names = Layout::actionTable().keys();
    std::ranges::sort(names);
    return names;
}

inline QStringList outputNames()
{
    return {QStringLiteral("DP-1"), QStringLiteral("DP-2"), QStringLiteral("HDMI-A-1"), QStringLiteral("eDP-1"), QStringLiteral("DP-3"),
        QStringLiteral("DP-4")};
}

inline QString pickOf(Dice &dice, const QStringList &values)
{
    return values.at(static_cast<qsizetype>(dice.below(static_cast<quint32>(values.size()))));
}

inline QStringList actionArguments(Dice &dice, const QString &name)
{
    static const QStringList indexed {QStringLiteral("focus-column"), QStringLiteral("focus-window-in-column"),
        QStringLiteral("move-column-to-index"), QStringLiteral("move-workspace-to-index")};
    static const QStringList referenced {
        QStringLiteral("focus-workspace"), QStringLiteral("move-window-to-workspace"), QStringLiteral("move-column-to-workspace")};
    static const QStringList sized {
        QStringLiteral("set-column-width"), QStringLiteral("set-window-width"), QStringLiteral("set-window-height")};
    static const QStringList monitors {QStringLiteral("focus-monitor"), QStringLiteral("move-window-to-monitor"),
        QStringLiteral("move-column-to-monitor"), QStringLiteral("move-workspace-to-monitor")};
    if (name == QLatin1String("order-taskbar-columns")) {
        return {pickOf(dice, outputNames()), QStringLiteral("[[6],[5],[4],[3],[2],[1]]")};
    }
    if (indexed.contains(name)) {
        return {QString::number(dice.below(7))};
    }
    if (referenced.contains(name)) {
        return {
            pickOf(dice, {QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("web"), QStringLiteral("chat")})};
    }
    if (sized.contains(name)) {
        return {pickOf(dice,
            {QStringLiteral("+10%"), QStringLiteral("-10%"), QStringLiteral("25%"), QStringLiteral("100%"), QStringLiteral("+120"),
                QStringLiteral("-300"), QStringLiteral("800"), QStringLiteral("1")})};
    }
    if (monitors.contains(name)) {
        return {pickOf(dice, outputNames())};
    }
    if (name == QLatin1String("set-column-display")) {
        return {pickOf(dice, {QStringLiteral("normal"), QStringLiteral("tabbed")})};
    }
    if (name == QLatin1String("set-workspace-name")) {
        return {pickOf(dice, {QStringLiteral("web"), QStringLiteral("chat"), QStringLiteral("scratch")})};
    }
    if (name == QLatin1String("unset-workspace-name") && dice.chance(50)) {
        return {pickOf(dice, {QStringLiteral("web"), QStringLiteral("scratch"), QStringLiteral("2")})};
    }
    if (name.startsWith(QLatin1String("spawn"))) {
        return {QStringLiteral("true")};
    }
    return {};
}

inline QList<std::pair<QString, QString>> actionProperties(Dice &dice, const QString &name)
{
    QList<std::pair<QString, QString>> properties;
    if (name == QLatin1String("move-floating-window")) {
        properties.append({QStringLiteral("x"), pickOf(dice, {QStringLiteral("+40"), QStringLiteral("-3000"), QStringLiteral("10%")})});
        properties.append({QStringLiteral("y"), pickOf(dice, {QStringLiteral("-25"), QStringLiteral("+2000"), QStringLiteral("50%")})});
    }
    if (name.contains(QLatin1String("workspace")) && dice.chance(40)) {
        properties.append({QStringLiteral("focus"), dice.chance(50) ? QStringLiteral("true") : QStringLiteral("false")});
    }
    if (name.contains(QLatin1String("workspace-name")) && dice.chance(40)) {
        properties.append({QStringLiteral("workspace"), pickOf(dice, {QStringLiteral("1"), QStringLiteral("web")})});
    }
    if (name == QLatin1String("move-workspace-to-index") && dice.chance(40)) {
        properties.append({QStringLiteral("reference"), pickOf(dice, {QStringLiteral("2"), QStringLiteral("chat")})});
    }
    if (name.startsWith(QLatin1String("switch-preset-column-width")) && dice.chance(50)) {
        properties.append({QStringLiteral("from-native"), QStringLiteral("true")});
    }
    if (dice.chance(30) || name == QLatin1String("focus-window")) {
        properties.append({QStringLiteral("id"), WindowPlaceholder});
    }
    return properties;
}

inline Step randomStep(Dice &dice)
{
    static const QList<std::pair<Step, quint32>> weights {{Step::AddWindow, 16}, {Step::RemoveWindow, 6}, {Step::Act, 40},
        {Step::Reload, 4}, {Step::AddOutput, 2}, {Step::RemoveOutput, 2}, {Step::UpdateOutput, 2}, {Step::Advance, 6},
        {Step::Fullscreen, 2}, {Step::FillWidth, 1}, {Step::Urgent, 1}, {Step::Activate, 3}, {Step::LayoutFocus, 1},
        {Step::FloatingFrame, 2}, {Step::FocusOutput, 2}, {Step::UpdateProperties, 2}, {Step::Swipe, 3}, {Step::WorkspaceSwipe, 3},
        {Step::Drag, 4}, {Step::Resize, 3}, {Step::DataDrag, 2}, {Step::FocusWorkspace, 2}};
    quint32 total = 0;
    for (const auto &[step, weight] : weights) {
        total += weight;
    }
    quint32 roll = dice.below(total);
    for (const auto &[step, weight] : weights) {
        if (roll < weight) {
            return step;
        }
        roll -= weight;
    }
    return Step::Act;
}

inline QList<Event> generateEvents(quint32 seed, int count)
{
    Dice dice(seed);
    const QStringList names = actionNames();
    QList<Event> events;
    events.append(Event {Step::Reload, dice.below(1u << 30), 0, 0, {}});
    for (int i = 1; i < count; ++i) {
        Event event;
        event.step = randomStep(dice);
        event.a = dice.below(1u << 30);
        event.b = dice.below(1u << 30);
        event.c = dice.below(1u << 30);
        if (event.step == Step::Act) {
            const QString name = pickOf(dice, names);
            event.action = Config::Action {name, actionArguments(dice, name), actionProperties(dice, name)};
        }
        event.index = i;
        events.append(event);
    }
    return events;
}

inline QString describe(const Event &event)
{
    static const QStringList steps {QStringLiteral("add-window"), QStringLiteral("remove-window"), QStringLiteral("act"),
        QStringLiteral("reload"), QStringLiteral("add-output"), QStringLiteral("remove-output"), QStringLiteral("update-output"),
        QStringLiteral("advance"), QStringLiteral("fullscreen"), QStringLiteral("fill-width"), QStringLiteral("urgent"),
        QStringLiteral("activate"), QStringLiteral("layout-focus"), QStringLiteral("floating-frame"), QStringLiteral("focus-output"),
        QStringLiteral("update-properties"), QStringLiteral("swipe"), QStringLiteral("workspace-swipe"), QStringLiteral("drag"),
        QStringLiteral("resize"), QStringLiteral("data-drag"), QStringLiteral("focus-workspace")};
    QString text = QStringLiteral("#%1 %2 %3 %4 %5")
                       .arg(event.index)
                       .arg(steps.at(static_cast<qsizetype>(event.step)))
                       .arg(event.a)
                       .arg(event.b)
                       .arg(event.c);
    if (isGesture(event.step)) {
        static const QStringList phases {QStringLiteral("begin"), QStringLiteral("update"), QStringLiteral("end"), QStringLiteral("whole")};
        text += QLatin1Char(' ') + phases.at(static_cast<qsizetype>(gesturePhase(event)));
    }
    if (isGesture(event.step)) {
        static const QStringList phases {QStringLiteral("begin"), QStringLiteral("update"), QStringLiteral("end"), QStringLiteral("whole")};
        text += QLatin1Char(' ') + phases.at(static_cast<qsizetype>(gesturePhase(event)));
    }
    if (event.step == Step::Act) {
        text += QStringLiteral(" %1 %2").arg(event.action.name, event.action.arguments.join(QLatin1Char(' ')));
        for (const auto &[key, value] : event.action.properties) {
            text += QStringLiteral(" %1=%2").arg(key, value);
        }
    }
    return text;
}

}
