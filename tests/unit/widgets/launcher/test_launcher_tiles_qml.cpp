#include "launcherharness.h"

class TestLauncherTilesQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QPoint at(const QString &item, double fx = 0.5, double fy = 0.5)
    {
        const QVariantList point
            = eval(QStringLiteral("(i => { const p = i.mapToItem(null, i.width * %2, i.height * %3); return [p.x, p.y] })(%1)")
                       .arg(item)
                       .arg(fx)
                       .arg(fy))
                  .toList();
        return point.size() == 2 ? QPoint(qRound(point.at(0).toDouble()), qRound(point.at(1).toDouble())) : QPoint();
    }

    void drag(const QPoint &from, const QPoint &to)
    {
        QTest::mousePress(m_harness.window(), Qt::LeftButton, Qt::NoModifier, from);
        for (int step = 1; step <= 10; ++step) {
            QTest::mouseMove(m_harness.window(), from + (to - from) * step / 10);
        }
        QTest::mouseRelease(m_harness.window(), Qt::LeftButton, Qt::NoModifier, to);
    }

    QString pinnedTile(int index) { return QStringLiteral("launcher.currentView().sections[0].itemAtIndex(%1)").arg(index); }

    bool waitForPinnedTiles(int count)
    {
        return QTest::qWaitFor(
            [this, count] {
                return eval(QStringLiteral("launcher.currentView() && launcher.currentView().sections[0].count === %1 && %2 !== null")
                                .arg(count)
                                .arg(pinnedTile(count - 1)))
                    .toBool();
            },
            30000);
    }

private Q_SLOTS:
    void clickingATileLaunchesIt()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(waitForPinnedTiles(2));
        QTest::mouseClick(m_harness.window(), Qt::LeftButton, Qt::NoModifier, at(pinnedTile(1)));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(
            eval(QStringLiteral("launcherData.favorites.triggered[0].favoriteId")).toString(), QStringLiteral("org.kde.dolphin.desktop"));
    }

    void rightClickingATileOpensItsMenu()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(waitForPinnedTiles(2));
        QTest::mouseClick(m_harness.window(), Qt::RightButton, Qt::NoModifier, at(pinnedTile(0)));
        TRY_VERIFY(m_harness.view()->property("menuOpen").toBool());
        QCOMPARE(eval(QStringLiteral("menu.entries.map(e => e.text)[2]")).toString(), QStringLiteral("Unpin from Home"));
        QCOMPARE(eval(QStringLiteral("launcher.currentSection().currentIndex")).toInt(), 0);
    }

    void droppingAPinOnAnotherMakesAFolder()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(waitForPinnedTiles(2));
        drag(at(pinnedTile(0)), at(pinnedTile(1)));
        TRY_COMPARE(eval(QStringLiteral("launcherData.folders.length")).toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.folders[0].apps")).toStringList(),
            QStringList({QStringLiteral("org.kde.dolphin.desktop"), QStringLiteral("org.kde.konsole.desktop")}));
        QCOMPARE(eval(QStringLiteral("launcher.openFolder")).toString(), eval(QStringLiteral("launcherData.folders[0].id")).toString());
        QVERIFY(m_harness.commands().last().startsWith(QStringLiteral("$HOME/.local/bin/portal-games --folder-create ")));
        QCOMPARE(eval(QStringLiteral("launcherData.favorites.triggered.length")).toInt(), 0);
    }

    void droppingAPinOnAnEdgeReorders()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(waitForPinnedTiles(2));
        drag(at(pinnedTile(0)), at(pinnedTile(1), 0.9));
        TRY_COMPARE(eval(QStringLiteral("launcherData.favoriteIds")).toStringList(),
            QStringList({QStringLiteral("org.kde.dolphin.desktop"), QStringLiteral("org.kde.konsole.desktop")}));
        QCOMPARE(eval(QStringLiteral("launcherData.folders.length")).toInt(), 0);
    }

    void draggingAnAppToTheRailPinsItToTheSidebar()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("apps")));
        const QString tile = QStringLiteral("launcher.currentView().sections[0].itemAtIndex(0)");
        TRY_VERIFY(eval(tile + QStringLiteral(" !== null")).toBool());
        const QPoint from = at(tile);
        drag(from, QPoint(at(QStringLiteral("rail")).x(), from.y()));
        TRY_COMPARE(eval(QStringLiteral("launcherData.sidebarPins.map(p => p.id)")).toStringList(),
            QStringList {QStringLiteral("org.kde.dolphin")});
        QVERIFY(eval(QStringLiteral("launcher.sidebarDrag === null")).toBool());
        QCOMPARE(eval(QStringLiteral("launcher.currentView().categoryModel.triggered.length")).toInt(), 0);
    }

    void draggingAnAppElsewhereDoesNothing()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("apps")));
        const QString tile = QStringLiteral("launcher.currentView().sections[0].itemAtIndex(0)");
        TRY_VERIFY(eval(tile + QStringLiteral(" !== null")).toBool());
        drag(at(tile), at(tile, 3.0));
        QVERIFY(eval(QStringLiteral("launcher.sidebarDrag === null")).toBool());
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarPins.length")).toInt(), 0);
        QCOMPARE(eval(QStringLiteral("launcher.currentView().categoryModel.triggered.length")).toInt(), 0);
    }

    void touchHoldOpensTheMenu()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(waitForPinnedTiles(2));
        static QPointingDevice *const touchscreen = QTest::createTouchDevice();
        const QPoint point = at(pinnedTile(0));
        QTest::touchEvent(m_harness.window(), touchscreen).press(0, point);
        QVERIFY(eval(QStringLiteral("launcher.touchMode && launcher.touchDown")).toBool());
        TRY_VERIFY(eval(QStringLiteral("launcher.pendingMenu !== null")).toBool());
        QVERIFY(!m_harness.view()->property("menuOpen").toBool());
        QTest::touchEvent(m_harness.window(), touchscreen).release(0, point);
        TRY_VERIFY(m_harness.view()->property("menuOpen").toBool());
        QCOMPARE(eval(QStringLiteral("menu.entries[0].text")).toString(), QStringLiteral("Open"));
        QCOMPARE(eval(QStringLiteral("launcherData.favorites.triggered.length")).toInt(), 0);
    }

    void recentFilesRowsOpenTheDocument()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(goTo(QStringLiteral("files")));
        const QString row = QStringLiteral("launcher.currentView().sections[1].itemAtIndex(0)");
        TRY_VERIFY(eval(row + QStringLiteral(" !== null")).toBool());
        QTest::mouseClick(m_harness.window(), Qt::LeftButton, Qt::NoModifier, at(row));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.recentDocs.triggered[0].favoriteId")).toString(),
            QStringLiteral("file:///home/user/notes.txt"));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherTilesQml)
#include "test_launcher_tiles_qml.moc"
