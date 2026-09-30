#include "appsharness.h"

#include <QProcess>

using namespace AppsTest;

class TestLauncherRowsQml : public AppsTest::TestCase
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { m_harness.useSystemKicker(); }

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
