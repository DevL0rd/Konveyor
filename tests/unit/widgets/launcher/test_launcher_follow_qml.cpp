#include "appsharness.h"

namespace
{

const QString pinned = QStringLiteral("launcher.currentView().sections[0]");
const QString session = QStringLiteral("launcher.currentView().sections[0]");
const QString links = QStringLiteral("launcher.currentView().sections[1]");

}

class TestLauncherFollowQml : public AppsTest::TestCase
{
    Q_OBJECT

private:
    bool pinnedAre(const QStringList &labels)
    {
        return QTest::qWaitFor(
            [this, &labels] { return tilesOf(pinned, QStringLiteral("label")) == labels && staleTiles(pinned).isEmpty(); }, 30000);
    }

    QString lastTriggered() { return eval(QStringLiteral("launcherData.favorites.triggered.slice(-1)[0].favoriteId")).toString(); }

private Q_SLOTS:
    void pinnedTilesFollowTheFavorites()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(pinnedAre({QStringLiteral("Konsole"), QStringLiteral("Dolphin")}));
        eval(QStringLiteral("launcherData.favorites.addFavorite('firefox.desktop')"));
        QVERIFY(pinnedAre({QStringLiteral("Konsole"), QStringLiteral("Dolphin"), QStringLiteral("Firefox")}));
        QCOMPARE(tilesOf(pinned, QStringLiteral("iconSource")),
            QStringList({QStringLiteral("utilities-terminal"), QStringLiteral("system-file-manager"), QStringLiteral("firefox")}));
        eval(QStringLiteral("launcherData.favorites.moveRow(2, 0)"));
        QVERIFY(pinnedAre({QStringLiteral("Firefox"), QStringLiteral("Konsole"), QStringLiteral("Dolphin")}));
        eval(QStringLiteral("launcherData.favorites.removeFavorite('org.kde.konsole.desktop')"));
        QVERIFY(pinnedAre({QStringLiteral("Firefox"), QStringLiteral("Dolphin")}));
        QCOMPARE(hitsOtherTiles(pinned), QString());
        QVERIFY(hoverLightsOnlyThat(pinned, 1));
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(lastTriggered(), QStringLiteral("org.kde.dolphin.desktop"));
    }

    void theSelectedPinStaysSelectedWhenPinsMove()
    {
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("launcherData.favorites.addFavorite('firefox.desktop')"));
        QVERIFY(pinnedAre({QStringLiteral("Konsole"), QStringLiteral("Dolphin"), QStringLiteral("Firefox")}));
        QVERIFY(hoverLightsOnlyThat(pinned, 1));
        QTest::mouseMove(m_harness.window(), QPoint(2, 2));
        eval(QStringLiteral("launcherData.favorites.moveRow(2, 0)"));
        QVERIFY(pinnedAre({QStringLiteral("Firefox"), QStringLiteral("Konsole"), QStringLiteral("Dolphin")}));
        TRY_COMPARE(selectedIn(pinned), QList<int> {2});
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(lastTriggered(), QStringLiteral("org.kde.dolphin.desktop"));
    }

    void sessionTilesRunTheOneUnderThePointer()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("system")));
        TRY_COMPARE(tilesOf(session, QStringLiteral("label")),
            QStringList({QStringLiteral("Lock"), QStringLiteral("Log Out"), QStringLiteral("Sleep"), QStringLiteral("Restart"),
                QStringLiteral("Shut Down")}));
        TRY_COMPARE(tilesOf(links, QStringLiteral("label")).size(), 6);
        TRY_VERIFY(eval(QStringLiteral("launcher.liveSections().some(s => s.sectionActive)")).toBool());
        for (const QString &grid : {session, links}) {
            QCOMPARE(hitsOtherTiles(grid), QString());
            const int shown = eval(grid + QStringLiteral(".shownCount")).toInt();
            for (int position = 0; position < shown; ++position) {
                QVERIFY2(hoverLightsOnlyThat(grid, position), qPrintable(tileOf(grid, position)->property("label").toString()));
            }
        }
        QVERIFY(hoverLightsOnlyThat(session, 2));
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(eval(QStringLiteral("launcherData.system.triggered.slice(-1)[0].favoriteId")).toString(), QStringLiteral("suspend"));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherFollowQml)
#include "test_launcher_follow_qml.moc"
