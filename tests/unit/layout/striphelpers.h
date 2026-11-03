#pragma once

#include "helpers.h"
#include "layout/floating/floatinglayer.h"
#include "layout/monitor/monitor.h"
#include "layout/strip/columnstrip.h"
#include "layout/workspace/workspace.h"

namespace LayoutTest
{

struct StripKit
{
    Clock clock = Clock::frozenAt(Duration::zero());
    Layout::OptionsPtr options;
    QSizeF viewSize;
    double scaleFactor;
    Layout::ColumnStrip strip;
    Layout::WindowId nextId = 0;

    explicit StripKit(const Config::Config &config = instantConfig(), QSizeF view = QSizeF(1920, 1080),
        QRectF parent = QRectF(0, 0, 1920, 1080), double scale = 1.0)
        : StripKit(Layout::makeOptions(Layout::optionsFromConfig(config)), view, parent, scale)
    { }

    StripKit(Layout::OptionsPtr prepared, QSizeF view, QRectF parent, double scale)
        : options(std::move(prepared))
        , viewSize(view)
        , scaleFactor(scale)
        , strip(view, parent, scale, clock, options)
    { }

    Layout::Tile makeTile(const Layout::WindowProperties &properties = makeWindow(), std::optional<QSize> nativeSize = std::nullopt)
    {
        return Layout::Tile(Layout::LayoutWindow(++nextId, properties, nativeSize), viewSize, scaleFactor, clock, options);
    }

    Layout::WindowId addColumn(
        const Layout::WindowProperties &properties = makeWindow(), double width = 0.5, std::optional<QSize> nativeSize = std::nullopt)
    {
        Layout::Tile tile = makeTile(properties, nativeSize);
        const Layout::WindowId id = tile.id();
        strip.addTile(std::nullopt, std::move(tile), true, Layout::ColumnWidth::proportion(width), false, std::nullopt);
        return id;
    }

    Layout::WindowId addPinned(Config::ColumnPosition position, bool activate)
    {
        Layout::Tile tile = makeTile();
        const Layout::WindowId id = tile.id();
        Layout::EffectiveWindowRules rules;
        rules.columnPosition = position;
        tile.window().setRules(rules);
        strip.addTile(std::nullopt, std::move(tile), activate, Layout::ColumnWidth::proportion(0.5), false, std::nullopt);
        return id;
    }

    Layout::WindowId addToColumn(std::size_t column, const Layout::WindowProperties &properties = makeWindow())
    {
        Layout::Tile tile = makeTile(properties);
        const Layout::WindowId id = tile.id();
        strip.insertIntoColumn(column, std::nullopt, std::move(tile), true);
        return id;
    }
};

struct FloatKit
{
    Clock clock = Clock::frozenAt(Duration::zero());
    Layout::OptionsPtr options;
    QRectF area;
    Layout::FloatingLayer layer;
    Layout::WindowId nextId = 0;

    explicit FloatKit(const Config::Config &config = instantConfig(), QRectF workingArea = QRectF(0, 0, 1920, 1080))
        : FloatKit(Layout::makeOptions(Layout::optionsFromConfig(config)), workingArea)
    { }

    explicit FloatKit(Layout::OptionsPtr prepared, QRectF workingArea)
        : options(std::move(prepared))
        , area(workingArea)
        , layer(area.size(), area, 1.0, clock, options)
    { }

    Layout::Tile makeTile(const Layout::WindowProperties &properties = makeWindow(), std::optional<QSize> nativeSize = std::nullopt)
    {
        return Layout::Tile(Layout::LayoutWindow(++nextId, properties, nativeSize), area.size(), 1.0, clock, options);
    }

    Layout::WindowId add(
        const Layout::WindowProperties &properties = makeWindow(), bool activate = true, std::optional<QSize> nativeSize = std::nullopt)
    {
        Layout::Tile tile = makeTile(properties, nativeSize);
        const Layout::WindowId id = tile.id();
        layer.addTile(std::move(tile), activate);
        return id;
    }
};

struct MonitorKit
{
    Clock clock = Clock::frozenAt(Duration::zero());
    Layout::OptionsPtr options;
    Layout::OutputArea area;
    Layout::Monitor monitor;
    Layout::WindowId nextId = 0;

    explicit MonitorKit(const Config::Config &config = instantConfig())
        : options(Layout::makeOptions(Layout::optionsFromConfig(config)))
        , area(makeArea())
        , monitor(area, {}, std::nullopt, clock, options, std::nullopt)
    { }

    static Layout::OutputArea makeArea()
    {
        Layout::OutputArea result;
        result.outputName = QStringLiteral("DP-1");
        result.outputId = QStringLiteral("DP-1");
        return result;
    }

    Layout::Tile makeTile(const Layout::WindowProperties &properties = makeWindow())
    {
        return Layout::Tile(Layout::LayoutWindow(++nextId, properties), area.viewSize, area.scale, clock, options);
    }

    Layout::WindowId add(std::size_t workspace, const Layout::WindowProperties &properties = makeWindow(), bool activate = true)
    {
        Layout::Tile tile = makeTile(properties);
        const Layout::WindowId id = tile.id();
        Layout::MonitorAddRequest request;
        request.target = Layout::MonitorAddTarget::onWorkspace(monitor.workspaces().at(workspace).id());
        request.activate = activate ? Layout::Activation::Always : Layout::Activation::Never;
        request.width = Layout::ColumnWidth::proportion(0.5);
        monitor.addTile(std::move(tile), request);
        return id;
    }
};

inline Layout::OptionsPtr optionsWithoutPresets()
{
    Layout::Options options = Layout::optionsFromConfig(instantConfig());
    options.layout.presetColumnWidths.clear();
    return Layout::makeOptions(options);
}

}
