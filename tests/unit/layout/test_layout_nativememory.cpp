#include "forcehelpers.h"

using namespace LayoutTest;

class TestLayoutNativeMemory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void reopeningWithForceOnReturnsToTheStoredNativeSize()
    {
        Fixture fixture(forceResizableConfig());
        const auto first = fixture.addWith(nonResizableGame());
        fixture.settle();
        fixture.remove(first);

        Layout::WindowProperties savedForcedSize = nonResizableGame();
        savedForcedSize.frameSize = QSizeF(900, 700);
        const auto reopened = fixture.addWith(savedForcedSize);
        QVERIFY(fixture.frame(reopened).size() != QSizeF(300, 200));
        fixture.setConfig(instantConfig());
        fixture.settle();
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(300, 200));
        VERIFY_INVARIANTS(fixture);
    }

    void reopeningWithForceOffTakesTheDeclaredSize()
    {
        Fixture fixture(instantConfig());
        const auto first = fixture.addWith(nonResizableGame());
        fixture.remove(first);

        Layout::WindowProperties changed = nonResizableGame();
        changed.frameSize = QSizeF(900, 700);
        const auto reopened = fixture.addWith(changed);
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(900, 700));
    }

    void aChoiceMadeWhileForcedIsStoredAcrossReopens()
    {
        Fixture fixture(forceResizableConfig());
        const auto first = fixture.addWith(nonResizableGame());
        fixture.engine().setWindowNativeSize(first, QSizeF(640, 480));
        fixture.settle();
        fixture.remove(first);

        Layout::WindowProperties savedForcedSize = nonResizableGame();
        savedForcedSize.frameSize = QSizeF(900, 700);
        const auto reopened = fixture.addWith(savedForcedSize);
        fixture.setConfig(instantConfig());
        fixture.settle();
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(640, 480));
    }

    void anotherAppKeepsItsOwnNativeSize()
    {
        Config::Config config = forceResizableConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("other"));
        rule.forceResizable = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto game = fixture.addWith(nonResizableGame());
        fixture.engine().setWindowNativeSize(game, QSizeF(640, 480));
        Layout::WindowProperties other = makeWindow(QStringLiteral("other"), QStringLiteral("other"), QSizeF(700, 500));
        other.isResizable = false;
        const auto second = fixture.addWith(other);
        fixture.settle();
        fixture.remove(second);
        fixture.remove(game);

        other.frameSize = QSizeF(1000, 800);
        const auto reopened = fixture.addWith(other);
        fixture.setConfig(instantConfig());
        fixture.settle();
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(700, 500));
    }
};

QTEST_GUILESS_MAIN(TestLayoutNativeMemory)
#include "test_layout_nativememory.moc"
