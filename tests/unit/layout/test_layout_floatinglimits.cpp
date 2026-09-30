#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config floatingConfig(std::optional<int> maxWidth = std::nullopt)
{
    Config::Config config = instantConfig();
    Config::WindowRule rule = ruleFor(QStringLiteral("float"));
    rule.openFloating = true;
    rule.maxWidth = maxWidth;
    config.windowRules.append(rule);
    return config;
}

}

class TestLayoutFloatingLimits : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void reloadedMaximumShrinksAFloatingWindow()
    {
        Fixture fixture(floatingConfig());
        const auto id = fixture.add(QStringLiteral("float"), QSizeF(900, 500));
        QCOMPARE(fixture.frame(id).size(), QSizeF(900, 500));
        fixture.setConfig(floatingConfig(600));
        fixture.settle();
        QCOMPARE(fixture.frame(id).size(), QSizeF(600, 500));
        VERIFY_INVARIANTS(fixture);
    }

    void largerMinimumHintGrowsAFloatingWindow()
    {
        Fixture fixture(floatingConfig());
        Layout::WindowProperties properties = makeWindow(QStringLiteral("float"), QStringLiteral("float"), QSizeF(400, 300));
        const auto id = fixture.addWith(properties);
        properties.minSize = QSizeF(800, 600);
        fixture.engine().updateWindowProperties(id, properties);
        fixture.settle();
        QCOMPARE(fixture.frame(id).size(), QSizeF(800, 600));
        VERIFY_INVARIANTS(fixture);
    }

    void maximizedColumnFloatsWithinItsMaximum()
    {
        Fixture fixture;
        Layout::WindowProperties properties = makeWindow(QStringLiteral("narrow"));
        properties.maxSize = QSizeF(600, 0);
        const auto id = fixture.addWith(properties);
        QVERIFY(fixture.perform(QStringLiteral("maximize-column")).ok);
        QVERIFY(fixture.frame(id).width() > 600.0);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QVERIFY(fixture.state(id).isFloating);
        QCOMPARE(fixture.frame(id).width(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }

    void resizeThatEndsWithoutAResizeStillHonoursANewMaximum()
    {
        Fixture fixture(floatingConfig());
        const auto id = fixture.add(QStringLiteral("float"), QSizeF(900, 500));
        QVERIFY(fixture.engine().beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Right)));
        fixture.engine().endResize();
        fixture.settle();
        fixture.setConfig(floatingConfig(600));
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFloatingLimits)
#include "test_layout_floatinglimits.moc"
