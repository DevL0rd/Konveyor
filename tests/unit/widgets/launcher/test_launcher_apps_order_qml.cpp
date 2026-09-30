#include "appsharness.h"

using namespace AppsTest;

class TestLauncherAppsOrderQml : public AppsTest::TestCase
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { m_harness.useSystemKicker(); }

    void sortingReordersTheTiles()
    {
        QVERIFY(openApps());
        eval(QStringLiteral("launcherData.recentRank = ({ 'dev.zed.Zed': 0, 'com.valvesoftware.Steam': 1 })"));
        eval(QStringLiteral("launcherData.popularRank = ({ 'org.kde.krita': 0, 'org.example.chess': 1, 'org.kde.ark': 2 })"));
        const QStringList recent = firstThen({QStringLiteral("Zed"), QStringLiteral("Steam")}, everyApp);
        const QStringList popular = firstThen({QStringLiteral("Krita"), QStringLiteral("Chess Master"), QStringLiteral("Ark")}, everyApp);
        const QList<QPair<QString, QStringList>> steps
            = {{QStringLiteral("recent"), recent}, {QStringLiteral("name"), everyApp}, {QStringLiteral("popular"), popular},
                {QStringLiteral("recent"), recent}, {QStringLiteral("popular"), popular}, {QStringLiteral("name"), everyApp}};
        for (const auto &step : steps) {
            setSort(step.first);
            TRY_COMPARE(shownLabels(), step.second);
            TRY_COMPARE(page(QStringLiteral("letters.length")).toInt(), step.first == QLatin1String("name") ? 27 : 0);
        }
        setSort(QStringLiteral("popular"));
        TRY_COMPARE(shownLabels(), popular);
        QVERIFY(showCategory(QStringLiteral("Games")));
        TRY_COMPARE(shownLabels(), firstThen({QStringLiteral("Chess Master")}, categories().at(2).second));
        setSort(QStringLiteral("name"));
        TRY_COMPARE(shownLabels(), categories().at(2).second);
        QVERIFY(showCategory(QStringLiteral("All Applications")));
        TRY_COMPARE(shownLabels(), everyApp);
    }

    void theLetterIndexJumpsToTheRightApp()
    {
        QVERIFY(openApps({{QStringLiteral("appsSort"), QStringLiteral("recent")}}));
        eval(QStringLiteral("launcherData.recentRank = ({ 'dev.zed.Zed': 0, 'com.valvesoftware.Steam': 1 })"));
        TRY_COMPARE(shownLabels().value(0), QStringLiteral("Zed"));
        setSort(QStringLiteral("name"));
        TRY_COMPARE(shownLabels(), everyApp);
        for (const char *letter : {"A", "K", "S", "Z"}) {
            const int row = page(QStringLiteral("letters.find(l => l.key === '%1').row").arg(QLatin1String(letter))).toInt();
            QVERIFY(row >= 0);
            page(QStringLiteral("jump(%1)").arg(row));
            QVERIFY(tile(row));
            QCOMPARE(tile(row)->property("label").toString().left(1).toUpper(), QString::fromLatin1(letter));
            QCOMPARE(everyApp.indexOf(tile(row)->property("label").toString()), row);
            QVERIFY(tile(row)->property("selected").toBool());
        }
        QCOMPARE(page(QStringLiteral("letters.find(l => l.key === 'Q').row")).toInt(), -1);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherAppsOrderQml)
#include "test_launcher_apps_order_qml.moc"
