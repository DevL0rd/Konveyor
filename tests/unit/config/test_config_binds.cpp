#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

Bind firstBind(const QString &body)
{
    const QList<Bind> binds = parsed(QStringLiteral("binds {\n%1\n}\n").arg(body)).binds;
    return binds.isEmpty() ? Bind {} : binds.first();
}

LoadError bindError(const QString &body)
{
    return mustFail(QStringLiteral("binds {\n%1\n}\n").arg(body));
}

}

class TestConfigBinds : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesKeysymTriggers();
    void parsesModifiers_data();
    void parsesModifiers();
    void parsesMouseTriggers_data();
    void parsesMouseTriggers();
    void parsesScrollTriggers_data();
    void parsesScrollTriggers();
    void parsesBindProperties();
    void hotkeyOverlayTitleNullHides();
    void resolvesModKey();
    void resolvesModKeyFromInputSection();
    void capturesActionArgumentsAndProperties();
    void rejectsUnknownAction();
    void rejectsMissingActionArgument();
    void rejectsUnknownActionProperty();
    void rejectsMissingAction();
    void rejectsMultipleActions();
    void rejectsDuplicateBinds();
    void rejectsInvalidModifierAndKey();
    void rejectsRemovedBindOptions();
};

void TestConfigBinds::parsesKeysymTriggers()
{
    const Bind bind = firstBind(QStringLiteral("Mod+Shift+Slash { show-hotkey-overlay; }"));
    QCOMPARE(bind.trigger, BindTrigger::Key);
    QCOMPARE(bind.keyText, QStringLiteral("Slash"));
    QCOMPARE(bind.keysym, keysymFromName(QStringLiteral("slash")));
    QCOMPARE(bind.action.name, QStringLiteral("show-hotkey-overlay"));

    const Bind media = firstBind(QStringLiteral("XF86AudioPlay { spawn-sh \"playerctl play-pause\"; }"));
    QCOMPARE(media.keysym, keysymFromName(QStringLiteral("XF86AudioPlay")));
    QCOMPARE(media.keyModifiers, BindModifiers());

    const Bind arrow = firstBind(QStringLiteral("Mod+Page_Down { focus-workspace-down; }"));
    QCOMPARE(arrow.key, int(Qt::Key_PageDown));
    QCOMPARE(firstBind(QStringLiteral("Mod+K { close-window; }")).key, int(Qt::Key_K));
}

void TestConfigBinds::parsesModifiers_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<int>("flags");

    QTest::newRow("ctrl") << QStringLiteral("Ctrl+A") << int(BindModifier::Ctrl);
    QTest::newRow("control") << QStringLiteral("Control+A") << int(BindModifier::Ctrl);
    QTest::newRow("shift") << QStringLiteral("Shift+A") << int(BindModifier::Shift);
    QTest::newRow("alt") << QStringLiteral("Alt+A") << int(BindModifier::Alt);
    QTest::newRow("super") << QStringLiteral("Super+A") << int(BindModifier::Super);
    QTest::newRow("win") << QStringLiteral("Win+A") << int(BindModifier::Super);
    QTest::newRow("iso3") << QStringLiteral("ISO_Level3_Shift+A") << int(BindModifier::IsoLevel3Shift);
    QTest::newRow("mod5") << QStringLiteral("Mod5+A") << int(BindModifier::IsoLevel3Shift);
    QTest::newRow("iso5") << QStringLiteral("ISO_Level5_Shift+A") << int(BindModifier::IsoLevel5Shift);
    QTest::newRow("mod3") << QStringLiteral("Mod3+A") << int(BindModifier::IsoLevel5Shift);
    QTest::newRow("mod") << QStringLiteral("Mod+A") << int(BindModifier::Mod);
    QTest::newRow("lowercase") << QStringLiteral("mod+ctrl+A") << int(BindModifier::Mod | BindModifier::Ctrl);
    QTest::newRow("combined") << QStringLiteral("Mod+Ctrl+Shift+A") << int(BindModifier::Mod | BindModifier::Ctrl | BindModifier::Shift);
}

void TestConfigBinds::parsesModifiers()
{
    QFETCH(QString, key);
    QFETCH(int, flags);
    const Bind bind = firstBind(key + QStringLiteral(" { close-window; }"));
    QCOMPARE(int(bind.keyModifiers.toInt()), flags);
}

void TestConfigBinds::parsesMouseTriggers_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<int>("button");

    QTest::newRow("left") << QStringLiteral("MouseLeft") << int(MouseButton::Left);
    QTest::newRow("right") << QStringLiteral("MouseRight") << int(MouseButton::Right);
    QTest::newRow("middle") << QStringLiteral("MouseMiddle") << int(MouseButton::Middle);
    QTest::newRow("back") << QStringLiteral("MouseBack") << int(MouseButton::Back);
    QTest::newRow("forward") << QStringLiteral("MouseForward") << int(MouseButton::Forward);
    QTest::newRow("case insensitive") << QStringLiteral("mouseleft") << int(MouseButton::Left);
}

void TestConfigBinds::parsesMouseTriggers()
{
    QFETCH(QString, key);
    QFETCH(int, button);
    const Bind bind = firstBind(QStringLiteral("Mod+%1 { close-window; }").arg(key));
    QCOMPARE(bind.trigger, BindTrigger::MouseButton);
    QCOMPARE(int(bind.mouseButton), button);
}

void TestConfigBinds::parsesScrollTriggers_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<int>("trigger");
    QTest::addColumn<int>("direction");

    QTest::newRow("wheel down") << QStringLiteral("WheelScrollDown") << int(BindTrigger::Wheel) << int(ScrollDirection::Down);
    QTest::newRow("wheel up") << QStringLiteral("WheelScrollUp") << int(BindTrigger::Wheel) << int(ScrollDirection::Up);
    QTest::newRow("wheel left") << QStringLiteral("WheelScrollLeft") << int(BindTrigger::Wheel) << int(ScrollDirection::Left);
    QTest::newRow("wheel right") << QStringLiteral("WheelScrollRight") << int(BindTrigger::Wheel) << int(ScrollDirection::Right);
    QTest::newRow("touchpad down") << QStringLiteral("TouchpadScrollDown") << int(BindTrigger::TouchpadScroll)
                                   << int(ScrollDirection::Down);
    QTest::newRow("touchpad up") << QStringLiteral("TouchpadScrollUp") << int(BindTrigger::TouchpadScroll) << int(ScrollDirection::Up);
    QTest::newRow("touchpad left") << QStringLiteral("TouchpadScrollLeft") << int(BindTrigger::TouchpadScroll)
                                   << int(ScrollDirection::Left);
    QTest::newRow("touchpad right") << QStringLiteral("TouchpadScrollRight") << int(BindTrigger::TouchpadScroll)
                                    << int(ScrollDirection::Right);
}

void TestConfigBinds::parsesScrollTriggers()
{
    QFETCH(QString, key);
    QFETCH(int, trigger);
    QFETCH(int, direction);
    const Bind bind = firstBind(QStringLiteral("Mod+%1 { focus-workspace-down; }").arg(key));
    QCOMPARE(int(bind.trigger), trigger);
    QCOMPARE(int(bind.scrollDirection), direction);
}

void TestConfigBinds::parsesBindProperties()
{
    const Bind bind
        = firstBind(QStringLiteral("Mod+T repeat=false cooldown-ms=150 hotkey-overlay-title=\"Terminal\" { spawn \"alacritty\"; }"));
    QCOMPARE(bind.repeat, false);
    QCOMPARE(bind.cooldownMs, std::optional {150});
    QCOMPARE(bind.hotkeyOverlayTitle, std::optional {QStringLiteral("Terminal")});
    QCOMPARE(bind.hideFromHotkeyOverlay, false);

    const Bind plain = firstBind(QStringLiteral("Mod+Q { close-window; }"));
    QCOMPARE(plain.repeat, true);
    QVERIFY(!plain.cooldownMs.has_value());
    QVERIFY(!plain.hotkeyOverlayTitle.has_value());
}

void TestConfigBinds::hotkeyOverlayTitleNullHides()
{
    const Bind bind = firstBind(QStringLiteral("Mod+Q hotkey-overlay-title=null { close-window; }"));
    QCOMPARE(bind.hideFromHotkeyOverlay, true);
    QVERIFY(!bind.hotkeyOverlayTitle.has_value());
}

void TestConfigBinds::resolvesModKey()
{
    const Bind bind = firstBind(QStringLiteral("Mod+Ctrl+A { close-window; }"));
    QCOMPARE(bind.keyModifiers, BindModifiers(BindModifier::Mod | BindModifier::Ctrl));
    QCOMPARE(bind.resolvedModifiers, BindModifiers(BindModifier::Super | BindModifier::Ctrl));
    QVERIFY(bind.modifiers.testFlag(Qt::MetaModifier));
    QVERIFY(bind.modifiers.testFlag(Qt::ControlModifier));
}

void TestConfigBinds::resolvesModKeyFromInputSection()
{
    const Config config = parsed(QStringLiteral(R"(
        binds {
            Mod+A { close-window; }
        }
        input {
            mod-key "Alt"
        }
    )"));
    QCOMPARE(config.input.modKey, QStringLiteral("Alt"));
    QCOMPARE(config.binds.first().resolvedModifiers, BindModifiers(BindModifier::Alt));
    QVERIFY(config.binds.first().modifiers.testFlag(Qt::AltModifier));
}

void TestConfigBinds::capturesActionArgumentsAndProperties()
{
    const Bind spawn = firstBind(QStringLiteral("Mod+B { spawn \"brightnessctl\" \"set\" \"+10%\"; }"));
    QCOMPARE(spawn.action.name, QStringLiteral("spawn"));
    QCOMPARE(spawn.action.arguments, QStringList({QStringLiteral("brightnessctl"), QStringLiteral("set"), QStringLiteral("+10%")}));

    const Bind workspace = firstBind(QStringLiteral("Mod+1 { focus-workspace 1; }"));
    QCOMPARE(workspace.action.arguments, QStringList({QStringLiteral("1")}));

    const Bind named = firstBind(QStringLiteral("Mod+2 { focus-workspace \"chat\"; }"));
    QCOMPARE(named.action.arguments, QStringList({QStringLiteral("chat")}));

    const Bind width = firstBind(QStringLiteral("Mod+Minus { set-column-width \"-10%\"; }"));
    QCOMPARE(width.action.arguments, QStringList({QStringLiteral("-10%")}));

    const Bind move = firstBind(QStringLiteral("Mod+Shift+1 { move-column-to-workspace 1 focus=false; }"));
    QCOMPARE(move.action.properties.size(), 1);
    QCOMPARE(move.action.properties.first().first, QStringLiteral("focus"));
    QCOMPARE(move.action.properties.first().second, QStringLiteral("false"));
}

void TestConfigBinds::rejectsUnknownAction()
{
    QCOMPARE(bindError(QStringLiteral("Mod+A { do-a-barrel-roll; }")).message, QStringLiteral("unknown action `do-a-barrel-roll`"));
}

void TestConfigBinds::rejectsMissingActionArgument()
{
    QVERIFY(bindError(QStringLiteral("Mod+A { focus-workspace; }")).message.contains(QStringLiteral("requires an argument")));
    QVERIFY(bindError(QStringLiteral("Mod+A { spawn-sh; }")).message.contains(QStringLiteral("requires an argument")));
    QVERIFY(bindError(QStringLiteral("Mod+A { close-window 1; }")).message.contains(QStringLiteral("unexpected argument")));
}

void TestConfigBinds::rejectsUnknownActionProperty()
{
    QCOMPARE(bindError(QStringLiteral("Mod+A { close-window bogus=true; }")).message, QStringLiteral("unexpected property `bogus`"));
}

void TestConfigBinds::rejectsMissingAction()
{
    QCOMPARE(bindError(QStringLiteral("Mod+A { }")).message, QStringLiteral("expected an action for this keybind"));
}

void TestConfigBinds::rejectsMultipleActions()
{
    QCOMPARE(bindError(QStringLiteral("Mod+A { close-window; toggle-overview; }")).message,
        QStringLiteral("only one action is allowed per keybind"));
}

void TestConfigBinds::rejectsDuplicateBinds()
{
    QVERIFY(bindError(QStringLiteral("Mod+A { close-window; }\nMod+A { toggle-overview; }"))
            .message.contains(QStringLiteral("duplicate keybind")));
    const Config config = parsed(QStringLiteral("binds {\nMod+A { close-window; }\nMod+Shift+A { toggle-overview; }\n}\n"));
    QCOMPARE(config.binds.size(), 2);
}

void TestConfigBinds::rejectsInvalidModifierAndKey()
{
    QCOMPARE(bindError(QStringLiteral("Hyper+A { close-window; }")).message, QStringLiteral("invalid modifier: Hyper"));
    QCOMPARE(bindError(QStringLiteral("Mod+NotAKey { close-window; }")).message, QStringLiteral("invalid key: NotAKey"));
}

void TestConfigBinds::rejectsRemovedBindOptions()
{
    QVERIFY(bindError(QStringLiteral("Mod+A allow-when-locked=true { spawn-sh \"true\"; }"))
            .message.contains(QStringLiteral("unexpected property")));
    QVERIFY(bindError(QStringLiteral("Mod+A allow-inhibiting=false { close-window; }"))
            .message.contains(QStringLiteral("unexpected property")));
    const QStringList actions {QStringLiteral("quit"), QStringLiteral("suspend"), QStringLiteral("screenshot"),
        QStringLiteral("power-off-monitors"), QStringLiteral("toggle-keyboard-shortcuts-inhibit"), QStringLiteral("toggle-debug-tint"),
        QStringLiteral("stop-cast")};
    for (const QString &name : actions) {
        QVERIFY2(bindError(QStringLiteral("Mod+A { %1; }").arg(name)).message.contains(QStringLiteral("unknown action")), qPrintable(name));
    }
}

QTEST_MAIN(TestConfigBinds)
#include "test_config_binds.moc"
