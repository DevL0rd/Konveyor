#include "helpers.h"

using namespace LayoutTest;

namespace
{

Layout::WindowProperties limited(const QString &appId, QSizeF size, QSizeF minSize, QSizeF maxSize)
{
    Layout::WindowProperties properties = makeWindow(appId, appId, size);
    properties.minSize = minSize;
    properties.maxSize = maxSize;
    return properties;
}

Layout::WindowProperties fixedSize(const QString &appId, QSizeF size)
{
    Layout::WindowProperties properties = makeWindow(appId, appId, size);
    properties.isResizable = false;
    return properties;
}

void consumeLeft(Fixture &fixture)
{
    fixture.perform(QStringLiteral("consume-or-expel-window-left"));
}

}

class TestLayoutConstraints : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stackedMinimumHeightsThatDoNotFitOverflowInOrder()
    {
        Fixture fixture;
        QList<Layout::WindowId> ids;
        for (int i = 0; i < 3; ++i) {
            ids.append(fixture.addWith(limited(QStringLiteral("tall"), QSizeF(300, 300), QSizeF(0, 500), QSizeF())));
            if (i > 0) {
                consumeLeft(fixture);
            }
        }
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(fixture.state(ids[i]).columnIndex, 0);
            QCOMPARE(fixture.frame(ids[i]).height(), 500.0);
            QCOMPARE(fixture.frame(ids[i]).y(), 16.0 + i * 516.0);
        }
        VERIFY_INVARIANTS(fixture);
    }

    void aMinimumHeightTakesItsShareFirst()
    {
        Fixture fixture;
        const auto small = fixture.add(QStringLiteral("a"));
        const auto tall = fixture.addWith(limited(QStringLiteral("tall"), QSizeF(300, 300), QSizeF(0, 800), QSizeF()));
        consumeLeft(fixture);
        QCOMPARE(fixture.frame(tall).height(), 800.0);
        QCOMPARE(fixture.frame(small).height(), 232.0);
        fixture.perform(QStringLiteral("focus-window-up"));
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("900")});
        QCOMPARE(fixture.frame(small).height(), 232.0);
        QCOMPARE(fixture.frame(tall).height(), 800.0);
        VERIFY_INVARIANTS(fixture);
    }

    void aMaximumHeightLeavesTheRestToItsNeighbour()
    {
        Fixture fixture;
        const auto free = fixture.add(QStringLiteral("a"));
        const auto capped = fixture.addWith(limited(QStringLiteral("capped"), QSizeF(300, 300), QSizeF(), QSizeF(0, 200)));
        consumeLeft(fixture);
        QCOMPARE(fixture.frame(capped).height(), 200.0);
        QCOMPARE(fixture.frame(free).height(), 832.0);
        QCOMPARE(fixture.frame(capped).y(), 864.0);
        VERIFY_INVARIANTS(fixture);
    }

    void widthLimitsBeatPresets()
    {
        Fixture fixture;
        const auto wide = fixture.addWith(limited(QStringLiteral("wide"), QSizeF(300, 300), QSizeF(1000, 0), QSizeF()));
        QCOMPARE(fixture.frame(wide).width(), 1000.0);
        fixture.perform(QStringLiteral("switch-preset-column-width-back"));
        QCOMPARE(fixture.frame(wide).width(), 1000.0);
        const auto narrow = fixture.addWith(limited(QStringLiteral("narrow"), QSizeF(300, 300), QSizeF(), QSizeF(500, 0)));
        QCOMPARE(fixture.frame(narrow).width(), 500.0);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(narrow).width(), 1888.0);
        VERIFY_INVARIANTS(fixture);
    }

    void consumingTwoFixedWidthWindowsKeepsBothSizes()
    {
        Fixture fixture;
        const auto narrow = fixture.addWith(fixedSize(QStringLiteral("narrow"), QSizeF(400, 300)));
        const auto wide = fixture.addWith(fixedSize(QStringLiteral("wide"), QSizeF(600, 300)));
        consumeLeft(fixture);
        QCOMPARE(fixture.state(wide).columnIndex, 0);
        QCOMPARE(fixture.frame(narrow).size(), QSizeF(400, 300));
        QCOMPARE(fixture.frame(wide).size(), QSizeF(600, 300));
        QCOMPARE(fixture.frame(wide).y(), fixture.frame(narrow).bottom() + 16.0);
        const auto next = fixture.add(QStringLiteral("next"));
        QCOMPARE(fixture.frame(next).x(), 16.0 + 600.0 + 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedSizeWindowsIgnoreEveryNormalSizeChange()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("other"));
        const auto id = fixture.addWith(fixedSize(QStringLiteral("fixed"), QSizeF(400, 300)));
        const QSizeF native(400, 300);
        for (const QString &action : {QStringLiteral("switch-preset-column-width"), QStringLiteral("switch-preset-window-height"),
                 QStringLiteral("expand-column-to-available-width"), QStringLiteral("reset-window-height")}) {
            fixture.perform(action);
            QCOMPARE(fixture.frame(id).size(), native);
        }
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("80%")});
        QCOMPARE(fixture.frame(id).size(), native);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("900")});
        QCOMPARE(fixture.frame(id).size(), native);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedSizeWindowsExpandOnlyWhileExpanded()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("other"));
        const auto id = fixture.addWith(fixedSize(QStringLiteral("fixed"), QSizeF(400, 300)));
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(id).size(), QSizeF(1888, 1048));
        QVERIFY(fixture.state(id).isExpansionForceResizable);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(id).size(), QSizeF(400, 300));
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QCOMPARE(fixture.frame(id).size(), QSizeF(400, 300));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(id).size(), QSizeF(400, 300));
        QVERIFY(!fixture.state(id).isExpansionForceResizable);
        VERIFY_INVARIANTS(fixture);
    }

    void aLateAnswerKeepsTheRowLaidOutForTheRequest()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        fixture.engine().activateWindow(a);
        fixture.holdCommits(true);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("400")});
        QCOMPARE(fixture.frame(a).width(), 400.0);
        QCOMPARE(fixture.frame(b).x(), 432.0);
        fixture.holdCommits(false);
        QCOMPARE(fixture.frame(a).width(), 400.0);
        QCOMPARE(fixture.frame(b).x(), 432.0);
        VERIFY_INVARIANTS(fixture);
    }

    void aDifferentAnswerMovesTheNeighbours()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        fixture.engine().activateWindow(a);
        fixture.holdCommits(true);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("400")});
        fixture.commitAs(a, QSizeF(450, 1048));
        QCOMPARE(fixture.frame(a).width(), 400.0);
        QCOMPARE(fixture.frame(b).x(), 16.0 + 450.0 + 16.0);
        QCOMPARE(fixture.frame(b).width(), 936.0);
        fixture.holdCommits(false);
        QCOMPARE(fixture.frame(b).x(), 16.0 + 400.0 + 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void anAppResizingItselfMovesTheNeighbours()
    {
        Fixture fixture;
        const auto a = fixture.addWith(fixedSize(QStringLiteral("fixed"), QSizeF(400, 300)));
        const auto b = fixture.add(QStringLiteral("b"));
        Layout::WindowProperties grown = fixedSize(QStringLiteral("fixed"), QSizeF(640, 480));
        fixture.engine().updateWindowProperties(a, grown);
        fixture.commitAs(a, QSizeF(640, 480));
        fixture.settle();
        QCOMPARE(fixture.frame(a).size(), QSizeF(640, 480));
        QCOMPARE(fixture.frame(b).x(), 16.0 + 640.0 + 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void minimumRulesWinOverTheApp()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("app"));
        rule.minWidth = 1200;
        rule.maxHeight = 600;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.addWith(limited(QStringLiteral("app"), QSizeF(300, 300), QSizeF(100, 100), QSizeF(800, 0)));
        QCOMPARE(fixture.frame(id).width(), 1200.0);
        QCOMPARE(fixture.frame(id).height(), 600.0);
        QCOMPARE(fixture.state(id).requestedMinSize, std::optional(QSizeF(1200, 100)));
        QCOMPARE(fixture.state(id).requestedMaxSize, std::optional(QSizeF(800, 600)));
    }
};

QTEST_GUILESS_MAIN(TestLayoutConstraints)
#include "test_layout_constraints.moc"
