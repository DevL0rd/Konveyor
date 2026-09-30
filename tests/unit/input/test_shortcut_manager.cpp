#include "../dbus/fakekglobalaccel.h"
#include "config/loader.h"
#include "input/shortcutmanager.h"

#include <QAction>
#include <QTest>

#include <xkbcommon/xkbcommon.h>

#include <memory>

using Konveyor::ShortcutManager;
using Konveyor::Config::BindModifier;
using Konveyor::Config::BindModifiers;
using Konveyor::Config::BindTrigger;
using Konveyor::Config::MouseButton;
using Konveyor::Config::ScrollDirection;
using Konveyor::Test::FakeKGlobalAccel;
using Konveyor::Test::PrivateSession;

namespace
{

constexpr quint32 keycodeH = 43;
constexpr quint32 keycodeJ = 44;
constexpr quint32 keycodeL = 46;
constexpr quint32 keycodeY = 29;
constexpr quint32 keycodeZ = 52;
const QStringList overview {QStringLiteral("kwin"), QStringLiteral("Overview"), QStringLiteral("KWin"), QStringLiteral("Toggle Overview")};

QList<Konveyor::Config::Bind> binds(const QString &body, const QString &input = QString())
{
    const auto loaded
        = Konveyor::Config::loadString(input + QStringLiteral("binds {\n") + body + QStringLiteral("\n}\n"), QStringLiteral("config.kdl"));
    if (!loaded) {
        qWarning() << loaded.error().toString();
        return {};
    }
    return loaded->config.binds;
}

}

class TestShortcutManager : public QObject
{
    Q_OBJECT

public:
    static void initMain()
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/konveyor-test-bus");
    }

private:
    std::unique_ptr<PrivateSession> m_session;
    std::unique_ptr<FakeKGlobalAccel> m_fake;
    xkb_context *m_context = nullptr;
    QStringList m_fired;

    std::unique_ptr<ShortcutManager> manager()
    {
        m_fired.clear();
        return std::make_unique<ShortcutManager>([this](const Konveyor::Config::Bind &bind) {
            m_fired.append(Konveyor::Config::bindKeyLabel(bind) + QLatin1Char(' ') + bind.action.name);
        });
    }

    xkb_keymap *keymap(const char *layout)
    {
        const xkb_rule_names names {"evdev", "pc105", layout, "", ""};
        return xkb_keymap_new_from_names(m_context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    }

    QList<QKeySequence> keys(const QString &action) const { return m_fake->keys(QStringLiteral("konveyor"), action); }

private Q_SLOTS:
    void initTestCase()
    {
        m_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        QVERIFY(m_context);
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
        m_fake = std::make_unique<FakeKGlobalAccel>();
        QVERIFY(m_fake->start(m_session->address()));
    }

    void cleanupTestCase()
    {
        xkb_context_unref(m_context);
        m_fake.reset();
        m_session.reset();
    }

    void registersEveryKeyBindWithKde()
    {
        auto shortcuts = manager();
        shortcuts->setBinds(binds(QStringLiteral("Mod+H { focus-column-left; }\n"
                                                 "Mod+Shift+1 hotkey-overlay-title=\"Carry\" { move-column-to-workspace 1; }\n"
                                                 "Print { spawn \"true\"; }\n"
                                                 "Mod+L repeat=false { focus-column-right; }\n"
                                                 "Mod+WheelScrollDown { focus-workspace-down; }\n"
                                                 "Mod+MouseMiddle { focus-workspace-up; }")));
        FakeKGlobalAccel::settle();
        QCOMPARE(keys(QStringLiteral("konveyor-Super+H")), QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+H"))});
        QCOMPARE(keys(QStringLiteral("konveyor-Super+Shift+1")),
            (QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+Shift+1")), QKeySequence(QStringLiteral("Meta+!"))}));
        QCOMPARE(keys(QStringLiteral("konveyor-Print")), QList<QKeySequence> {QKeySequence(Qt::Key_Print)});
        QVERIFY(keys(QStringLiteral("konveyor-Super+WheelScrollDown")).isEmpty());
        QVERIFY(keys(QStringLiteral("konveyor-Super+MouseMiddle")).isEmpty());
        const auto *carry = shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+Shift+1"));
        QVERIFY(carry);
        QCOMPARE(carry->text(), QStringLiteral("Carry"));
        const auto *left = shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+H"));
        QVERIFY(left);
        QCOMPARE(left->text(), QStringLiteral("Konveyor: focus-column-left"));
        QVERIFY(left->autoRepeat());
        QVERIFY(!shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+L"))->autoRepeat());
        QCOMPARE(shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Print"))->text(), QStringLiteral("Konveyor: spawn true"));
        QCOMPARE(shortcuts->binds().size(), 6);
    }

    void leavesIsoLevelBindsOutOfKde()
    {
        auto shortcuts = manager();
        shortcuts->setBinds(
            binds(QStringLiteral("Mod+H { focus-column-left; }\nISO_Level5_Shift+K { focus-window-up; }\nSuper+J { focus-window-down; }"),
                QStringLiteral("input { mod-key \"ISO_Level3_Shift\"; }\n")));
        FakeKGlobalAccel::settle();
        QVERIFY(keys(QStringLiteral("konveyor-ISO_Level3_Shift+H")).isEmpty());
        QVERIFY(keys(QStringLiteral("konveyor-H")).isEmpty());
        QVERIFY(keys(QStringLiteral("konveyor-ISO_Level5_Shift+K")).isEmpty());
        QCOMPARE(keys(QStringLiteral("konveyor-Super+J")), QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+J"))});
        QCOMPARE(shortcuts->findChildren<QAction *>().size(), 1);
        xkb_keymap *map = keymap("us");
        QVERIFY(shortcuts->triggerKeyPosition(keycodeH, BindModifier::IsoLevel3Shift, false, map, 0));
        QVERIFY(!shortcuts->triggerKeyPosition(keycodeH, BindModifiers(), false, map, 0));
        QVERIFY(shortcuts->triggerKeyPosition(45, BindModifier::IsoLevel5Shift, false, map, 0));
        xkb_keymap_unref(map);
        QCOMPARE(m_fired,
            (QStringList {QStringLiteral("ISO_Level3_Shift+H focus-column-left"), QStringLiteral("ISO_Level5_Shift+K focus-window-up")}));
    }

    void aKdeShortcutRunsTheBind()
    {
        auto shortcuts = manager();
        shortcuts->setBinds(binds(QStringLiteral("Mod+H { focus-column-left; }")));
        shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+H"))->trigger();
        QCOMPARE(m_fired, QStringList {QStringLiteral("Super+H focus-column-left")});
    }

    void reloadingDropsBindsThatAreGone()
    {
        auto shortcuts = manager();
        shortcuts->setBinds(binds(QStringLiteral("Mod+H { focus-column-left; }\nMod+J { focus-window-down; }")));
        QVERIFY(shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+J")));
        shortcuts->setBinds(binds(QStringLiteral("Mod+H { focus-column-left; }")));
        QVERIFY(!shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+J")));
        QCOMPARE(shortcuts->findChildren<QAction *>().size(), 1);
    }

    void takesOverAKdeShortcutAndGivesItBack()
    {
        m_fake->add(overview, {QKeySequence(QStringLiteral("Meta+W"))});
        {
            auto shortcuts = manager();
            shortcuts->setBinds(binds(QStringLiteral("Mod+W { toggle-overview; }")));
            FakeKGlobalAccel::settle();
            QVERIFY(m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Overview")).isEmpty());
            shortcuts->setBinds(binds(QStringLiteral("Mod+H { focus-column-left; }")));
            FakeKGlobalAccel::settle();
            QCOMPARE(m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Overview")),
                QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+W"))});
            shortcuts->setBinds(binds(QStringLiteral("Mod+W { toggle-overview; }")));
            FakeKGlobalAccel::settle();
            QVERIFY(m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Overview")).isEmpty());
        }
        FakeKGlobalAccel::settle();
        QCOMPARE(
            m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Overview")), QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+W"))});
    }

    void pointerBindsMatchExactly_data()
    {
        QTest::addColumn<int>("trigger");
        QTest::addColumn<int>("modifiers");
        QTest::addColumn<int>("button");
        QTest::addColumn<int>("direction");
        QTest::addColumn<QString>("fired");
        const int meta = int(BindModifier::Super);
        const int metaShift = int(BindModifier::Super) | int(BindModifier::Shift);
        const int level3 = int(BindModifier::IsoLevel3Shift);
        const int wheel = int(BindTrigger::Wheel);
        const int touchpad = int(BindTrigger::TouchpadScroll);
        const int mouse = int(BindTrigger::MouseButton);
        const int down = int(ScrollDirection::Down);
        const int up = int(ScrollDirection::Up);
        const int left = int(MouseButton::Left);
        const int middle = int(MouseButton::Middle);
        QTest::newRow("wheel down") << wheel << meta << left << down << QStringLiteral("Super+WheelScrollDown focus-workspace-down");
        QTest::newRow("wheel up has no bind") << wheel << meta << left << up << QString();
        QTest::newRow("shift wheel down") << wheel << metaShift << left << down
                                          << QStringLiteral("Super+Shift+WheelScrollDown focus-column-right");
        QTest::newRow("extra modifier") << wheel << int(metaShift | int(BindModifier::Ctrl)) << left << down << QString();
        QTest::newRow("missing modifier") << wheel << int(BindModifier::Shift) << left << down << QString();
        QTest::newRow("touchpad is not the wheel") << touchpad << metaShift << left << down << QString();
        QTest::newRow("touchpad down") << touchpad << meta << left << down << QStringLiteral("Super+TouchpadScrollDown focus-column-left");
        QTest::newRow("middle button") << mouse << meta << middle << down << QStringLiteral("Super+MouseMiddle focus-workspace-up");
        QTest::newRow("middle button ignores direction")
            << mouse << meta << middle << up << QStringLiteral("Super+MouseMiddle focus-workspace-up");
        QTest::newRow("left button has no bind") << mouse << meta << left << down << QString();
        QTest::newRow("level3 wheel") << wheel << level3 << left << down
                                      << QStringLiteral("ISO_Level3_Shift+WheelScrollDown focus-column-first");
        QTest::newRow("level3 is not meta") << wheel << int(level3 | meta) << left << down << QString();
        QTest::newRow("plain back button") << mouse << 0 << int(MouseButton::Back) << down
                                           << QStringLiteral("MouseBack focus-workspace-previous");
        QTest::newRow("back with meta has no bind") << mouse << meta << int(MouseButton::Back) << down << QString();
        QTest::newRow("forward with shift") << mouse << metaShift << int(MouseButton::Forward) << down
                                            << QStringLiteral("Super+Shift+MouseForward focus-column-last");
    }

    void pointerBindsMatchExactly()
    {
        QFETCH(int, trigger);
        QFETCH(int, modifiers);
        QFETCH(int, button);
        QFETCH(int, direction);
        QFETCH(QString, fired);
        auto shortcuts = manager();
        shortcuts->setBinds(binds(QStringLiteral("Mod+WheelScrollDown { focus-workspace-down; }\n"
                                                 "Mod+Shift+WheelScrollDown { focus-column-right; }\n"
                                                 "Mod+TouchpadScrollDown { focus-column-left; }\n"
                                                 "Mod+MouseMiddle { focus-workspace-up; }\n"
                                                 "Mod+Shift+MouseForward { focus-column-last; }\n"
                                                 "ISO_Level3_Shift+WheelScrollDown { focus-column-first; }\n"
                                                 "MouseBack { focus-workspace-previous; }")));
        const bool matched = shortcuts->triggerPointerBind(static_cast<BindTrigger>(trigger), BindModifiers::fromInt(modifiers),
            static_cast<MouseButton>(button), static_cast<ScrollDirection>(direction));
        QCOMPARE(matched, !fired.isEmpty());
        QCOMPARE(m_fired, fired.isEmpty() ? QStringList {} : QStringList {fired});
    }

    void theModKeyDecidesTheModifier()
    {
        auto shortcuts = manager();
        shortcuts->setBinds(
            binds(QStringLiteral("Mod+WheelScrollDown { focus-workspace-down; }"), QStringLiteral("input { mod-key \"Alt\"; }\n")));
        QVERIFY(!shortcuts->triggerPointerBind(BindTrigger::Wheel, BindModifier::Super, MouseButton::Left, ScrollDirection::Down));
        QVERIFY(shortcuts->triggerPointerBind(BindTrigger::Wheel, BindModifier::Alt, MouseButton::Left, ScrollDirection::Down));
        QCOMPARE(m_fired, QStringList {QStringLiteral("Alt+WheelScrollDown focus-workspace-down")});
    }

    void aCooldownSwallowsQuickRepeats()
    {
        auto shortcuts = manager();
        shortcuts->setBinds(binds(QStringLiteral("Mod+WheelScrollDown cooldown-ms=60000 { focus-workspace-down; }\n"
                                                 "Mod+WheelScrollUp cooldown-ms=0 { focus-workspace-up; }")));
        for (int turn = 0; turn < 3; ++turn) {
            QVERIFY(shortcuts->triggerPointerBind(BindTrigger::Wheel, BindModifier::Super, MouseButton::Left, ScrollDirection::Down));
            QVERIFY(shortcuts->triggerPointerBind(BindTrigger::Wheel, BindModifier::Super, MouseButton::Left, ScrollDirection::Up));
        }
        QCOMPARE(m_fired.count(QStringLiteral("Super+WheelScrollDown focus-workspace-down")), 1);
        QCOMPARE(m_fired.count(QStringLiteral("Super+WheelScrollUp focus-workspace-up")), 3);
        shortcuts->setBinds(shortcuts->binds());
        QVERIFY(shortcuts->triggerPointerBind(BindTrigger::Wheel, BindModifier::Super, MouseButton::Left, ScrollDirection::Down));
        QCOMPARE(m_fired.count(QStringLiteral("Super+WheelScrollDown focus-workspace-down")), 2);
    }

    void keyPositionsCoverLayoutsWithoutTheSymbol_data()
    {
        QTest::addColumn<QByteArray>("layout");
        QTest::addColumn<quint32>("keycode");
        QTest::addColumn<int>("modifiers");
        QTest::addColumn<bool>("repeat");
        QTest::addColumn<bool>("matched");
        QTest::addColumn<QString>("fired");
        const int meta = int(BindModifier::Super);
        const int level3 = int(BindModifier::IsoLevel3Shift);
        QTest::newRow("ru h") << QByteArrayLiteral("ru") << keycodeH << meta << false << true
                              << QStringLiteral("Super+H focus-column-left");
        QTest::newRow("ru h held without repeat") << QByteArrayLiteral("ru") << keycodeH << meta << true << true << QString();
        QTest::newRow("ru l repeats by default")
            << QByteArrayLiteral("ru") << keycodeL << meta << true << true << QStringLiteral("Super+L focus-column-right");
        QTest::newRow("ru h with shift") << QByteArrayLiteral("ru") << keycodeH << int(Qt::MetaModifier | Qt::ShiftModifier) << false
                                         << false << QString();
        QTest::newRow("ru other key") << QByteArrayLiteral("ru") << quint32(44) << meta << false << false << QString();
        QTest::newRow("us level3 j") << QByteArrayLiteral("us") << keycodeJ << level3 << false << true
                                     << QStringLiteral("ISO_Level3_Shift+J focus-window-down");
        QTest::newRow("de level3 y where de types it")
            << QByteArrayLiteral("de") << keycodeZ << level3 << false << true << QStringLiteral("ISO_Level3_Shift+Y focus-window-up");
        QTest::newRow("de level3 not at the us y") << QByteArrayLiteral("de") << keycodeY << level3 << false << false << QString();
        QTest::newRow("ru level3 j by us position")
            << QByteArrayLiteral("ru") << keycodeJ << level3 << false << true << QStringLiteral("ISO_Level3_Shift+J focus-window-down");
        QTest::newRow("level3 j without level3") << QByteArrayLiteral("us") << keycodeJ << meta << false << false << QString();
        QTest::newRow("us types h itself") << QByteArrayLiteral("us") << keycodeH << meta << false << false << QString();
    }

    void keyPositionsCoverLayoutsWithoutTheSymbol()
    {
        QFETCH(QByteArray, layout);
        QFETCH(quint32, keycode);
        QFETCH(int, modifiers);
        QFETCH(bool, repeat);
        QFETCH(bool, matched);
        QFETCH(QString, fired);
        auto shortcuts = manager();
        shortcuts->setBinds(binds(QStringLiteral("Mod+H repeat=false { focus-column-left; }\n"
                                                 "Mod+L { focus-column-right; }\n"
                                                 "ISO_Level3_Shift+J { focus-window-down; }\n"
                                                 "ISO_Level3_Shift+Y { focus-window-up; }\n"
                                                 "Mod+WheelScrollDown { focus-workspace-down; }")));
        xkb_keymap *map = keymap(layout.constData());
        QVERIFY(map);
        QCOMPARE(shortcuts->triggerKeyPosition(keycode, BindModifiers::fromInt(modifiers), repeat, map, 0), matched);
        QCOMPARE(m_fired, fired.isEmpty() ? QStringList {} : QStringList {fired});
        xkb_keymap_unref(map);
    }
};

QTEST_MAIN(TestShortcutManager)

#include "test_shortcut_manager.moc"
