#include "launcherharness.h"

class TestLauncherKeysQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QObject *open(const QVariantMap &settings = {})
    {
        QObject *host = m_harness.openHost(false, settings);
        if (host && !QTest::qWaitFor([this] { return eval(QStringLiteral("field.activeFocus")).toBool(); }, 30000)) {
            return nullptr;
        }
        return host;
    }

    void press(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier) { QTest::keyClick(m_harness.window(), Qt::Key(key), modifiers); }

    QString page() { return eval(QStringLiteral("launcher.page")).toString(); }

private Q_SLOTS:
    void escapeClearsTheSearchThenCloses()
    {
        QObject *host = open();
        QVERIFY(host);
        for (const char key : {'k', 'o', 'n'}) {
            QTest::keyClick(m_harness.window(), key);
        }
        QCOMPARE(eval(QStringLiteral("field.text")).toString(), QStringLiteral("kon"));
        press(Qt::Key_Escape);
        QCOMPARE(eval(QStringLiteral("field.text")).toString(), QString());
        QCOMPARE(host->property("hideCount").toInt(), 0);
        press(Qt::Key_Escape);
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QVERIFY(!host->property("open").toBool());
    }

    void altNumbersJumpToTheShownPages_data()
    {
        QTest::addColumn<bool>("games");
        QTest::addColumn<int>("key");
        QTest::addColumn<QString>("expected");
        QTest::newRow("games shown") << true << int(Qt::Key_3) << QStringLiteral("games");
        QTest::newRow("games hidden") << false << int(Qt::Key_3) << QStringLiteral("files");
        QTest::newRow("last page") << true << int(Qt::Key_8) << QStringLiteral("settings");
        QTest::newRow("past the last page") << false << int(Qt::Key_8) << QStringLiteral("home");
    }

    void altNumbersJumpToTheShownPages()
    {
        QFETCH(bool, games);
        QFETCH(int, key);
        QFETCH(QString, expected);
        QVERIFY(open({{QStringLiteral("showGames"), games}}));
        press(key, Qt::AltModifier);
        QCOMPARE(page(), expected);
    }

    void controlShortcutsSwitchPages()
    {
        QVERIFY(open());
        press(Qt::Key_Comma, Qt::ControlModifier);
        QCOMPARE(page(), QStringLiteral("settings"));
        press(Qt::Key_Tab, Qt::ControlModifier);
        QCOMPARE(page(), QStringLiteral("home"));
        press(Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(page(), QStringLiteral("settings"));
    }

    void controlNumbersLaunchPinnedApps()
    {
        QObject *host = open();
        QVERIFY(host);
        press(Qt::Key_9, Qt::ControlModifier);
        QCOMPARE(host->property("hideCount").toInt(), 0);
        press(Qt::Key_2, Qt::ControlModifier);
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.favorites.triggered.map(t => t.favoriteId)")).toStringList(),
            QStringList {QStringLiteral("org.kde.dolphin.desktop")});
    }

    void enterLaunchesTheSelectedPin()
    {
        QObject *host = open();
        QVERIFY(host);
        TRY_COMPARE(eval(QStringLiteral("launcher.currentSection() ? launcher.currentSection().currentIndex : -2")).toInt(), 0);
        press(Qt::Key_Right);
        press(Qt::Key_Return);
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.favorites.triggered.map(t => t.favoriteId)")).toStringList(),
            QStringList {QStringLiteral("org.kde.dolphin.desktop")});
        QVERIFY(m_harness.commands().contains(QStringLiteral("$HOME/.local/bin/portal-games --track-app 'org.kde.dolphin'")));
    }

    void controlPPinsAndShiftPinsToTheSidebar()
    {
        QVERIFY(open());
        TRY_COMPARE(eval(QStringLiteral("launcher.currentSection() ? launcher.currentSection().currentIndex : -2")).toInt(), 0);
        press(Qt::Key_P, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarPins.map(p => p.id)")).toStringList(),
            QStringList {QStringLiteral("org.kde.konsole")});
        press(Qt::Key_P, Qt::ControlModifier);
        QVERIFY(!eval(QStringLiteral("launcherData.favorites.isFavorite('org.kde.konsole.desktop')")).toBool());
    }

    void arrowsMoveBetweenSectionsAndTheRail()
    {
        QVERIFY(open());
        TRY_COMPARE(eval(QStringLiteral("launcher.sectionIndex")).toInt(), 0);
        eval(QStringLiteral("launcherData.sidebarPins = [{ kind: 'app', id: 'firefox', name: 'Firefox' }]"));
        press(Qt::Key_Down);
        QCOMPARE(eval(QStringLiteral("launcher.sectionIndex")).toInt(), 1);
        press(Qt::Key_Up);
        QCOMPARE(eval(QStringLiteral("launcher.sectionIndex")).toInt(), 0);
        press(Qt::Key_Left);
        QCOMPARE(eval(QStringLiteral("launcher.railIndex")).toInt(), 0);
        QVERIFY(!eval(QStringLiteral("launcher.currentSection().sectionActive")).toBool());
        press(Qt::Key_Right);
        QCOMPARE(eval(QStringLiteral("launcher.railIndex")).toInt(), -1);
        QVERIFY(eval(QStringLiteral("launcher.currentSection().sectionActive")).toBool());
    }

    void tabCyclesSections()
    {
        QVERIFY(open());
        TRY_COMPARE(eval(QStringLiteral("launcher.liveSections().length")).toInt(), 3);
        for (const int expected : {1, 2, 0}) {
            press(Qt::Key_Tab);
            QCOMPARE(eval(QStringLiteral("launcher.sectionIndex")).toInt(), expected);
        }
        press(Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(eval(QStringLiteral("launcher.sectionIndex")).toInt(), 2);
    }

    void menuKeyOpensTheMenuForTheSelection()
    {
        QVERIFY(open());
        TRY_COMPARE(eval(QStringLiteral("launcher.currentSection() ? launcher.currentSection().currentIndex : -2")).toInt(), 0);
        press(Qt::Key_Menu);
        TRY_VERIFY(m_harness.view()->property("menuOpen").toBool());
        QCOMPARE(eval(QStringLiteral("menu.entries[0].text")).toString(), QStringLiteral("Open"));
    }

    void holdingAltShowsTheHints()
    {
        QVERIFY(open());
        QTest::keyPress(m_harness.window(), Qt::Key_Alt, Qt::AltModifier);
        QVERIFY(eval(QStringLiteral("launcher.altHeld")).toBool());
        QTest::keyRelease(m_harness.window(), Qt::Key_Alt);
        QVERIFY(!eval(QStringLiteral("launcher.altHeld")).toBool());
    }
};

LAUNCHER_TEST_MAIN(TestLauncherKeysQml)
#include "test_launcher_keys_qml.moc"
