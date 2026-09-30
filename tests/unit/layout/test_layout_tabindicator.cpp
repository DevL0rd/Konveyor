#include "helpers.h"

using namespace LayoutTest;

Q_DECLARE_METATYPE(Config::TabIndicatorPosition)

namespace
{

Config::Config tabbedConfig(Config::TabIndicatorPosition position, double width, double gap)
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
    config.layout.tabIndicator.position = position;
    config.layout.tabIndicator.width = width;
    config.layout.tabIndicator.gap = gap;
    return config;
}

QList<Layout::WindowId> addTabs(Fixture &fixture, int count)
{
    QList<Layout::WindowId> ids {fixture.add(QStringLiteral("tab"))};
    for (int i = 1; i < count; ++i) {
        ids.append(fixture.add(QStringLiteral("tab")));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
    }
    return ids;
}

int topmostTab(Fixture &fixture, const QList<Layout::WindowId> &tabs)
{
    int top = 0;
    for (int i = 1; i < tabs.size(); ++i) {
        if (fixture.state(tabs[i]).stackingIndex > fixture.state(tabs[top]).stackingIndex) {
            top = i;
        }
    }
    return top;
}

}

class TestLayoutTabIndicator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void theShownTabIsStackedAboveTheOtherTabs()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        QCOMPARE(topmostTab(fixture, tabs), 2);
        fixture.perform(QStringLiteral("focus-window-up"));
        QCOMPARE(topmostTab(fixture, tabs), 1);
        fixture.perform(QStringLiteral("focus-window-top"));
        QCOMPARE(topmostTab(fixture, tabs), 0);
        QVERIFY(fixture.state(tabs[0]).visible);
        fixture.perform(QStringLiteral("focus-window-bottom"));
        QCOMPARE(topmostTab(fixture, tabs), 2);
    }

    void clickingATabRaisesIt()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.advance(1);
        const QList<QRectF> rects = fixture.state(tabs[2]).tabBar.tabRects;
        const auto clicked = fixture.engine().windowAt(rects[0].center());
        QCOMPARE(clicked, std::optional(tabs[0]));
        fixture.engine().activateWindow(*clicked);
        fixture.settle();
        QCOMPARE(topmostTab(fixture, tabs), 0);
    }
};

QTEST_GUILESS_MAIN(TestLayoutTabIndicator)
#include "test_layout_tabindicator.moc"
