#include "helpers.h"

#include <functional>

using namespace LayoutTest;

namespace
{

struct CloseLog
{
    QList<Layout::WindowId> closed;

    Layout::Hooks hooks()
    {
        Layout::Hooks hooks;
        hooks.closeWindow = [this](Layout::WindowId id) { closed.append(id); };
        return hooks;
    }
};

struct TargetFixture
{
    CloseLog log;
    Fixture fixture {instantConfig(), QRectF(0, 0, 1920, 1080), log.hooks()};
    Layout::WindowId first = fixture.add(QStringLiteral("a"));
    Layout::WindowId second = fixture.add(QStringLiteral("b"));
    Layout::WindowId third = fixture.add(QStringLiteral("c"));
};

using TargetCheck = std::function<bool(TargetFixture &, Layout::WindowId)>;

struct TargetCase
{
    QString action;
    QStringList arguments;
    TargetCheck changed;
    bool clearedByFocus = false;
};

bool isMode(TargetFixture &f, Layout::WindowId id, Layout::WindowMode mode)
{
    return f.fixture.state(id).requestedSizingMode == mode;
}

QList<TargetCase> targetCases()
{
    const auto width = [](double value) {
        return [value](TargetFixture &f, Layout::WindowId id) { return qFuzzyCompare(f.fixture.frame(id).width(), value); };
    };
    return {
        {QStringLiteral("fullscreen-window"), {},
            [](TargetFixture &f, Layout::WindowId id) { return isMode(f, id, Layout::WindowMode::Fullscreen); }},
        {QStringLiteral("maximize-window-to-edges"), {},
            [](TargetFixture &f, Layout::WindowId id) { return isMode(f, id, Layout::WindowMode::Maximized); }},
        {QStringLiteral("cycle-window-expansion"), {},
            [](TargetFixture &f, Layout::WindowId id) { return f.fixture.frame(id).width() > 1000.0; }},
        {QStringLiteral("toggle-windowed-fullscreen"), {},
            [](TargetFixture &f, Layout::WindowId id) { return f.fixture.state(id).isWindowedFullscreen; }},
        {QStringLiteral("toggle-window-floating"), {},
            [](TargetFixture &f, Layout::WindowId id) { return f.fixture.state(id).isFloating; }},
        {QStringLiteral("move-window-to-floating"), {},
            [](TargetFixture &f, Layout::WindowId id) { return f.fixture.state(id).isFloating; }},
        {QStringLiteral("set-window-urgent"), {}, [](TargetFixture &f, Layout::WindowId id) { return f.fixture.state(id).isUrgent; }, true},
        {QStringLiteral("toggle-window-urgent"), {}, [](TargetFixture &f, Layout::WindowId id) { return f.fixture.state(id).isUrgent; },
            true},
        {QStringLiteral("close-window"), {},
            [](TargetFixture &f, Layout::WindowId id) { return f.log.closed == QList<Layout::WindowId> {id}; }},
        {QStringLiteral("set-window-width"), {QStringLiteral("400")}, width(400.0)},
        {QStringLiteral("set-column-width"), {QStringLiteral("400")}, width(400.0)},
        {QStringLiteral("switch-preset-window-width"), {}, width(1253.0)},
        {QStringLiteral("switch-preset-column-width"), {}, width(1253.0)},
        {QStringLiteral("switch-preset-window-width-back"), {}, width(618.0)},
        {QStringLiteral("switch-preset-column-width-back"), {}, width(618.0)},
        {QStringLiteral("maximize-column"), {}, width(1888.0)},
        {QStringLiteral("set-window-height"), {QStringLiteral("400")},
            [](TargetFixture &f, Layout::WindowId id) { return qFuzzyCompare(f.fixture.frame(id).height(), 400.0); }},
    };
}

void addTargetRows()
{
    QTest::addColumn<int>("row");
    const QList<TargetCase> cases = targetCases();
    for (int row = 0; row < cases.size(); ++row) {
        QTest::newRow(qPrintable(cases[row].action)) << row;
    }
}

}

class TestLayoutActionTargets : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionsWithoutIdActOnTheLastLayoutWindowAfterFocusLeft_data() { addTargetRows(); }

    void actionsWithoutIdActOnTheLastLayoutWindowAfterFocusLeft()
    {
        QFETCH(int, row);
        const TargetCase testCase = targetCases().at(row);
        TargetFixture f;
        f.fixture.engine().setLayoutFocused(false);
        QCOMPARE(f.fixture.focused(), std::optional<Layout::WindowId>());
        const Layout::ActionResult result = f.fixture.perform(testCase.action, testCase.arguments);
        QVERIFY2(result.ok, qPrintable(result.error));
        QVERIFY2(testCase.changed(f, f.third) != testCase.clearedByFocus, "the action did not act on the last focused layout window");
        QCOMPARE(f.fixture.focused(), std::optional(f.third));
        VERIFY_INVARIANTS(f.fixture);
    }

    void actionsWithIdActOnThatWindowOnly_data() { addTargetRows(); }

    void actionsWithIdActOnThatWindowOnly()
    {
        QFETCH(int, row);
        const TargetCase testCase = targetCases().at(row);
        TargetFixture f;
        const QRectF focusedBefore = f.fixture.frame(f.third);
        const Layout::ActionResult result = f.fixture.perform(testCase.action, testCase.arguments, idProperty(f.first));
        QVERIFY2(result.ok, qPrintable(result.error));
        QVERIFY2(testCase.changed(f, f.first), "the action did not act on the window given by id");
        QVERIFY2(!testCase.changed(f, f.third), "the focused window changed too");
        QCOMPARE(f.fixture.frame(f.third).size(), focusedBefore.size());
        QVERIFY(!f.fixture.state(f.third).isFloating);
        QCOMPARE(f.fixture.state(f.third).requestedSizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(f.fixture);
    }

    void actionsWithTargetArgumentActOnThatWindowOnly_data() { addTargetRows(); }

    void actionsWithTargetArgumentActOnThatWindowOnly()
    {
        QFETCH(int, row);
        const TargetCase testCase = targetCases().at(row);
        TargetFixture f;
        const QSizeF focusedBefore = f.fixture.frame(f.third).size();
        const Layout::ActionResult result = f.fixture.engine().perform(action(testCase.action, testCase.arguments), f.second);
        f.fixture.settle();
        QVERIFY2(result.ok, qPrintable(result.error));
        QVERIFY2(testCase.changed(f, f.second), "the action did not act on the target window");
        QCOMPARE(f.fixture.frame(f.third).size(), focusedBefore);
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveWindowToMonitorWithoutIdMovesTheLastLayoutWindowAfterFocusLeft()
    {
        TargetFixture f;
        f.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        f.fixture.engine().setLayoutFocused(false);
        QVERIFY(f.fixture.perform(QStringLiteral("move-window-to-monitor-next")).ok);
        QCOMPARE(f.fixture.state(f.third).output, QStringLiteral("DP-2"));
        QCOMPARE(f.fixture.state(f.first).output, QStringLiteral("DP-1"));
        QCOMPARE(f.fixture.focused(), std::optional(f.third));
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveWindowToMonitorWithIdMovesThatWindow()
    {
        TargetFixture f;
        f.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QVERIFY(f.fixture.perform(QStringLiteral("move-window-to-monitor"), {QStringLiteral("DP-2")}, idProperty(f.first)).ok);
        QCOMPARE(f.fixture.state(f.first).output, QStringLiteral("DP-2"));
        QCOMPARE(f.fixture.state(f.third).output, QStringLiteral("DP-1"));
        VERIFY_INVARIANTS(f.fixture);
    }

    void closeWindowWithUnknownIdIsForwarded()
    {
        TargetFixture f;
        QVERIFY(f.fixture.perform(QStringLiteral("close-window"), {}, idProperty(99)).ok);
        QCOMPARE(f.log.closed, QList<Layout::WindowId> {99});
    }

    void windowActionsWithUnknownIdReportAnError_data()
    {
        QTest::addColumn<QString>("name");
        for (const char *name : {"toggle-windowed-fullscreen", "toggle-window-rule-opacity", "set-window-urgent", "unset-window-urgent",
                 "toggle-window-urgent", "focus-window"}) {
            QTest::newRow(name) << QString::fromLatin1(name);
        }
    }

    void windowActionsWithUnknownIdReportAnError()
    {
        QFETCH(QString, name);
        TargetFixture f;
        QVERIFY(!f.fixture.perform(name, {}, idProperty(99)).ok);
        QCOMPARE(f.fixture.focused(), std::optional(f.third));
        VERIFY_INVARIANTS(f.fixture);
    }

    void closeWindowWithNothingInTheLayoutReportsAnError()
    {
        CloseLog log;
        Fixture fixture(instantConfig(), QRectF(0, 0, 1920, 1080), log.hooks());
        QVERIFY(!fixture.perform(QStringLiteral("close-window")).ok);
        QVERIFY(log.closed.isEmpty());
    }

    void windowIdPropertyAliasIsAccepted()
    {
        TargetFixture f;
        const QList<std::pair<QString, QString>> properties {{QStringLiteral("window-id"), QString::number(f.first)}};
        QVERIFY(f.fixture.perform(QStringLiteral("toggle-window-floating"), {}, properties).ok);
        QVERIFY(f.fixture.state(f.first).isFloating);
        QVERIFY(!f.fixture.state(f.third).isFloating);
    }

    void idPropertyWinsOverTheTargetArgument()
    {
        TargetFixture f;
        const Config::Action toggle = action(QStringLiteral("toggle-window-floating"), {}, idProperty(f.first));
        QVERIFY(f.fixture.engine().perform(toggle, f.second).ok);
        QVERIFY(f.fixture.state(f.first).isFloating);
        QVERIFY(!f.fixture.state(f.second).isFloating);
    }
};

QTEST_GUILESS_MAIN(TestLayoutActionTargets)
#include "test_layout_actiontargets.moc"
