#pragma once

#include "helpers.h"

#include <QRandomGenerator>

#include <initializer_list>

namespace LayoutTest
{

class Dice
{
public:
    explicit Dice(quint32 seed)
        : m_random(seed)
    { }

    quint32 below(quint32 count) { return count == 0 ? 0 : m_random.bounded(count); }
    bool chance(quint32 percent) { return below(100) < percent; }
    double between(double low, double high) { return low + m_random.generateDouble() * (high - low); }

    template<typename T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(static_cast<quint32>(values.size()))); }

private:
    QRandomGenerator m_random;
};

inline QList<Config::PresetSize> randomPresets(Dice &dice, bool heights)
{
    switch (dice.below(heights ? 4 : 5)) {
    case 0:
        return {};
    case 1:
        return {Config::Proportion {1.0 / 3.0}, Config::Proportion {0.5}, Config::Proportion {2.0 / 3.0}};
    case 2:
        return {Config::Fixed {300}, Config::Fixed {600}, Config::Fixed {900}};
    case 3:
        return {Config::Proportion {0.25}, Config::Fixed {1500}, Config::Proportion {1.0}};
    default:
        return {Config::Proportion {0.5}};
    }
}

inline std::optional<Config::PresetSize> randomDefaultWidth(Dice &dice)
{
    switch (dice.below(5)) {
    case 0:
        return std::nullopt;
    case 1:
        return Config::Proportion {1.0 / 3.0};
    case 2:
        return Config::Fixed {700};
    case 3:
        return Config::Proportion {1.0};
    default:
        return Config::Proportion {0.5};
    }
}

inline Config::Struts randomStruts(Dice &dice)
{
    switch (dice.below(6)) {
    case 0:
        return {64, 64, 0, 0};
    case 1:
        return {0, 0, 32, 48};
    case 2:
        return {700, 700, 500, 500};
    case 3:
        return {12.5, 40, 7.25, 0};
    default:
        return {};
    }
}

inline Config::Border randomBorder(Dice &dice)
{
    Config::Border border;
    border.enabled = dice.chance(50);
    border.width = dice.pick({1.0, 4.0, 9.5});
    border.active.color = QColor(0x80, 0x40, 0xff);
    return border;
}

inline Config::TabIndicator randomTabIndicator(Dice &dice)
{
    Config::TabIndicator indicator;
    indicator.enabled = dice.chance(80);
    indicator.hideWhenSingleTab = dice.chance(50);
    indicator.placeWithinColumn = dice.chance(50);
    indicator.gap = dice.pick({0.0, 5.0, 12.0});
    indicator.width = dice.pick({2.0, 4.0, 10.0});
    indicator.lengthTotalProportion = dice.pick({0.25, 0.5, 1.0});
    indicator.position = dice.pick({Config::TabIndicatorPosition::Left, Config::TabIndicatorPosition::Right,
        Config::TabIndicatorPosition::Top, Config::TabIndicatorPosition::Bottom});
    indicator.gapsBetweenTabs = dice.pick({0.0, 3.0});
    indicator.cornerRadius = dice.pick({0.0, 6.0});
    return indicator;
}

inline Config::Layout randomLayout(Dice &dice)
{
    Config::Layout layout;
    layout.gaps = dice.pick({0.0, 8.0, 16.0, 33.5});
    layout.centerFocusedColumn
        = dice.pick({Config::CenterFocusedColumn::Never, Config::CenterFocusedColumn::Always, Config::CenterFocusedColumn::OnOverflow});
    layout.newColumnPosition = dice.pick({Config::NewColumnPosition::Left, Config::NewColumnPosition::Right});
    layout.alwaysCenterSingleColumn = dice.chance(50);
    layout.alwaysExpandSingleColumn = dice.chance(50);
    layout.emptyWorkspaceAboveFirst = dice.chance(40);
    layout.defaultColumnDisplay = dice.chance(25) ? Config::ColumnDisplay::Tabbed : Config::ColumnDisplay::Normal;
    layout.presetColumnWidths = randomPresets(dice, false);
    layout.defaultColumnWidth = randomDefaultWidth(dice);
    layout.rememberWindowSizes = dice.chance(40);
    layout.rememberWindowPositions = dice.chance(40);
    layout.groupAppWindows = dice.pick({Config::GroupAppWindows::Off, Config::GroupAppWindows::Beside, Config::GroupAppWindows::Stack});
    layout.maxRowsPerColumn = dice.pick({1, 2, 3, 5});
    layout.newWindowPlacement = dice.pick({Config::NewWindowPlacement::Column, Config::NewWindowPlacement::Stack});
    layout.floatChildWindows = dice.chance(50);
    layout.presetWindowHeights = randomPresets(dice, true);
    layout.struts = randomStruts(dice);
    layout.focusRing = randomBorder(dice);
    layout.border = randomBorder(dice);
    layout.tabIndicator = randomTabIndicator(dice);
    layout.insertHint.enabled = dice.chance(70);
    return layout;
}

inline Config::Animations randomAnimations(Dice &dice, bool enabled)
{
    Config::Animations animations = linearAnimationConfig(dice.pick({50.0, 150.0, 400.0})).animations;
    animations.enabled = enabled;
    animations.slowdown = dice.pick({1.0, 0.5, 3.0});
    if (dice.chance(50)) {
        animations.windowMovement.kind = Config::SpringParams {dice.pick({0.6, 1.0}), 800, 0.0001};
        animations.horizontalViewMovement.kind = Config::SpringParams {1.0, dice.pick({300.0, 1000.0}), 0.0001};
    }
    return animations;
}

inline QList<Config::WindowRule> randomRules(Dice &dice)
{
    QList<Config::WindowRule> rules;
    Config::WindowRule game = ruleFor(QStringLiteral("game"));
    game.forceResizable = dice.chance(50);
    game.openFullscreen = dice.chance(30);
    rules.append(game);
    Config::WindowRule dialog = ruleFor(QStringLiteral("dialog"));
    dialog.openFloating = true;
    dialog.defaultFloatingPosition = Config::FloatingPosition {dice.between(-50, 400), 20, Config::FloatingRelativeTo::BottomRight};
    rules.append(dialog);
    Config::WindowRule term = ruleFor(QStringLiteral("term"));
    term.minWidth = dice.pick({0, 400, 2500});
    term.maxHeight = dice.pick({0, 300});
    term.columnPosition = dice.chance(30) ? std::optional(Config::ColumnPosition::End) : std::nullopt;
    term.openOnWorkspace = dice.chance(30) ? std::optional(QStringLiteral("chat")) : std::nullopt;
    rules.append(term);
    Config::WindowRule browser = ruleFor(QStringLiteral("browser"));
    browser.openMaximized = dice.chance(30);
    browser.openMaximizedToEdges = dice.chance(20);
    browser.defaultColumnDisplay = dice.chance(30) ? std::optional(Config::ColumnDisplay::Tabbed) : std::nullopt;
    browser.openOnOutput = dice.chance(30) ? std::optional(QStringLiteral("DP-2")) : std::nullopt;
    rules.append(browser);
    return rules;
}

inline Config::Config randomConfig(quint32 seed, bool animated)
{
    Dice dice(seed);
    Config::Config config;
    config.layout = randomLayout(dice);
    config.animations = randomAnimations(dice, animated);
    config.windowRules = randomRules(dice);
    config.input.workspaceAutoBackAndForth = dice.chance(50);
    config.gestures.dndEdgeViewScroll
        = Config::DndEdgeScroll {dice.pick({1.0, 30.0, 400.0}), dice.pick({0.0, 100.0}), dice.pick({1.0, 1500.0, 20000.0})};
    config.gestures.dndEdgeWorkspaceSwitch = Config::DndEdgeScroll {dice.pick({1.0, 50.0}), dice.pick({0.0, 100.0}), 1500.0};
    if (dice.chance(60)) {
        Config::NamedWorkspace web {QStringLiteral("web"), std::nullopt, std::nullopt, std::nullopt};
        Config::NamedWorkspace chat {QStringLiteral("chat"), QStringLiteral("DP-2"), std::nullopt, std::nullopt};
        if (dice.chance(50)) {
            chat.layout = randomLayout(dice);
        }
        config.workspaces = {web, chat};
    }
    if (dice.chance(40)) {
        config.outputs.append(Config::OutputConfig {QStringLiteral("HDMI-A-1"), randomLayout(dice), std::nullopt, std::nullopt});
    }
    if (dice.chance(40)) {
        Config::MonitorProfile portrait;
        portrait.name = QStringLiteral("small");
        Config::MonitorMatch match;
        match.widthBelow = 1600;
        portrait.matches.append(match);
        portrait.layout = randomLayout(dice);
        config.monitorProfiles.append(portrait);
    }
    return config;
}

}
