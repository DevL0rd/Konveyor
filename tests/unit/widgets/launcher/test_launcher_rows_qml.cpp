#include "appsharness.h"

#include <QProcess>

using namespace AppsTest;

namespace
{

const QString places = QStringLiteral("launcher.currentView().sections[0]");
const QString appResults = QStringLiteral("launcher.currentView().sections.find(g => g && g.parent && g.parent.title === 'Applications')");

}

class TestLauncherRowsQml : public AppsTest::TestCase
{
    Q_OBJECT

private:
    bool searchApps(const QString &term)
    {
        eval(QStringLiteral("launcher.setQuery('%1')").arg(term));
        return QTest::qWaitFor(
            [this, &term] {
                return eval(QStringLiteral("launcher.presentedTerm")).toString() == term
                    && eval(appResults + QStringLiteral(" !== undefined")).toBool()
                    && tilesOf(appResults, QStringLiteral("label")).size() > 1
                    && eval(QStringLiteral("launcher.liveSections()[0].sectionActive && launcher.liveSections()[0].currentIndex === 0"))
                           .toBool();
            },
            30000);
    }

    bool reopen()
    {
        m_harness.root()->setProperty("open", true);
        return QTest::qWaitFor([this] { return m_harness.view()->property("progress").toDouble() == 1.0; }, 30000);
    }

private Q_SLOTS:
    void initTestCase() { m_harness.useSystemKicker(); }

    void placesHighlightThePlaceUnderThePointer()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("files")));
        TRY_COMPARE(tilesOf(places, QStringLiteral("label")),
            QStringList({QStringLiteral("Home"), QStringLiteral("Trash"), QStringLiteral("Network"), QStringLiteral("Recent Files"),
                QStringLiteral("Recent Locations")}));
        for (int position = 0; position < 5; ++position) {
            QVERIFY2(hoverLightsOnlyThat(places, position), qPrintable(QString::number(position)));
        }
        for (int position = 4; position >= 0; --position) {
            QVERIFY2(hoverLightsOnlyThat(places, position), qPrintable(QString::number(position)));
        }
    }

    void searchedAppsHighlightTheRowUnderThePointer_data()
    {
        QTest::addColumn<QString>("term");
        for (const char *term : {"k", "ka", "kr", "s", "st", "dol"}) {
            QTest::newRow(term) << QString::fromLatin1(term);
        }
    }

    void searchedAppsHighlightTheRowUnderThePointer()
    {
        QFETCH(QString, term);
        QVERIFY(openApps());
        QVERIFY(searchApps(term));
        const QStringList labels = tilesOf(appResults, QStringLiteral("label"));
        QCOMPARE(tilesOf(appResults, QStringLiteral("model.display")), labels);
        QVERIFY(!labels.contains(eval(QStringLiteral("launcher.currentView().sections[0].itemAtIndex(0).label")).toString()));
        for (int position = 0; position < labels.size(); ++position) {
            QVERIFY2(hoverLightsOnlyThat(appResults, position), qPrintable(QString::number(position)));
        }
        for (int position = 0; position < labels.size(); ++position) {
            if (position > 0) {
                QVERIFY(reopen());
                QVERIFY(searchApps(term));
            }
            const QString label = tilesOf(appResults, QStringLiteral("label")).value(position);
            QVERIFY(hoverLightsOnlyThat(appResults, position));
            const qsizetype before = m_harness.launched().size();
            QTest::keyClick(m_harness.window(), Qt::Key_Return);
            TRY_COMPARE(m_harness.launched().size(), before + 1);
            QCOMPARE(m_harness.launched().constLast(), idOf(label));
        }
    }

    void everyResultIsTheTileUnderItsOwnPointer()
    {
        QVERIFY(openApps());
        for (const char *term : {"k", "ka", "kat", "k", "kr", "s", "st", "ste", "s", "dol", "d", "zed", "k"}) {
            eval(QStringLiteral("launcher.setQuery('%1')").arg(QLatin1String(term)));
            TRY_COMPARE(eval(QStringLiteral("launcher.presentedTerm")).toString(), QString::fromLatin1(term));
            TRY_VERIFY(eval(QStringLiteral("launcher.searchSettled && launcher.liveSections().length > 1")).toBool());
            const auto wrong = [this] {
                QStringList found;
                const int count = eval(QStringLiteral("launcher.liveSections().length")).toInt();
                for (int section = 0; section < count; ++section) {
                    const QString grid = QStringLiteral("launcher.liveSections()[%1]").arg(section);
                    for (const QString &problems : {hitsOtherTiles(grid), staleTiles(grid)}) {
                        if (!problems.isEmpty()) {
                            found.append(problems);
                        }
                    }
                }
                return found.join(QStringLiteral("; "));
            };
            QTRY_VERIFY2_WITH_TIMEOUT(wrong().isEmpty(), qPrintable(QStringLiteral("%1: %2").arg(QLatin1String(term), wrong())), 5000);
        }
    }

    void aMenuClosesWhenItsAppsChange()
    {
        QVERIFY(openApps());
        TRY_COMPARE(shownLabels(), everyApp);
        click(tile(18), Qt::RightButton);
        TRY_VERIFY(eval(QStringLiteral("launcher.menuOpen")).toBool());
        QCOMPARE(tile(18)->property("label").toString(), QStringLiteral("Zed"));
        QVERIFY(m_harness.writeFile(m_harness.applicationsDir() + QStringLiteral("/org.example.newide.desktop"),
            "[Desktop Entry]\nType=Application\nName=Brand New IDE\nExec=konveyor-test-launch "
            "org.example.newide\nCategories=Development;\n"));
        QCOMPARE(QProcess::execute(QStringLiteral("kbuildsycoca6"), {}), 0);
        TRY_COMPARE(shownLabels().value(3), QStringLiteral("Brand New IDE"));
        TRY_VERIFY(!eval(QStringLiteral("launcher.menuOpen")).toBool());
        QCOMPARE(tile(19)->property("label").toString(), QStringLiteral("Zed"));
        click(tile(19), Qt::RightButton);
        TRY_VERIFY(eval(QStringLiteral("launcher.menuOpen")).toBool());
        eval(QStringLiteral("menu.entries.find(e => e.text === 'Open').run()"));
        TRY_COMPARE(m_harness.launched(), QStringList {QStringLiteral("dev.zed.Zed")});
        QVERIFY(QFile::remove(m_harness.applicationsDir() + QStringLiteral("/org.example.newide.desktop")));
        QCOMPARE(QProcess::execute(QStringLiteral("kbuildsycoca6"), {}), 0);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherRowsQml)
#include "test_launcher_rows_qml.moc"
