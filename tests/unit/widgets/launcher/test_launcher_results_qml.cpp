#include "launcherharness.h"

#include <QClipboard>

using LauncherTest::response;

namespace
{

const QStringList files {QStringLiteral("krunner_placesrunner"), QStringLiteral("krunner_recentdocuments"), QStringLiteral("baloosearch"),
    QStringLiteral("locations")};

const QString walk = QStringLiteral("(function walk(i, out) { out.push(i); for (const c of i.children) walk(c, out); return out })");

}

class TestLauncherResultsQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    bool selectRow(const QString &label)
    {
        const QString find = QStringLiteral("launcher.liveSections().find(g => { for (let i = 0; i < g.shownCount; ++i) { const t = "
                                            "g.itemAtIndex(i); if (t && t.label === '%1') return true } return false })")
                                 .arg(label);
        if (!QTest::qWaitFor([this, &find] { return eval(find + QStringLiteral(" !== undefined")).toBool(); }, 30000)) {
            return false;
        }
        eval(QStringLiteral(
            "(g => { for (let i = 0; i < g.shownCount; ++i) if (g.itemAtIndex(i).label === '%1') launcher.select(g, i) })(%2)")
                .arg(label, find));
        return true;
    }

private Q_SLOTS:
    void eachSearchSettingDropsOnlyItsRunners_data()
    {
        QTest::addColumn<QString>("key");
        QTest::addColumn<QStringList>("runners");
        QTest::newRow("files") << QStringLiteral("searchFiles") << files;
        QTest::newRow("system settings") << QStringLiteral("searchSettings") << QStringList {QStringLiteral("krunner_systemsettings")};
        QTest::newRow("calculator") << QStringLiteral("searchCalculator")
                                    << QStringList {QStringLiteral("calculator"), QStringLiteral("unitconverter")};
        QTest::newRow("commands") << QStringLiteral("searchCommands") << QStringList {QStringLiteral("krunner_shell")};
    }

    void eachSearchSettingDropsOnlyItsRunners()
    {
        QFETCH(QString, key);
        QFETCH(QStringList, runners);
        QVERIFY(m_harness.openHost(false));
        const QStringList all = eval(QStringLiteral("launcherData.allRunners")).toStringList();
        m_harness.config()->insert(key, false);
        QStringList expected = all;
        for (const QString &runner : runners) {
            QVERIFY(expected.removeOne(runner));
        }
        TRY_COMPARE(eval(QStringLiteral("launcherData.allRunners")).toStringList(), expected);
        QVERIFY(search(QStringLiteral("kon")));
        QCOMPARE(eval(QStringLiteral("launcherData.runner.runners")).toStringList(), expected);
    }

    void answersCopyTheResultAndClose()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(search(QStringLiteral("=2+2")));
        QVERIFY(selectRow(QStringLiteral("4")));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("4"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
    }

    void commandsRunThroughTheirRunner()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(search(QStringLiteral(">ls")));
        QVERIFY(selectRow(QStringLiteral("Run ls")));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.runner.modelForRow(0).triggered.length")).toInt(), 1);
    }

    void aSettingResultOpensItsSettingsPage()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(search(QStringLiteral("gaps")));
        TRY_VERIFY(results(QStringLiteral("settingMatches.length")).toInt() > 0);
        const QVariantMap first = results(QStringLiteral("settingMatches[0]")).toMap();
        QVERIFY(selectRow(first.value(QStringLiteral("label")).toString()));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("settings"));
        QCOMPARE(eval(QStringLiteral("field.text")).toString(), QString());
        QCOMPARE(eval(QStringLiteral("launcherData.settingsTarget.page")).toString(), first.value(QStringLiteral("page")).toString());
        QCOMPARE(eval(QStringLiteral("launcherData.settingsTarget.label")).toString(), first.value(QStringLiteral("label")).toString());
    }

    void aShortcutResultOpensTheShortcutsPage()
    {
        m_harness.respond({response(QStringLiteral("konveyor-cheatsheet --json"), LauncherTest::fixture("shortcuts.json"))});
        QVERIFY(m_harness.openHost(false));
        QVERIFY(search(QStringLiteral("show desk")));
        QVERIFY(selectRow(QStringLiteral("Show Desktop")));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("shortcuts"));
        QCOMPARE(eval(QStringLiteral("launcherData.shortcutFocus")).toString(), QStringLiteral("Show Desktop"));
    }

    void nothingFoundOffersPrefixes()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(search(QStringLiteral("zzzz")));
        const QString chips = walk + QStringLiteral("(searchLoader.item, []).filter(c => c.modelData && c.modelData.prefix !== undefined)");
        TRY_COMPARE(eval(chips + QStringLiteral(".map(c => c.modelData.prefix)")).toStringList(),
            QStringList({QStringLiteral("g "), QStringLiteral("a "), QStringLiteral("f "), QStringLiteral("s "), QStringLiteral("@"),
                QStringLiteral("="), QStringLiteral(">")}));
        eval(chips + QStringLiteral("[0].clicked({})"));
        QCOMPARE(eval(QStringLiteral("field.text")).toString(), QStringLiteral("g zzzz"));
        QCOMPARE(eval(QStringLiteral("launcher.mode")).toString(), QStringLiteral("games"));
    }

    void theSearchBoxCountsResultsAndNamesTheMode()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(search(QStringLiteral("g e")));
        TRY_COMPARE(results(QStringLiteral("totalResults")).toInt(), 2);
        const QString count = walk + QStringLiteral("(field.parent, []).find(c => c.text !== undefined && /results?$/.test(c.text))");
        TRY_COMPARE(eval(count + QStringLiteral(".text")).toString(), QStringLiteral("2 results"));
        QVERIFY(eval(count + QStringLiteral(".visible")).toBool());
        QCOMPARE(eval(QStringLiteral("modeLabel.text")).toString(), QStringLiteral("Games"));
        QVERIFY(eval(QStringLiteral("modeLabel.parent.visible")).toBool());
        QVERIFY(search(QStringLiteral("e")));
        QVERIFY(!eval(QStringLiteral("modeLabel.parent.visible")).toBool());
    }

    void searchingCoversThePageAndClearingReturnsToIt()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("files")));
        QVERIFY(search(QStringLiteral("kon")));
        QVERIFY(eval(QStringLiteral("launcher.currentView() === searchLoader.item")).toBool());
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("files"));
        eval(QStringLiteral("field.text = ''"));
        QVERIFY(eval(QStringLiteral("launcher.searchSettled")).toBool());
        TRY_VERIFY(eval(QStringLiteral("launcher.currentView() !== searchLoader.item && launcher.currentView() !== null")).toBool());
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("files"));
    }

    void onlyTheLastQueryReachesTheRunners()
    {
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("field.text = 'k'"));
        eval(QStringLiteral("field.text = 'ko'"));
        QVERIFY(!eval(QStringLiteral("launcher.searchSettled")).toBool());
        TRY_VERIFY(eval(QStringLiteral("launcher.searchSettled")).toBool());
        QCOMPARE(eval(QStringLiteral("launcher.presentedTerm")).toString(), QStringLiteral("ko"));
        QCOMPARE(eval(QStringLiteral("launcherData.runner.query")).toString(), QStringLiteral("ko"));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherResultsQml)
#include "test_launcher_results_qml.moc"
