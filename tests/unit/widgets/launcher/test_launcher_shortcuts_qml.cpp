#include "launcherharness.h"

using LauncherTest::response;

class TestLauncherShortcutsQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    bool openShortcuts(const QVariantList &responses)
    {
        m_harness.respond(responses);
        return m_harness.openHost(false) && goTo(QStringLiteral("shortcuts"))
            && m_harness.waitHandled(QStringLiteral("PATH=\"$HOME/.local/bin:$PATH\" konveyor-cheatsheet --json # "));
    }

    QVariantList cheatsheet() { return {response(QStringLiteral("konveyor-cheatsheet --json"), LauncherTest::fixture("shortcuts.json"))}; }

private Q_SLOTS:
    void categoriesGroupKdeSections()
    {
        QVERIFY(openShortcuts(cheatsheet()));
        QCOMPARE(page(QStringLiteral("chips")).toStringList(),
            QStringList({QStringLiteral("All"), QStringLiteral("Focus"), QStringLiteral("Sizing"), QStringLiteral("KDE"),
                QStringLiteral("Gestures")}));
        QCOMPARE(page(QStringLiteral("shownSections.length")).toInt(), 5);
        eval(QStringLiteral("launcherData.shortcutCategory = 'KDE'"));
        QCOMPARE(page(QStringLiteral("shownSections.map(s => s.name)")).toStringList(),
            QStringList({QStringLiteral("KDE Windows & Desktops"), QStringLiteral("KDE Plasma")}));
        eval(QStringLiteral("launcherData.shortcutCategory = 'Focus'"));
        TRY_COMPARE(page(QStringLiteral("sections.length")).toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.shortcutsError")).toString(), QString());
    }

    void reopeningRereadsTheShortcuts()
    {
        QVERIFY(openShortcuts(cheatsheet()));
        QObject *host = m_harness.view()->parent();
        host->setProperty("open", false);
        TRY_VERIFY(!eval(QStringLiteral("launcher.shown")).toBool());
        host->setProperty("open", true);
        host->setProperty("open", false);
        TRY_VERIFY(!eval(QStringLiteral("launcher.shown")).toBool());
        QCOMPARE(m_harness.commands().filter(QStringLiteral("konveyor-cheatsheet")).size(), 1);
        host->setProperty("requestedPage", QStringLiteral("shortcuts"));
        host->setProperty("open", true);
        QVERIFY(m_harness.waitHandled(QStringLiteral("PATH=\"$HOME/.local/bin:$PATH\" konveyor-cheatsheet --json # "), 2));
    }

    void failuresShowTheLastErrorLine_data()
    {
        QTest::addColumn<int>("exitCode");
        QTest::addColumn<QString>("stdoutText");
        QTest::addColumn<QString>("stderrText");
        QTest::addColumn<QString>("expected");
        QTest::newRow("traceback") << 1 << QString() << QStringLiteral("Traceback (most recent call last):\nKeyError: 'swipe-diagonal'\n")
                                   << QStringLiteral("KeyError: 'swipe-diagonal'");
        QTest::newRow("silent failure") << 3 << QString() << QString() << QStringLiteral("konveyor-cheatsheet exited with code 3");
        QTest::newRow("not json") << 0 << QStringLiteral("oops") << QString() << QStringLiteral("JSON.parse: Parse error");
    }

    void failuresShowTheLastErrorLine()
    {
        QFETCH(int, exitCode);
        QFETCH(QString, stdoutText);
        QFETCH(QString, stderrText);
        QFETCH(QString, expected);
        QVERIFY(openShortcuts({response(QStringLiteral("konveyor-cheatsheet"), stdoutText.toUtf8(), exitCode, stderrText)}));
        QCOMPARE(eval(QStringLiteral("launcherData.shortcutsError")).toString(), expected);
        QCOMPARE(eval(QStringLiteral("launcherData.shortcuts.length")).toInt(), 0);
    }

    void aFocusedShortcutIsSelected()
    {
        QVERIFY(openShortcuts(cheatsheet()));
        TRY_COMPARE(page(QStringLiteral("sections.length")).toInt(), 5);
        eval(QStringLiteral("launcherData.shortcutFocus = 'Show Desktop'"));
        TRY_COMPARE(eval(QStringLiteral("launcherData.shortcutFocus")).toString(), QString());
        QCOMPARE(eval(QStringLiteral("launcher.currentSection().section.name")).toString(), QStringLiteral("KDE Windows & Desktops"));
        QCOMPARE(eval(QStringLiteral("launcher.currentSection().currentIndex")).toInt(), 0);
    }

    void plusKeysShowAsOneCap_data()
    {
        QTest::addColumn<QString>("key");
        QTest::addColumn<QStringList>("caps");
        QTest::newRow("plain") << QStringLiteral("Mod+Shift+H")
                               << QStringList({QStringLiteral("Mod"), QStringLiteral("Shift"), QStringLiteral("H")});
        QTest::newRow("plus") << QStringLiteral("Meta++") << QStringList({QStringLiteral("Meta"), QStringLiteral("+")});
        QTest::newRow("only plus") << QStringLiteral("+") << QStringList {QStringLiteral("+")};
    }

    void plusKeysShowAsOneCap()
    {
        QFETCH(QString, key);
        QFETCH(QStringList, caps);
        QVERIFY(m_harness.host(false));
        const QString create = QStringLiteral(
            "Qt.createQmlObject('import QtQuick; import org.kde.konveyor.components; KeyCaps { keyName: \"%1\" }', launcher)")
                                   .arg(key);
        QCOMPARE(
            eval(create + QStringLiteral(".children.filter(c => c.modelData !== undefined).map(c => c.modelData)")).toStringList(), caps);
    }

    void settingsPageFollowsTheTarget()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("settings")));
        QCOMPARE(page(QStringLiteral("sections.length")).toInt(), 1);
        eval(QStringLiteral("launcherData.settingsTarget = { page: 'gestures', section: '', label: '' }"));
        TRY_COMPARE(eval(QStringLiteral("launcher.currentView().children[0].pageId")).toString(), QStringLiteral("gestures"));
    }

    void controlZUndoesTheLastSettingsEdit()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("settings")));
        TRY_VERIFY(eval(QStringLiteral("field.activeFocus")).toBool());
        const QString store
            = QStringLiteral("Qt.createQmlObject('import QtQuick; import org.kde.konveyor.settings; QtObject { property var store: "
                             "SettingsStore }', launcher).store");
        QVERIFY(eval(store + QStringLiteral(".setValue('layout/gaps', [27], {})")).toBool());
        QCOMPARE(eval(store + QStringLiteral(".scope('layout').gaps")).toInt(), 27);
        QTest::keyClick(m_harness.window(), Qt::Key_Z, Qt::ControlModifier);
        QVERIFY(eval(store + QStringLiteral(".scope('layout').gaps")).toInt() != 27);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherShortcutsQml)
#include "test_launcher_shortcuts_qml.moc"
