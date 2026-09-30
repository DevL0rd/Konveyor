#pragma once

#include "helpers.h"

#include <algorithm>

Q_DECLARE_METATYPE(Konveyor::Config::TabIndicatorPosition)

namespace LayoutTest
{

inline Config::Config tabbedConfig(Config::TabIndicatorPosition position, double width, double gap)
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
    config.layout.tabIndicator.position = position;
    config.layout.tabIndicator.width = width;
    config.layout.tabIndicator.gap = gap;
    return config;
}

inline QList<Layout::WindowId> addTabs(Fixture &fixture, int count)
{
    QList<Layout::WindowId> ids {fixture.add(QStringLiteral("tab"))};
    for (int i = 1; i < count; ++i) {
        ids.append(fixture.add(QStringLiteral("tab")));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
    }
    return ids;
}

inline Layout::WindowId addPlainColumn(Fixture &fixture, const QString &appId)
{
    const Layout::WindowId id = fixture.add(appId);
    fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
    return id;
}

inline int topmostTab(Fixture &fixture, const QList<Layout::WindowId> &tabs)
{
    int top = 0;
    for (int i = 1; i < tabs.size(); ++i) {
        if (fixture.state(tabs[i]).stackingIndex > fixture.state(tabs[top]).stackingIndex) {
            top = i;
        }
    }
    return top;
}

inline double distance(const QRectF &a, const QRectF &b)
{
    const double dx = std::max({0.0, b.left() - a.right(), a.left() - b.right()});
    const double dy = std::max({0.0, b.top() - a.bottom(), a.top() - b.bottom()});
    return std::max(dx, dy);
}

inline bool beside(const QRectF &a, const QRectF &b, Config::TabIndicatorPosition position)
{
    const bool vertical = position == Config::TabIndicatorPosition::Left || position == Config::TabIndicatorPosition::Right;
    return vertical ? a.top() < b.bottom() && b.top() < a.bottom() : a.left() < b.right() && b.left() < a.right();
}

inline void verifyShownTab(Fixture &fixture, const QList<Layout::WindowId> &tabs, int shown)
{
    QCOMPARE(topmostTab(fixture, tabs), shown);
    for (int i = 0; i < tabs.size(); ++i) {
        QCOMPARE(fixture.state(tabs[i]).visible, i == shown);
        if (i != shown) {
            QVERIFY(!fixture.state(tabs[i]).tabBar.visible);
        }
    }
}

}
