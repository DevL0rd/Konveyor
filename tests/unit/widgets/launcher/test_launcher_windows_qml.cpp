#include "launcherharness.h"

namespace
{

QVariantMap window(const QString &title, const QString &appName, const QString &appId, const QVariantMap &extra = {})
{
    QVariantMap row {{QStringLiteral("display"), title}, {QStringLiteral("AppName"), appName}, {QStringLiteral("AppId"), appId},
        {QStringLiteral("decoration"), QStringLiteral("text-plain")}};
    row.insert(extra);
    return row;
}

const QVariantList openWindows {
    window(QStringLiteral("notes.txt"), QStringLiteral("KWrite"), QStringLiteral("org.kde.kwrite.desktop"),
        {{QStringLiteral("GenericName"), QStringLiteral("Text Editor")}}),
    window(QStringLiteral("main.cpp - Konveyor"), QStringLiteral("Zed"), QStringLiteral("dev.zed.Zed.desktop"),
        {{QStringLiteral("GenericName"), QStringLiteral("Code Editor")}, {QStringLiteral("IsMinimized"), true}}),
    window(QStringLiteral("Starting Firefox"), QStringLiteral("Firefox"), QStringLiteral("firefox.desktop"),
        {{QStringLiteral("IsWindow"), false}}),
};

}

class TestLauncherWindowsQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QObject *fixture() { return m_harness.singleton("org.kde.taskmanager", "TaskFixture"); }

    bool search(const QString &text)
    {
        eval(QStringLiteral("field.text = '%1'; launcher.settleSearch()").arg(text));
        return QTest::qWaitFor(
            [this] { return eval(QStringLiteral("launcher.searchSettled && searchLoader.item !== null")).toBool(); }, 30000);
    }

    QObject *openWithWindows(const QVariantMap &settings = {})
    {
        fixture()->setProperty("windows", openWindows);
        return m_harness.openHost(false, settings);
    }

    QVariant results(const QString &expression) { return eval(QStringLiteral("searchLoader.item.") + expression); }

    QStringList shown()
    {
        return results(QStringLiteral("sections.filter(g => g && g.visible).map(g => {"
                                      "  const out = [];"
                                      "  for (let i = 0; i < g.shownCount; ++i) { const t = g.itemAtIndex(i); out.push(t ? t.label : '') }"
                                      "  return out.join('|') })"))
            .toStringList();
    }

    bool windowRows(int count)
    {
        return QTest::qWaitFor(
            [this, count] {
                return onPage(
                    QStringLiteral(
                        "p.windowSection && p.windowSection.grid.count === %1 && p.windowSection.grid.itemAtIndex(%1 - 1) !== null")
                        .arg(count))
                    .toBool();
            },
            30000);
    }

    QVariant onPage(const QString &expression) { return eval(QStringLiteral("(p => %1)(searchLoader.item)").arg(expression)); }

    QString windowSection()
    {
        return onPage(QStringLiteral("p.windowSection && p.windowSection.visible ? p.windowSection.title : ''")).toString();
    }

private Q_SLOTS:
    void searchingAnOpenAppListsItsWindows_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QStringList>("titles");
        QTest::newRow("app name") << QStringLiteral("kwrite") << QStringList {QStringLiteral("notes.txt")};
        QTest::newRow("app name, other case") << QStringLiteral("ZED") << QStringList {QStringLiteral("main.cpp - Konveyor")};
        QTest::newRow("generic name") << QStringLiteral("text editor") << QStringList {QStringLiteral("notes.txt")};
        QTest::newRow("window title") << QStringLiteral("main.cpp") << QStringList {QStringLiteral("main.cpp - Konveyor")};
        QTest::newRow("app id") << QStringLiteral("dev.zed") << QStringList {QStringLiteral("main.cpp - Konveyor")};
        QTest::newRow("both editors") << QStringLiteral("editor")
                                      << QStringList {QStringLiteral("notes.txt"), QStringLiteral("main.cpp - Konveyor")};
        QTest::newRow("a starting app is no window") << QStringLiteral("firefox") << QStringList();
        QTest::newRow("nothing open matches") << QStringLiteral("dolphin") << QStringList();
    }

    void searchingAnOpenAppListsItsWindows()
    {
        QFETCH(QString, text);
        QFETCH(QStringList, titles);
        QVERIFY(openWithWindows());
        QVERIFY(search(text));
        TRY_COMPARE(results(QStringLiteral("windowMatches.map(w => w.title)")).toStringList(), titles);
        if (titles.isEmpty()) {
            QVERIFY(windowSection().isEmpty());
            return;
        }
        QCOMPARE(windowSection(), QStringLiteral("Open windows"));
        TRY_VERIFY(shown().contains(titles.join(QLatin1Char('|'))));
    }

    void activatingAWindowSwitchesToItAndCloses()
    {
        QObject *host = openWithWindows();
        QVERIFY(host);
        QVERIFY(search(QStringLiteral("zed")));
        QVERIFY(windowRows(1));
        eval(QStringLiteral("launcher.select(searchLoader.item.windowSection.grid, 0); launcher.activateCurrent()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.windows.tasks.activated")).toStringList(),
            QStringList {QStringLiteral("main.cpp - Konveyor")});
    }

    void clickingAWindowSwitchesToIt()
    {
        QObject *host = openWithWindows();
        QVERIFY(host);
        QVERIFY(search(QStringLiteral("notes")));
        QVERIFY(windowRows(1));
        eval(QStringLiteral("searchLoader.item.windowSection.grid.itemAtIndex(0).clicked()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.windows.tasks.activated")).toStringList(), QStringList {QStringLiteral("notes.txt")});
    }

    void aWindowsMenuSwitchesToOrClosesIt()
    {
        QObject *host = openWithWindows();
        QVERIFY(host);
        QVERIFY(search(QStringLiteral("notes")));
        QVERIFY(windowRows(1));
        eval(QStringLiteral("launcher.select(searchLoader.item.windowSection.grid, 0); launcher.menuForCurrent()"));
        QCOMPARE(eval(QStringLiteral("menu.entries.map(e => e.text)")).toStringList(),
            QStringList({QStringLiteral("Switch to window"), QStringLiteral("Close window")}));
        eval(QStringLiteral("menu.entries[1].run()"));
        QCOMPARE(eval(QStringLiteral("launcherData.windows.tasks.closed")).toStringList(), QStringList {QStringLiteral("notes.txt")});
        QCOMPARE(host->property("hideCount").toInt(), 0);
        eval(QStringLiteral("menu.entries[0].run()"));
        QCOMPARE(eval(QStringLiteral("launcherData.windows.tasks.activated")).toStringList(), QStringList {QStringLiteral("notes.txt")});
        QCOMPARE(host->property("hideCount").toInt(), 1);
    }

    void aWindowRowNamesItsAppAndWhetherItIsMinimized()
    {
        QVERIFY(openWithWindows());
        QVERIFY(search(QStringLiteral("editor")));
        QVERIFY(windowRows(2));
        const QString row = QStringLiteral("windowSection.grid.itemAtIndex(%1).");
        QCOMPARE(results(row.arg(0) + QStringLiteral("subtitle")).toString(), QStringLiteral("KWrite"));
        QCOMPARE(results(row.arg(1) + QStringLiteral("subtitle")).toString(), QStringLiteral("Zed · minimized"));
        QCOMPARE(results(row.arg(0) + QStringLiteral("iconSource")).toString(), QStringLiteral("text-plain"));
        QCOMPARE(results(row.arg(0) + QStringLiteral("query")).toString(), QStringLiteral("editor"));
    }

    void windowsFollowOpeningAndClosingWhileSearching()
    {
        QVERIFY(openWithWindows());
        QVERIFY(search(QStringLiteral("editor")));
        TRY_COMPARE(results(QStringLiteral("windowMatches.length")).toInt(), 2);
        fixture()->setProperty("windows", QVariantList {openWindows.at(1)});
        TRY_COMPARE(
            results(QStringLiteral("windowMatches.map(w => w.title)")).toStringList(), QStringList {QStringLiteral("main.cpp - Konveyor")});
        fixture()->setProperty("windows", QVariantList());
        TRY_COMPARE(results(QStringLiteral("windowMatches.length")).toInt(), 0);
        TRY_VERIFY(windowSection().isEmpty());
    }

    void windowsStayOutWhenTurnedOff()
    {
        QVERIFY(openWithWindows({{QStringLiteral("searchWindows"), false}}));
        QVERIFY(search(QStringLiteral("kwrite")));
        QCOMPARE(results(QStringLiteral("windowMatches.length")).toInt(), 0);
        QVERIFY(windowSection().isEmpty());
        m_harness.config()->insert(QStringLiteral("searchWindows"), true);
        TRY_COMPARE(results(QStringLiteral("windowMatches.length")).toInt(), 1);
    }

    void onlyTheAllModeListsWindows_data()
    {
        QTest::addColumn<QString>("text");
        for (const char *prefix : {"a ", "f ", "g ", "s ", "@", "=", ">"}) {
            QTest::newRow(prefix) << QLatin1String(prefix) + QStringLiteral("kwrite");
        }
    }

    void onlyTheAllModeListsWindows()
    {
        QFETCH(QString, text);
        QVERIFY(openWithWindows());
        QVERIFY(search(text));
        QCOMPARE(results(QStringLiteral("windowMatches.length")).toInt(), 0);
    }

    void theWindowRunnerIsNotAskedTwice()
    {
        QVERIFY(openWithWindows());
        QVERIFY(search(QStringLiteral("kwrite")));
        QVERIFY(!eval(QStringLiteral("launcherData.runner.runners")).toStringList().contains(QStringLiteral("windows")));
    }

    void theWindowSourceSeesEveryWindow()
    {
        QVERIFY(openWithWindows());
        QCOMPARE(eval(QStringLiteral("launcherData.windows.tasks.groupMode")).toInt(), 0);
        for (const char *filter : {"filterByVirtualDesktop", "filterByScreen", "filterByActivity", "filterNotMinimized"}) {
            QVERIFY2(!eval(QStringLiteral("launcherData.windows.tasks.") + QLatin1String(filter)).toBool(), filter);
        }
    }

    void windowsFollowTheResultOrder_data()
    {
        QTest::addColumn<QString>("order");
        QTest::addColumn<bool>("windowsFirst");
        QTest::newRow("windows first") << QStringLiteral("windows,apps") << true;
        QTest::newRow("windows after apps") << QStringLiteral("apps,windows") << false;
    }

    void windowsFollowTheResultOrder()
    {
        QFETCH(QString, order);
        QFETCH(bool, windowsFirst);
        m_harness.respondWithLibrary();
        QVERIFY(openWithWindows({{QStringLiteral("searchOrder"), order}}));
        QVERIFY(search(QStringLiteral("o")));
        TRY_VERIFY(results(QStringLiteral("windowSection.grid.count")).toInt() > 0);
        TRY_VERIFY(onPage(QStringLiteral("p.sections.indexOf(p.windowSection.grid) > 1")).toBool());
        const QString apps = QStringLiteral("p.sections.findIndex(g => g && g.visible && g.count > 0 && g !== p.sections[0] "
                                            "&& g !== p.sections[1] && g !== p.windowSection.grid)");
        TRY_VERIFY(onPage(apps + QStringLiteral(" >= 0")).toBool());
        const int windows = onPage(QStringLiteral("p.sections.indexOf(p.windowSection.grid)")).toInt();
        QCOMPARE(windows < onPage(apps).toInt(), windowsFirst);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherWindowsQml)
#include "test_launcher_windows_qml.moc"
