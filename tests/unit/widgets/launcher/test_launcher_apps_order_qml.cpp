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

    void hoverHighlightsOnlyTheTileUnderThePointer_data()
    {
        QTest::addColumn<QString>("view");
        QTest::addColumn<QString>("sort");
        QTest::addColumn<bool>("hideOne");
        for (const char *view : {"grid", "list"}) {
            for (const char *sort : {"name", "recent"}) {
                for (const bool hide : {false, true}) {
                    QTest::addRow("%s %s%s", view, sort, hide ? " with a hidden app" : "")
                        << QString::fromLatin1(view) << QString::fromLatin1(sort) << hide;
                }
            }
        }
    }

    void hoverHighlightsOnlyTheTileUnderThePointer()
    {
        QFETCH(QString, view);
        QFETCH(QString, sort);
        QFETCH(bool, hideOne);
        QVERIFY(openApps({{QStringLiteral("appsView"), view}, {QStringLiteral("appsSort"), sort}}));
        eval(QStringLiteral("launcherData.recentRank = ({ 'dev.zed.Zed': 0, 'org.kde.krita': 1 })"));
        QStringList expected
            = sort == QLatin1String("recent") ? firstThen({QStringLiteral("Zed"), QStringLiteral("Krita")}, everyApp) : everyApp;
        if (hideOne) {
            eval(QStringLiteral("launcherData.setHidden('org.kde.dolphin.desktop', true)"));
            expected.removeAll(QStringLiteral("Dolphin"));
        }
        TRY_COMPARE(shownLabels(), expected);
        for (int position = 0; position < expected.size(); ++position) {
            hover(tile(position));
            TRY_COMPARE(grid(QStringLiteral("currentIndex")).toInt(), position);
            QStringList selected;
            const QStringList flags = tiles(QStringLiteral("selected"));
            for (int i = 0; i < flags.size(); ++i) {
                if (flags.at(i) == QLatin1String("true")) {
                    selected.append(expected.value(i));
                }
            }
            QCOMPARE(selected, QStringList {expected.at(position)});
        }
        hover(tile(3));
        TRY_COMPARE(grid(QStringLiteral("currentIndex")).toInt(), 3);
        QTest::keyClick(m_harness.window(), Qt::Key_Right);
        QCOMPARE(grid(QStringLiteral("currentIndex")).toInt(), 4);
        QCOMPARE(tiles(QStringLiteral("selected")).count(QStringLiteral("true")), 1);
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(m_harness.launched(), QStringList {idOf(expected.at(4))});
    }

    void keyboardWalksTheShownOrder_data()
    {
        QTest::addColumn<QString>("sort");
        QTest::newRow("name") << QStringLiteral("name");
        QTest::newRow("recent") << QStringLiteral("recent");
    }

    void keyboardWalksTheShownOrder()
    {
        QFETCH(QString, sort);
        QVERIFY(openApps({{QStringLiteral("appsSort"), sort}}));
        eval(QStringLiteral("launcherData.recentRank = ({ 'org.kde.konsole': 0, 'org.example.mystery': 1 })"));
        const QStringList expected
            = sort == QLatin1String("recent") ? firstThen({QStringLiteral("Konsole"), QStringLiteral("Mystery Tool")}, everyApp) : everyApp;
        TRY_COMPARE(shownLabels(), expected);
        eval(QStringLiteral("launcher.resetSelection()"));
        QCOMPARE(grid(QStringLiteral("currentIndex")).toInt(), 0);
        for (int position = 1; position < 6; ++position) {
            QTest::keyClick(m_harness.window(), Qt::Key_Right);
            QCOMPARE(grid(QStringLiteral("currentIndex")).toInt(), position);
            QVERIFY(tile(position)->property("selected").toBool());
        }
        const int columns = grid(QStringLiteral("columns")).toInt();
        QTest::keyClick(m_harness.window(), Qt::Key_Down);
        const int chosen = qMin(5 + columns, int(expected.size() - 1));
        QCOMPARE(grid(QStringLiteral("currentIndex")).toInt(), chosen);
        QCOMPARE(tiles(QStringLiteral("selected")).count(QStringLiteral("true")), 1);
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(m_harness.launched(), QStringList {idOf(expected.at(chosen))});
    }

    void theContextMenuActsOnTheClickedApp()
    {
        QVERIFY(openApps({{QStringLiteral("appsSort"), QStringLiteral("recent")}}));
        eval(QStringLiteral("launcherData.recentRank = ({ 'org.kde.krita': 0 })"));
        QVERIFY(showCategory(QStringLiteral("Graphics")));
        TRY_COMPARE(shownLabels(),
            QStringList({QStringLiteral("Krita"), QStringLiteral("Blender"), QStringLiteral("GNU Image Manipulation Program")}));
        click(tile(1), Qt::RightButton);
        TRY_VERIFY(eval(QStringLiteral("launcher.menuOpen")).toBool());
        QCOMPARE(grid(QStringLiteral("currentIndex")).toInt(), 1);
        QCOMPARE(
            tiles(QStringLiteral("selected")), QStringList({QStringLiteral("false"), QStringLiteral("true"), QStringLiteral("false")}));
        QVERIFY(eval(QStringLiteral("launcher.currentView().sections[0].itemAtIndex(1).favoriteId")).toString()
            == QLatin1String("org.blender.Blender.desktop"));
        QTest::keyClick(m_harness.window(), Qt::Key_Escape);
        TRY_VERIFY(!eval(QStringLiteral("launcher.menuOpen")).toBool());
        eval(QStringLiteral("launcher.kickerEntries(launcher.currentView().categoryModel, "
                            "launcher.currentView().activeGroup.get(1).model.index, [], 'org.blender.Blender.desktop')[0].run()"));
        TRY_COMPARE(m_harness.launched(), QStringList {QStringLiteral("org.blender.Blender")});
    }

    void pinningFromTheAppsPagePinsTheSelectedApp()
    {
        QVERIFY(openApps({{QStringLiteral("appsSort"), QStringLiteral("recent")}}));
        eval(QStringLiteral("launcherData.recentRank = ({ 'md.obsidian.Obsidian': 0 })"));
        QVERIFY(showCategory(QStringLiteral("Office")));
        TRY_COMPARE(shownLabels(), QStringList({QStringLiteral("Obsidian"), QStringLiteral("LibreOffice Writer")}));
        hover(tile(1));
        TRY_COMPARE(grid(QStringLiteral("currentIndex")).toInt(), 1);
        QTest::keyClick(m_harness.window(), Qt::Key_P, Qt::ControlModifier);
        TRY_VERIFY(eval(QStringLiteral("launcherData.favorites.isFavorite('org.libreoffice.Writer.desktop')")).toBool());
        QVERIFY(!eval(QStringLiteral("launcherData.favorites.isFavorite('md.obsidian.Obsidian.desktop')")).toBool());
    }
};

LAUNCHER_TEST_MAIN(TestLauncherAppsOrderQml)
#include "test_launcher_apps_order_qml.moc"
