#pragma once

#include "helpers.h"

namespace LayoutTest
{

inline Config::Config forceResizableConfig()
{
    Config::Config config = instantConfig();
    Config::WindowRule rule = ruleFor(QStringLiteral("game"));
    rule.forceResizable = true;
    config.windowRules.append(rule);
    return config;
}

inline Layout::WindowProperties nonResizableGame()
{
    Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
    properties.isResizable = false;
    return properties;
}

}
