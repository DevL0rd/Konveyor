#include "helpers.h"

using namespace LayoutTest;

namespace
{

constexpr double Gaps = 16.0;

Config::Config atStartupFloatingConfig()
{
    Config::Config config = instantConfig();
    Config::WindowRule rule;
    Config::Match match;
    match.atStartup = true;
    rule.matches.append(match);
    rule.openFloating = true;
    config.windowRules.append(rule);
    return config;
}

Client lateClient()
{
    Client client;
    client.commitsLate = true;
    return client;
}

}

class TestLayoutHarness : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusChangeMidScrollEndsOnTheLastRequest()
    {
        Fixture fixture(linearAnimationConfig());
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto last = fixture.add(QStringLiteral("c"));
        fixture.advanceInSteps(400);
        fixture.perform(QStringLiteral("focus-column-first"));
        fixture.advanceInSteps(64);
        QVERIFY(fixture.engine().isAnimating());
        QVERIFY(fixture.state(first).renderFrame.x() < fixture.frame(first).x());
        fixture.perform(QStringLiteral("focus-column-last"));
        fixture.advanceInSteps(400);
        QVERIFY(!fixture.engine().isAnimating());
        QCOMPARE(fixture.focused(), std::optional(last));
        QCOMPARE(fixture.state(last).renderFrame, fixture.frame(last));
        QCOMPARE(fixture.frame(last).right(), 1920.0 - Gaps);
    }

    void closingAWindowWhileItOpensFocusesTheOneBefore()
    {
        Fixture fixture(linearAnimationConfig());
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.advanceInSteps(400);
        const auto opening = fixture.add(QStringLiteral("b"));
        fixture.advanceInSteps(48);
        QVERIFY(fixture.state(opening).renderAlpha < 1.0);
        fixture.remove(opening);
        fixture.advanceInSteps(400);
        QCOMPARE(fixture.focused(), std::optional(first));
        QCOMPARE(fixture.state(first).renderFrame, fixture.frame(first));
    }

    void resizeWhileSlidingSettlesAtTheNewWidth()
    {
        Fixture fixture(linearAnimationConfig());
        fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        fixture.advanceInSteps(400);
        fixture.perform(QStringLiteral("move-column-left"));
        fixture.advanceInSteps(32);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        fixture.advanceInSteps(400);
        QCOMPARE(fixture.state(second).renderFrame, fixture.frame(second));
        QCOMPARE(fixture.frame(second).x(), Gaps);
    }

    void clientThatRefusesToGrowKeepsItsOwnWidth()
    {
        Fixture fixture;
        Client narrow;
        narrow.maxSize = QSizeF(500, 0);
        const auto id = fixture.addClient(makeWindow(QStringLiteral("narrow")), narrow);
        const auto next = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(id).renderFrame.width(), 500.0);
        QCOMPARE(fixture.frame(next).left(), fixture.state(id).renderFrame.right() + Gaps);
    }

    void clientThatRefusesToShrinkPushesItsNeighbour()
    {
        Fixture fixture;
        Client wide;
        wide.minSize = QSizeF(1200, 0);
        const auto id = fixture.addClient(makeWindow(QStringLiteral("wide")), wide);
        const auto next = fixture.add(QStringLiteral("b"));
        fixture.advance(1);
        QCOMPARE(fixture.state(id).renderFrame.width(), 1200.0);
        QCOMPARE(fixture.state(next).renderFrame.left(), fixture.state(id).renderFrame.right() + Gaps);
        QCOMPARE(fixture.state(next).renderFrame.right(), 1920.0 - Gaps);
    }

    void fixedSizeClientIgnoresEveryRequest()
    {
        Fixture fixture;
        Client fixed;
        fixed.fixedSize = QSizeF(640, 480);
        const auto id = fixture.addClient(makeWindow(QStringLiteral("fixed")), fixed);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.state(id).renderFrame.size(), QSizeF(640, 480));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.state(id).renderFrame.size(), QSizeF(640, 480));
    }

    void lateClientAnswersOnTheNextSettle()
    {
        Fixture fixture;
        const auto id = fixture.addClient(makeWindow(QStringLiteral("late")), lateClient());
        QCOMPARE(fixture.state(id).renderFrame.width(), 936.0);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        QCOMPARE(fixture.state(id).renderFrame.width(), 936.0);
        QCOMPARE(fixture.frame(id).width(), 460.0);
        fixture.settle();
        QCOMPARE(fixture.state(id).renderFrame.width(), 460.0);
    }

    void atStartupRulesStopAfterSixtySeconds()
    {
        Fixture fixture(atStartupFloatingConfig());
        const auto early = fixture.add(QStringLiteral("a"));
        QVERIFY(fixture.state(early).isFloating);
        fixture.advance(59'999);
        QVERIFY(fixture.state(fixture.add(QStringLiteral("b"))).isFloating);
        fixture.advance(2);
        const auto late = fixture.add(QStringLiteral("c"));
        QVERIFY(!fixture.state(late).isFloating);
        QVERIFY(fixture.state(early).isFloating);
    }
};

QTEST_GUILESS_MAIN(TestLayoutHarness)
#include "test_layout_harness.moc"
