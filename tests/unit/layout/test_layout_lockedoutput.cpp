#include "helpers.h"

using namespace LayoutTest;

namespace
{

void shrinkTo(Fixture &fixture, Layout::WindowId id, QSizeF size)
{
    const Layout::WindowProperties smaller = lockedWindow(size);
    fixture.setClient(id, LayoutTest::Client::honoring(smaller));
    fixture.engine().setWindowNativeSize(id, size);
    fixture.engine().updateWindowProperties(id, smaller);
}

}

class TestLayoutLockedOutput : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void windowOpeningLockedToOutputSizeCoversItInItsColumn()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto id = fixture.addWith(lockedWindow());
        QVERIFY(!fixture.state(id).isFloating);
        QVERIFY(fixture.state(id).isWindowedFullscreen);
        QCOMPARE(fixture.state(id).columnIndex, 1);
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void coveringWindowGivesWayWhenANeighbourIsFocused()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto id = fixture.addWith(lockedWindow());
        fixture.perform(QStringLiteral("focus-column-left"));
        QVERIFY(fixture.state(first).stackingIndex > fixture.state(id).stackingIndex);
        QVERIFY(fixture.frame(id).left() >= fixture.frame(first).right());
        VERIFY_INVARIANTS(fixture);
    }

    void lockedWindowReturnsToTheGridWhenItShrinks_data()
    {
        QTest::addColumn<bool>("throughFullscreen");
        QTest::newRow("with a fullscreen step") << true;
        QTest::newRow("without a fullscreen step") << false;
    }

    void lockedWindowReturnsToTheGridWhenItShrinks()
    {
        QFETCH(bool, throughFullscreen);
        Fixture fixture;
        fixture.add();
        const auto id = fixture.addWith(lockedWindow());
        QVERIFY(fixture.state(id).isWindowedFullscreen);
        if (throughFullscreen) {
            fixture.engine().setWindowFullscreen(id, true);
            fixture.commitAs(id, QSizeF(1920, 1080));
            QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        }
        shrinkTo(fixture, id, QSizeF(960, 1080));
        if (throughFullscreen) {
            fixture.engine().setWindowFullscreen(id, false);
        }
        fixture.settle();
        QVERIFY(!fixture.state(id).isFloating);
        QVERIFY(!fixture.state(id).isWindowedFullscreen);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.frame(id).width(), 960.0);
        VERIFY_INVARIANTS(fixture);
    }

    void userFloatedFixedWindowStaysFloatingThroughFullscreen()
    {
        Fixture fixture;
        fixture.add();
        const auto id = fixture.addWith(lockedWindow(QSizeF(960, 1080)));
        QVERIFY(!fixture.state(id).isFloating);
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(id).isFloating);
        fixture.engine().setWindowFullscreen(id, true);
        fixture.settle();
        QVERIFY(!fixture.state(id).isFloating);
        fixture.engine().setWindowFullscreen(id, false);
        fixture.settle();
        QVERIFY(fixture.state(id).isFloating);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutLockedOutput)
#include "test_layout_lockedoutput.moc"
