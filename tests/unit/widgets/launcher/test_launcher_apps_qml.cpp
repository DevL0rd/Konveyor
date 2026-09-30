#include "appsharness.h"

#include <QProcess>

using namespace AppsTest;

class TestLauncherAppsQml : public AppsTest::TestCase
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { m_harness.useSystemKicker(); }

    void everyCategoryListsItsOwnApps_data()
    {
        QTest::addColumn<QString>("view");
        QTest::newRow("grid") << QStringLiteral("grid");
        QTest::newRow("list") << QStringLiteral("list");
    }

    void everyCategoryListsItsOwnApps()
    {
        QFETCH(QString, view);
        QVERIFY(openApps({{QStringLiteral("appsView"), view}}));
        QStringList names;
        for (const auto &category : categories()) {
            names.append(category.first);
        }
        QCOMPARE(chipLabels(), names);
        TRY_COMPARE(shownLabels(), everyApp);
        QList<QPair<QString, QStringList>> tour = categories().mid(1);
        for (auto it = categories().crbegin(); it != categories().crend(); ++it) {
            tour.append(*it);
        }
        for (const auto &category : tour) {
            QVERIFY2(showCategory(category.first), qPrintable(category.first));
            TRY_COMPARE(shownLabels(), category.second);
            QCOMPARE(page(QStringLiteral("activeGroup.count")).toInt(), category.second.size());
            QCOMPARE(m_harness.config()->value(QStringLiteral("appsCategory")).toString(), category.first);
        }
    }

    void clickingATileLaunchesThatApp_data()
    {
        QTest::addColumn<QString>("view");
        QTest::addColumn<QString>("sort");
        for (const char *view : {"grid", "list"}) {
            for (const char *sort : {"name", "recent"}) {
                QTest::addRow("%s %s", view, sort) << QString::fromLatin1(view) << QString::fromLatin1(sort);
            }
        }
    }

    void clickingATileLaunchesThatApp()
    {
        QFETCH(QString, view);
        QFETCH(QString, sort);
        QVERIFY(openApps({{QStringLiteral("appsView"), view}, {QStringLiteral("appsSort"), sort}}));
        eval(QStringLiteral("launcherData.recentRank = ({ 'dev.zed.Zed': 0, 'com.valvesoftware.Steam': 1, 'org.kde.ark': 2 })"));
        for (const auto &category : categories()) {
            QVERIFY(showCategory(category.first));
            const QStringList expected = sort == QLatin1String("recent")
                ? firstThen({QStringLiteral("Zed"), QStringLiteral("Steam"), QStringLiteral("Ark")}, category.second)
                : category.second;
            for (int position = 0; position < expected.size(); ++position) {
                TRY_COMPARE(shownLabels(), expected);
                QVERIFY(tile(position));
                QCOMPARE(launchedAfterClicking(position), QStringList {idOf(tile(position)->property("label").toString())});
                QVERIFY(reopenApps());
            }
        }
    }

    void tabCyclesTheCategories()
    {
        QVERIFY(openApps());
        for (int step = 1; step <= categories().size(); ++step) {
            QTest::keyClick(m_harness.window(), Qt::Key_Tab);
            const auto &category = categories().at(step % categories().size());
            TRY_COMPARE(m_harness.config()->value(QStringLiteral("appsCategory")).toString(), category.first);
            TRY_COMPARE(shownLabels(), category.second);
            TRY_COMPARE(grid(QStringLiteral("currentIndex")).toInt(), 0);
        }
        QTest::keyClick(m_harness.window(), Qt::Key_Backtab);
        TRY_COMPARE(shownLabels(), categories().last().second);
        TRY_COMPARE(grid(QStringLiteral("currentIndex")).toInt(), 0);
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(m_harness.launched(), QStringList {idOf(categories().last().second.first())});
    }

    void appsAddedOrRemovedWhileOpenShowUp()
    {
        QVERIFY(openApps());
        QVERIFY(showCategory(QStringLiteral("Development")));
        TRY_COMPARE(shownLabels(), QStringList({QStringLiteral("Kate"), QStringLiteral("Zed")}));
        const QString zed = m_harness.applicationsDir() + QStringLiteral("/dev.zed.Zed.desktop");
        const QString added = m_harness.applicationsDir() + QStringLiteral("/org.example.newide.desktop");
        QVERIFY(m_harness.writeFile(added,
            "[Desktop Entry]\nType=Application\nName=Brand New IDE\nExec=konveyor-test-launch "
            "org.example.newide\nCategories=Development;\n"));
        QVERIFY(QFile::remove(zed));
        QCOMPARE(QProcess::execute(QStringLiteral("kbuildsycoca6"), {}), 0);
        TRY_COMPARE(shownLabels(), QStringList({QStringLiteral("Brand New IDE"), QStringLiteral("Kate")}));
        QCOMPARE(m_harness.config()->value(QStringLiteral("appsCategory")).toString(), QStringLiteral("Development"));
        QCOMPARE(launchedAfterClicking(0), QStringList {QStringLiteral("org.example.newide")});
        QVERIFY(reopenApps());
        TRY_COMPARE(shownLabels(), QStringList({QStringLiteral("Brand New IDE"), QStringLiteral("Kate")}));
        QCOMPARE(launchedAfterClicking(1), QStringList {QStringLiteral("org.kde.kate")});
        QVERIFY(reopenApps());
        QVERIFY(showCategory(QStringLiteral("All Applications")));
        QStringList withNewIde = everyApp;
        withNewIde.removeAll(QStringLiteral("Zed"));
        withNewIde.insert(3, QStringLiteral("Brand New IDE"));
        TRY_COMPARE(shownLabels(), withNewIde);
        QVERIFY(QFile::remove(added));
        QVERIFY(QFile::copy(LauncherTest::source("tests/unit/widgets/launcher/fixtures/applications/dev.zed.Zed.desktop"), zed));
        QCOMPARE(QProcess::execute(QStringLiteral("kbuildsycoca6"), {}), 0);
        TRY_COMPARE(shownLabels(), everyApp);
        QCOMPARE(launchedAfterClicking(18), QStringList {QStringLiteral("dev.zed.Zed")});
    }

    void searchingAndClearingKeepsTheAppsPageRight()
    {
        QVERIFY(openApps());
        QVERIFY(showCategory(QStringLiteral("Internet")));
        TRY_COMPARE(shownLabels(), categories().at(4).second);
        for (const char key : {'k', 'a', 't'}) {
            QTest::keyClick(m_harness.window(), key);
        }
        TRY_VERIFY(eval(QStringLiteral("launcher.searching")).toBool());
        for (int i = 0; i < 3; ++i) {
            QTest::keyClick(m_harness.window(), Qt::Key_Backspace);
        }
        TRY_VERIFY(!eval(QStringLiteral("launcher.searching")).toBool());
        TRY_COMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("apps"));
        TRY_COMPARE(shownLabels(), categories().at(4).second);
        QVERIFY(showCategory(QStringLiteral("Games")));
        TRY_COMPARE(shownLabels(), categories().at(2).second);
        QCOMPARE(launchedAfterClicking(3), QStringList {QStringLiteral("com.valvesoftware.Steam")});
    }

    void theOnlyMatchingAppIsShownOnceAsTheBestMatch()
    {
        QVERIFY(openApps());
        const QList<QPair<QString, QString>> steps = {{QStringLiteral("b"), QStringLiteral("Blender")},
            {QStringLiteral("blend"), QString()}, {QStringLiteral("bl"), QStringLiteral("Blender")}, {QStringLiteral("blende"), QString()}};
        for (const auto &step : steps) {
            eval(QStringLiteral("launcher.setQuery('%1')").arg(step.first));
            TRY_COMPARE(eval(QStringLiteral("launcher.presentedTerm")).toString(), step.first);
            TRY_COMPARE(eval(QStringLiteral("launcher.currentView().hero.kind")).toString(), QStringLiteral("app"));
            const QString header = QStringLiteral("(v => { const found = []; const walk = item => { for (const child of item.children) { "
                                                  "if (child.title === 'Applications' && child.grid === undefined && child.visible) "
                                                  "found.push(child.trailing); walk(child) } }; "
                                                  "walk(v); return found })(launcher.currentView())");
            if (step.second.isEmpty()) {
                TRY_COMPARE(eval(header).toStringList(), QStringList());
                TRY_COMPARE(eval(QStringLiteral("launcher.currentView().totalResults")).toInt(), 1);
            } else {
                TRY_COMPARE(eval(header).toStringList().size(), 1);
                QVERIFY(eval(header).toStringList().constFirst().toInt() > 0);
            }
        }
    }
};

LAUNCHER_TEST_MAIN(TestLauncherAppsQml)
#include "test_launcher_apps_qml.moc"
