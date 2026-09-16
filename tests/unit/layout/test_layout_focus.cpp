#include "helpers.h"

using namespace LayoutTest;

namespace
{

struct FocusLog
{
    QList<Layout::WindowId> requests;

    Layout::Hooks hooks()
    {
        Layout::Hooks hooks;
        hooks.focusWindow = [this](Layout::WindowId id) { requests.append(id); };
        return hooks;
    }
};

struct FocusFixture
{
    FocusLog log;
    Fixture fixture {instantConfig(), QRectF(0, 0, 1920, 1080), log.hooks()};
    Layout::WindowId first = fixture.add();
    Layout::WindowId second = fixture.add();

    FocusFixture() { log.requests.clear(); }
};

}

class TestLayoutFocus : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layoutActionsRequestFocusOncePerChange()
    {
        FocusFixture f;
        f.fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(f.log.requests, QList<Layout::WindowId> {f.first});
        f.fixture.advance(1000);
        f.fixture.engine().windowSizeCommitted(f.first, f.fixture.frame(f.first).size());
        QCOMPARE(f.log.requests, QList<Layout::WindowId> {f.first});

        f.fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(f.log.requests, (QList<Layout::WindowId> {f.first, f.second}));
    }

    void focusReportedByTheCompositorIsNotEchoedBack()
    {
        FocusFixture f;
        f.fixture.engine().activateWindow(f.first);
        QCOMPARE(f.fixture.focused(), std::optional(f.first));
        QVERIFY(f.log.requests.isEmpty());
    }

    void focusOutsideTheLayoutDeactivatesEveryWindow()
    {
        FocusFixture f;
        f.fixture.engine().setLayoutFocused(false);
        QCOMPARE(f.fixture.focused(), std::optional<Layout::WindowId>());
        QVERIFY(!f.fixture.state(f.first).isActive && !f.fixture.state(f.second).isActive);
        QVERIFY(f.log.requests.isEmpty());

        f.fixture.engine().activateWindow(f.second);
        QCOMPARE(f.fixture.focused(), std::optional(f.second));
        QVERIFY(f.fixture.state(f.second).isActive);
        QVERIFY(f.log.requests.isEmpty());
    }

    void layoutActionTakesFocusBackFromOutside()
    {
        FocusFixture f;
        f.fixture.engine().setLayoutFocused(false);
        f.fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(f.fixture.focused(), std::optional(f.first));
        QCOMPARE(f.log.requests, QList<Layout::WindowId> {f.first});
    }

    void closingTheFocusedWindowRequestsFocusForItsReplacement()
    {
        FocusFixture f;
        f.fixture.remove(f.second);
        QCOMPARE(f.log.requests.size(), 1);
        QCOMPARE(std::optional(f.log.requests.constFirst()), f.fixture.focused());
    }
};

QTEST_GUILESS_MAIN(TestLayoutFocus)
#include "test_layout_focus.moc"
