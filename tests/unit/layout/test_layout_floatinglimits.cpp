#include "helpers.h"

using namespace LayoutTest;

Q_DECLARE_METATYPE(Konveyor::Config::FloatingRelativeTo)

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

    void defaultFloatingPositionRule_data()
    {
        QTest::addColumn<Config::FloatingRelativeTo>("relativeTo");
        QTest::addColumn<QPointF>("topLeft");
        QTest::newRow("top-left") << Config::FloatingRelativeTo::TopLeft << QPointF(10, 20);
        QTest::newRow("top-right") << Config::FloatingRelativeTo::TopRight << QPointF(1610, 20);
        QTest::newRow("bottom-left") << Config::FloatingRelativeTo::BottomLeft << QPointF(10, 860);
        QTest::newRow("bottom-right") << Config::FloatingRelativeTo::BottomRight << QPointF(1610, 860);
        QTest::newRow("top") << Config::FloatingRelativeTo::Top << QPointF(820, 20);
        QTest::newRow("bottom") << Config::FloatingRelativeTo::Bottom << QPointF(820, 860);
        QTest::newRow("left") << Config::FloatingRelativeTo::Left << QPointF(10, 460);
        QTest::newRow("right") << Config::FloatingRelativeTo::Right << QPointF(1610, 460);
    }

    void defaultFloatingPositionRule()
    {
        QFETCH(Config::FloatingRelativeTo, relativeTo);
        QFETCH(QPointF, topLeft);
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("corner"));
        rule.openFloating = true;
        rule.defaultFloatingPosition = Config::FloatingPosition {10, 20, relativeTo};
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("corner"), QSizeF(300, 200));
        QCOMPARE(fixture.frame(id).topLeft(), topLeft);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFloatingLimits)
#include "test_layout_floatinglimits.moc"
