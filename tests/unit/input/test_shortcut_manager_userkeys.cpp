#include "shortcutmanagerfixture.h"

#include <QAction>

using Konveyor::Test::binds;
using Konveyor::Test::FakeKGlobalAccel;
using Konveyor::Test::ShortcutManagerFixture;

class TestShortcutManagerUserKeys : public ShortcutManagerFixture
{
    Q_OBJECT

public:
    static void initMain()
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/konveyor-test-bus");
    }

private Q_SLOTS:
    void initTestCase() { QVERIFY(startFake()); }

    void cleanupTestCase() { stopFake(); }

    void keepsAKeyTheUserGaveToAnotherAction()
    {
        const QKeySequence metaR(QStringLiteral("Meta+R"));
        m_fake->add({QStringLiteral("ksmserver"), QStringLiteral("Reboot Without Confirmation"), QStringLiteral("Session Management"),
                        QStringLiteral("Reboot Without Confirmation")},
            {metaR}, {});
        {
            auto shortcuts = manager();
            shortcuts->setBinds(binds(QStringLiteral("Mod+R { switch-preset-column-width; }\nMod+H { focus-column-left; }")));
            FakeKGlobalAccel::settle();
            QVERIFY(!shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+R")));
            QVERIFY(shortcuts->findChild<QAction *>(QStringLiteral("konveyor-Super+H")));
            QCOMPARE(m_fake->keys(QStringLiteral("ksmserver"), QStringLiteral("Reboot Without Confirmation")), QList<QKeySequence> {metaR});
            QVERIFY(m_fake->keys(QStringLiteral("konveyor"), QStringLiteral("konveyor-Super+R")).isEmpty());
        }
        FakeKGlobalAccel::settle();
        QCOMPARE(m_fake->keys(QStringLiteral("ksmserver"), QStringLiteral("Reboot Without Confirmation")), QList<QKeySequence> {metaR});
    }
};

QTEST_MAIN(TestShortcutManagerUserKeys)

#include "test_shortcut_manager_userkeys.moc"
