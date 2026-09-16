#include "helpers.h"

#include <functional>

using namespace LayoutTest;

namespace
{

const QStringList &simpleActionNames()
{
    static const QStringList names {QStringLiteral("focus-column-left"), QStringLiteral("focus-column-right"),
        QStringLiteral("focus-column-first"), QStringLiteral("focus-column-last"), QStringLiteral("focus-column-right-or-first"),
        QStringLiteral("focus-column-left-or-last"), QStringLiteral("focus-window-down"), QStringLiteral("focus-window-up"),
        QStringLiteral("focus-window-down-or-column-left"), QStringLiteral("focus-window-down-or-column-right"),
        QStringLiteral("focus-window-up-or-column-left"), QStringLiteral("focus-window-up-or-column-right"),
        QStringLiteral("focus-window-top"), QStringLiteral("focus-window-bottom"), QStringLiteral("focus-window-down-or-top"),
        QStringLiteral("focus-window-up-or-bottom"), QStringLiteral("focus-window-or-workspace-down"),
        QStringLiteral("focus-window-or-workspace-up"), QStringLiteral("focus-window-or-monitor-up"),
        QStringLiteral("focus-window-or-monitor-down"), QStringLiteral("focus-column-or-monitor-left"),
        QStringLiteral("focus-column-or-monitor-right"), QStringLiteral("focus-window-previous"), QStringLiteral("focus-floating"),
        QStringLiteral("focus-tiling"), QStringLiteral("switch-focus-between-floating-and-tiling"), QStringLiteral("move-column-left"),
        QStringLiteral("move-column-right"), QStringLiteral("move-column-to-first"), QStringLiteral("move-column-to-last"),
        QStringLiteral("move-column-left-or-to-monitor-left"), QStringLiteral("move-column-right-or-to-monitor-right"),
        QStringLiteral("move-window-down"), QStringLiteral("move-window-up"), QStringLiteral("move-window-down-or-to-workspace-down"),
        QStringLiteral("move-window-up-or-to-workspace-up"), QStringLiteral("consume-or-expel-window-left"),
        QStringLiteral("consume-or-expel-window-right"), QStringLiteral("consume-window-into-column"),
        QStringLiteral("expel-window-from-column"), QStringLiteral("swap-window-left"), QStringLiteral("swap-window-right"),
        QStringLiteral("toggle-column-tabbed-display"), QStringLiteral("center-column"), QStringLiteral("center-window"),
        QStringLiteral("center-visible-columns"), QStringLiteral("focus-workspace-down"), QStringLiteral("focus-workspace-up"),
        QStringLiteral("focus-workspace-previous"), QStringLiteral("move-window-to-workspace-down"),
        QStringLiteral("move-window-to-workspace-up"), QStringLiteral("move-column-to-workspace-down"),
        QStringLiteral("move-column-to-workspace-up"), QStringLiteral("move-workspace-down"), QStringLiteral("move-workspace-up"),
        QStringLiteral("switch-preset-column-width"), QStringLiteral("switch-preset-column-width-back"),
        QStringLiteral("switch-preset-window-width"), QStringLiteral("switch-preset-window-width-back"),
        QStringLiteral("switch-preset-window-height"), QStringLiteral("switch-preset-window-height-back"),
        QStringLiteral("reset-window-height"), QStringLiteral("maximize-column"), QStringLiteral("maximize-window-to-edges"),
        QStringLiteral("expand-column-to-available-width"), QStringLiteral("fullscreen-window"),
        QStringLiteral("toggle-windowed-fullscreen"), QStringLiteral("toggle-window-floating"), QStringLiteral("move-window-to-floating"),
        QStringLiteral("move-window-to-tiling"), QStringLiteral("move-floating-window"), QStringLiteral("toggle-window-rule-opacity"),
        QStringLiteral("close-window"), QStringLiteral("toggle-overview"), QStringLiteral("open-overview"),
        QStringLiteral("close-overview")};
    return names;
}

const QStringList &directionalPrefixes()
{
    static const QStringList prefixes {QStringLiteral("focus-monitor-"), QStringLiteral("move-window-to-monitor-"),
        QStringLiteral("move-column-to-monitor-"), QStringLiteral("move-workspace-to-monitor-")};
    return prefixes;
}

const QStringList &compositorActionNames()
{
    static const QStringList names {QStringLiteral("show-hotkey-overlay")};
    return names;
}

QString checkResult(const QString &name, const Layout::ActionResult &result, Fixture &fixture)
{
    if (!result.ok) {
        return name + QStringLiteral(": ") + result.error;
    }
    const QString invariants = fixture.invariants();
    return invariants.isEmpty() ? QString() : name + QStringLiteral(": ") + invariants;
}

QString runSimpleAction(const QString &name)
{
    Fixture fixture;
    fixture.add(QStringLiteral("a"));
    fixture.add(QStringLiteral("b"));
    fixture.perform(QStringLiteral("consume-or-expel-window-left"));
    fixture.add(QStringLiteral("c"));
    return checkResult(name, fixture.perform(name), fixture);
}

QString runMonitorAction(const QString &name, const QStringList &arguments)
{
    Fixture fixture;
    fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
    fixture.add(QStringLiteral("a"));
    return checkResult(name, fixture.perform(name, arguments), fixture);
}

QStringList collectErrors(const QStringList &names, const std::function<QString(const QString &)> &run)
{
    QStringList errors;
    for (const QString &name : names) {
        const QString error = run(name);
        errors.append(error);
    }
    errors.removeAll(QString());
    return errors;
}

QStringList directionalActionNames()
{
    static const QStringList directions {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("up"), QStringLiteral("down"),
        QStringLiteral("previous"), QStringLiteral("next")};
    QStringList names;
    for (const QString &prefix : directionalPrefixes()) {
        for (const QString &direction : directions) {
            names.append(prefix + direction);
        }
    }
    return names;
}

}

class TestLayoutActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void everySimpleActionSucceeds() { QCOMPARE(collectErrors(simpleActionNames(), runSimpleAction), QStringList()); }

    void everyDirectionalMonitorActionSucceeds()
    {
        const auto run = [](const QString &name) { return runMonitorAction(name, {}); };
        QCOMPARE(collectErrors(directionalActionNames(), run), QStringList());
    }

    void everyNamedMonitorActionSucceeds()
    {
        static const QStringList names {QStringLiteral("focus-monitor"), QStringLiteral("move-window-to-monitor"),
            QStringLiteral("move-column-to-monitor"), QStringLiteral("move-workspace-to-monitor")};
        const auto run = [](const QString &name) { return runMonitorAction(name, {QStringLiteral("DP-2")}); };
        QCOMPARE(collectErrors(names, run), QStringList());
    }

    void actionsWithArgumentsSucceed()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const QString window = QString::number(id);
        QVERIFY(fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("1")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-in-column"), {QStringLiteral("1")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("focus-window"), {}, {{QStringLiteral("id"), window}}).ok);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("1")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-index"), {QStringLiteral("1")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("tabbed")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("2")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-workspace"), {QStringLiteral("1")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-workspace-to-index"), {QStringLiteral("1")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-workspace-name"), {QStringLiteral("x")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("unset-workspace-name")).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("50%")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("400")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("400")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-urgent"), {}, {{QStringLiteral("id"), window}}).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-window-urgent"), {}, {{QStringLiteral("id"), window}}).ok);
        QVERIFY(fixture.perform(QStringLiteral("unset-window-urgent"), {}, {{QStringLiteral("id"), window}}).ok);
        VERIFY_INVARIANTS(fixture);
    }

    void unknownActionIsReported()
    {
        Fixture fixture;
        const Layout::ActionResult result = fixture.perform(QStringLiteral("do-a-barrel-roll"));
        QVERIFY(!result.ok);
        QCOMPARE(result.error, QStringLiteral("unknown action: do-a-barrel-roll"));
        for (const QString &removed : {QStringLiteral("quit"), QStringLiteral("screenshot"), QStringLiteral("load-config-file")}) {
            QVERIFY(!fixture.perform(removed).ok);
        }
    }

    void badArgumentsAreReported()
    {
        Fixture fixture;
        fixture.add();
        QVERIFY(!fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("abc")}).ok);
        QVERIFY(!fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("abc")}).ok);
        QVERIFY(!fixture.perform(QStringLiteral("focus-workspace")).ok);
        QVERIFY(!fixture.perform(QStringLiteral("move-floating-window"), {}, {{QStringLiteral("x"), QStringLiteral("zz")}}).ok);
        QVERIFY(!fixture.perform(QStringLiteral("set-workspace-name")).ok);
    }

    void closeWindowUsesHook()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        QList<Layout::WindowId> closed;
        Layout::Hooks hooks;
        hooks.closeWindow = [&closed](Layout::WindowId id) { closed.append(id); };
        Layout::Engine engine(clock, hooks);
        engine.setConfig(instantConfig());
        engine.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080)));
        engine.addWindow(7, makeWindow(), QString(), Layout::ActivationPolicy::Focus);
        QVERIFY(engine.perform(action(QStringLiteral("close-window"))).ok);
        QCOMPARE(closed, QList<Layout::WindowId> {7});
    }

    void compositorActionsAreForwarded()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        QStringList forwarded;
        QStringList spawned;
        Layout::Hooks hooks;
        hooks.compositorAction = [&forwarded](const QString &name) { forwarded.append(name); };
        hooks.spawn = [&spawned](const QString &command) { spawned.append(command); };
        Layout::Engine engine(clock, hooks);
        engine.setConfig(instantConfig());
        engine.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080)));

        QStringList failed;
        for (const QString &name : compositorActionNames()) {
            const bool ok = engine.perform(action(name)).ok;
            failed.append(ok ? QString() : name);
        }
        failed.removeAll(QString());
        QCOMPARE(failed, QStringList());
        QCOMPARE(forwarded, compositorActionNames());

        QVERIFY(engine.perform(action(QStringLiteral("spawn"), {QStringLiteral("alacritty"), QStringLiteral("-e")})).ok);
        QVERIFY(engine.perform(action(QStringLiteral("spawn-sh"), {QStringLiteral("echo hi")})).ok);
        QCOMPARE(spawned, QStringList({QStringLiteral("alacritty -e"), QStringLiteral("echo hi")}));
    }

    void overviewHooksAreCalled()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        int toggles = 0;
        QList<bool> opened;
        Layout::Hooks hooks;
        hooks.toggleOverview = [&toggles]() { toggles += 1; };
        hooks.setOverviewOpen = [&opened](bool open) { opened.append(open); };
        Layout::Engine engine(clock, hooks);
        engine.setConfig(instantConfig());
        engine.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080)));
        QVERIFY(engine.perform(action(QStringLiteral("toggle-overview"))).ok);
        QCOMPARE(toggles, 1);
        QVERIFY(engine.perform(action(QStringLiteral("open-overview"))).ok);
        QVERIFY(engine.perform(action(QStringLiteral("close-overview"))).ok);
        QCOMPARE(opened, QList<bool>({true, false}));
        engine.setOverviewOpen(true);
        QVERIFY(engine.isOverviewOpen());
    }

    void focusWindowPreviousReturnsToPriorWindow()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.focused(), second);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-previous")).ok);
        QCOMPARE(fixture.focused(), first);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-previous")).ok);
        QCOMPARE(fixture.focused(), second);
        VERIFY_INVARIANTS(fixture);
    }

    void focusWindowByIdFailsForUnknownWindow()
    {
        Fixture fixture;
        fixture.add();
        const auto result = fixture.perform(QStringLiteral("focus-window"), {}, {{QStringLiteral("id"), QStringLiteral("99")}});
        QVERIFY(!result.ok);
    }

    void performCanTargetAnExplicitWindow()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto result = fixture.engine().perform(action(QStringLiteral("toggle-window-floating")), first);
        QVERIFY(result.ok);
        QVERIFY(fixture.state(first).isFloating);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutActions)
#include "test_layout_actions.moc"
