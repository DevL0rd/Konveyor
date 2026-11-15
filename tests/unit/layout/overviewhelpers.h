#pragma once

#include "helpers.h"

#include <optional>

namespace LayoutTest
{

inline Config::Config overviewConfig(bool animated = false)
{
    Config::Config config = animated ? linearAnimationConfig(200) : instantConfig();
    config.overview.enabled = true;
    return config;
}

inline const Layout::OverviewView *viewFor(const QList<Layout::OverviewView> &views, const QString &output)
{
    for (const Layout::OverviewView &view : views) {
        if (view.output == output) {
            return &view;
        }
    }
    return nullptr;
}

inline std::optional<Layout::OverviewWindow> windowFor(const Layout::OverviewView &view, Layout::WindowId id)
{
    for (const Layout::OverviewWindow &window : view.windows) {
        if (window.id == id) {
            return window;
        }
    }
    return std::nullopt;
}

struct Scene
{
    Fixture fixture {overviewConfig()};
    Layout::WindowId a = fixture.add(QStringLiteral("a"));
    Layout::WindowId b = fixture.add(QStringLiteral("b"));
    Layout::WindowId c = addBelow(QStringLiteral("c"));

    Layout::WindowId addBelow(const QString &appId)
    {
        fixture.perform(QStringLiteral("focus-workspace-down"));
        return fixture.add(appId);
    }

    void open()
    {
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.engine().setOverviewOpen(true);
    }

    Layout::OverviewView openView()
    {
        open();
        return fixture.engine().overviewViews().first();
    }

    QPointF grab(Layout::WindowId id, const Layout::OverviewView &view)
    {
        const auto window = windowFor(view, id);
        Q_ASSERT(window);
        fixture.engine().overviewBeginWindowDrag(id, QStringLiteral("DP-1"), window->rect.center());
        return window->rect.center();
    }

    void grabCard(int index, const Layout::OverviewView &view)
    {
        fixture.engine().overviewBeginWorkspaceDrag(QStringLiteral("DP-1"), index);
        fixture.engine().overviewUpdateDrag(betweenCards(view, 0, 1));
    }

    void finish()
    {
        fixture.engine().overviewEndDrag();
        fixture.settle();
    }

    void cancel()
    {
        fixture.engine().overviewCancelDrag();
        fixture.settle();
    }

    static QPointF betweenCards(const Layout::OverviewView &view, int first, int second)
    {
        const QPointF top = view.workspaceRects[first].center();
        const QPointF bottom = view.workspaceRects[second].center();
        return QPointF(top.x(), (top.y() + bottom.y()) / 2.0);
    }
};

struct FocusedPair
{
    Fixture fixture {overviewConfig()};
    Layout::WindowId a = fixture.add(QStringLiteral("a"));
    Layout::WindowId b = fixture.add(QStringLiteral("b"));
};

struct TwoOutputs
{
    explicit TwoOutputs(const Config::Config &config = overviewConfig())
        : fixture(config)
    {
        fixture.addOutput(QStringLiteral("HDMI-A-1"), QRectF(1920, 0, 1280, 720));
    }

    QList<Layout::OverviewView> openOneOnEach()
    {
        fixture.add(QStringLiteral("a"));
        fixture.addClient(makeWindow(QStringLiteral("b")), Client(), Layout::ActivationPolicy::Focus, QStringLiteral("HDMI-A-1"));
        fixture.engine().setOverviewOpen(true);
        return fixture.engine().overviewViews();
    }

    Fixture fixture;
};

}
