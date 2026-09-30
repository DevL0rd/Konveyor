#include "procmonfixture.h"

#include <QPointer>
#include <QQuickWindow>

using Procmon::feed;
using Procmon::runtime;
using Procmon::started;

namespace
{

QByteArray processes(int first, int count, int cpuBase = 0)
{
    QByteArray procs;
    for (int pid = first; pid < first + count; ++pid) {
        procs += (procs.isEmpty() ? "" : ",") + QByteArray("{\"pid\": ") + QByteArray::number(pid) + ", \"ppid\": 1, \"name\": \"worker"
            + QByteArray::number(pid) + "\", \"cpu\": " + QByteArray::number(((pid - first) * 7 + cpuBase) % count + 1) + "}";
    }
    return "{\"ts\": 2, \"ncpu\": 4, \"mem_total\": 1, \"vram_total\": 0, \"procs\": [" + procs + "]}";
}

QList<QQuickItem *> shownRows(PlasmoidHarness &harness)
{
    auto *view = harness.root()->property("rowsView").value<QQuickItem *>();
    return view ? shownDelegates(view) : QList<QQuickItem *>();
}

QString mismatch(PlasmoidHarness &harness)
{
    const QList<QQuickItem *> rows = shownRows(harness);
    if (rows.isEmpty()) {
        return QStringLiteral("no rows shown");
    }
    for (QQuickItem *row : rows) {
        const int index = row->property("index").toInt();
        const int pid = harness.eval(QStringLiteral("rows.get(%1).pid").arg(index)).toInt();
        const QString name = QStringLiteral("worker%1").arg(pid);
        if (row->property("pid").toInt() != pid || !plainTexts(row).contains(name)) {
            return QStringLiteral("row %1 should show %2 but shows %3").arg(index).arg(name, plainTexts(row).join(QLatin1Char('|')));
        }
    }
    return {};
}

QQuickItem *rowFor(PlasmoidHarness &harness, int pid)
{
    const QList<QQuickItem *> rows = shownRows(harness);
    const auto found = std::find_if(rows.cbegin(), rows.cend(), [pid](QQuickItem *row) { return row->property("pid").toInt() == pid; });
    return found == rows.cend() ? nullptr : *found;
}

QString commandFor(int pid)
{
    return QStringLiteral("tr '\\0' ' ' < /proc/%1/cmdline").arg(pid);
}

void click(QQuickItem *row, Qt::MouseButton button)
{
    clickAt(row, button, QPointF(row->width() / 2, row->property("rowHeight").toReal() / 2));
}

QList<int> openPids(PlasmoidHarness &harness)
{
    QList<int> pids;
    const QList<QQuickItem *> rows = shownRows(harness);
    for (QQuickItem *row : rows) {
        if (row->property("open").toBool()) {
            pids.append(row->property("pid").toInt());
        }
    }
    return pids;
}

bool settled(PlasmoidHarness &harness)
{
    return QTest::qWaitFor([&] { return mismatch(harness).isEmpty(); });
}

bool scrolledAway(PlasmoidHarness &harness, const QPointer<QQuickItem> &delegate, int pid)
{
    for (int step = 0; step < 150; step += 5) {
        harness.eval(QStringLiteral("rowsView.positionViewAtIndex(%1, ListView.Beginning)").arg(step));
        if (delegate && delegate->property("pid").toInt() != pid) {
            return settled(harness);
        }
    }
    return false;
}

}

class TestPlasmoidProcmonReuse : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(runtime() + QStringLiteral("/data.json")); }

    void rowsShowTheirOwnProcessThroughEveryChange()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        const auto first = [&] { return harness->eval(QStringLiteral("rows.count ? rows.get(0).pid : 0")).toInt(); };
        const auto count = [&] { return harness->eval(QStringLiteral("rows.count")).toInt(); };
        QVERIFY(feed(*harness, processes(1000, 150)));
        QTRY_COMPARE(count(), 150);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("headerSort('name')"));
        QTRY_COMPARE(first(), 1000);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->root()->setProperty("searchText", QStringLiteral("worker11"));
        QTRY_COMPARE(count(), 50);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->root()->setProperty("searchText", QString());
        QTRY_COMPARE(count(), 150);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("headerSort('name')"));
        QTRY_COMPARE(first(), 1149);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("rowsView.contentY = rowsView.contentHeight / 2"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(feed(*harness, processes(1040, 150)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.get(rows.count - 1).pid")).toInt(), 1150);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("headerSort('cpu')"));
        QTRY_COMPARE(first(), 1147);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->setConfig(QStringLiteral("sortDescending"), false);
        QTRY_COMPARE(first(), 1040);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(feed(*harness, processes(1020, 120, 5)));
        QTRY_COMPARE(count(), 120);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void commandLineBelongsToTheOpenProcess()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, processes(1000, 150)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        const int first = harness->eval(QStringLiteral("rows.get(3).pid")).toInt();
        QPointer<QQuickItem> delegate = rowFor(*harness, first);
        QVERIFY(delegate);
        harness->root()->setProperty("expandedPid", first);
        QVERIFY(harness->reply(commandFor(first), QStringLiteral("first --flag")));
        QTRY_VERIFY(visibleTexts(delegate).contains(QStringLiteral("first --flag")));
        harness->root()->setProperty("expandedPid", 0);
        QVERIFY(scrolledAway(*harness, delegate, first));
        const int second = delegate->property("pid").toInt();
        harness->root()->setProperty("expandedPid", second);
        QTRY_VERIFY(delegate->property("open").toBool());
        QTRY_VERIFY(!harness->command(commandFor(second)).isEmpty());
        QVERIFY2(
            !visibleTexts(delegate).contains(QStringLiteral("first --flag")), qPrintable(visibleTexts(delegate).join(QLatin1Char('|'))));
        QVERIFY(harness->reply(commandFor(second), QStringLiteral("second --flag")));
        QTRY_VERIFY(visibleTexts(delegate).contains(QStringLiteral("second --flag")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void lateCommandLineRepliesGoToTheirOwnProcess()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, processes(1000, 150)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        const int first = harness->eval(QStringLiteral("rows.get(3).pid")).toInt();
        QPointer<QQuickItem> delegate = rowFor(*harness, first);
        QVERIFY(delegate);
        harness->root()->setProperty("expandedPid", first);
        QTRY_VERIFY(!harness->command(commandFor(first)).isEmpty());
        QVERIFY(scrolledAway(*harness, delegate, first));
        const int second = delegate->property("pid").toInt();
        harness->root()->setProperty("expandedPid", second);
        QVERIFY(harness->reply(commandFor(second), QStringLiteral("second --flag")));
        QVERIFY(harness->reply(commandFor(first), QStringLiteral("first --flag")));
        QTRY_VERIFY(visibleTexts(delegate).contains(QStringLiteral("second --flag")));
        QVERIFY2(
            !visibleTexts(delegate).contains(QStringLiteral("first --flag")), qPrintable(visibleTexts(delegate).join(QLatin1Char('|'))));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void menuActsOnTheRightClickedProcessAfterTheListChanges()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, processes(1000, 150)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        const int target = harness->eval(QStringLiteral("rows.get(4).pid")).toInt();
        click(rowFor(*harness, target), Qt::RightButton);
        QVERIFY(feed(*harness, processes(1000, 150, 9)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(harness->eval(QStringLiteral("rows.get(4).pid")).toInt() != target);
        QObject *kill = withText(harness->findAll("MenuItem"), QStringLiteral("Kill (SIGTERM)"));
        QVERIFY(kill);
        QMetaObject::invokeMethod(kill, "triggered");
        QCOMPARE(harness->command(QStringLiteral("kill ")), QStringLiteral("kill -TERM %1").arg(target));
        QMetaObject::invokeMethod(withText(harness->findAll("MenuItem"), QStringLiteral("Copy command line")), "triggered");
        QVERIFY(!harness->command(commandFor(target)).isEmpty());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void openRowFollowsItsProcessThroughSortsAndScrolling()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, processes(1000, 150)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        const int target = harness->eval(QStringLiteral("rows.get(2).pid")).toInt();
        click(rowFor(*harness, target), Qt::LeftButton);
        QCOMPARE(harness->root()->property("expandedPid").toInt(), target);
        QCOMPARE(openPids(*harness), QList<int> {target});
        harness->eval(QStringLiteral("headerSort('name')"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QTRY_VERIFY(harness->eval(QStringLiteral("rows.get(2).pid")).toInt() != target);
        QVERIFY(openPids(*harness).isEmpty() || openPids(*harness) == QList<int> {target});
        harness->root()->setProperty("searchText", QString::number(target));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QTRY_COMPARE(openPids(*harness), QList<int> {target});
        harness->root()->setProperty("searchText", QString());
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 150);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        for (int step = 0; step < 150; step += 10) {
            harness->eval(QStringLiteral("rowsView.positionViewAtIndex(%1, ListView.Beginning)").arg(step));
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
            const QList<int> open = openPids(*harness);
            QVERIFY2(open.isEmpty() || open == QList<int> {target}, qPrintable(QString::number(open.value(0))));
        }
        const int last
            = harness
                  ->eval(QStringLiteral("rows.get(rows.count - 1).pid === %1 ? rows.get(rows.count - 2).pid : rows.get(rows.count - 1).pid")
                          .arg(target))
                  .toInt();
        harness->eval(QStringLiteral("rowsView.positionViewAtEnd()"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QQuickItem *lastRow = rowFor(*harness, last);
        QVERIFY(lastRow);
        click(lastRow, Qt::LeftButton);
        QCOMPARE(harness->root()->property("expandedPid").toInt(), last);
        QTRY_COMPARE(openPids(*harness), QList<int> {last});
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void hoverLightsOnlyTheRowUnderThePointer()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, processes(1000, 150)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QQuickItem *hovered = rowFor(*harness, harness->eval(QStringLiteral("rows.get(5).pid")).toInt());
        const QPoint pointer = hovered->mapToScene(QPointF(hovered->width() / 2, hovered->property("rowHeight").toReal() / 2)).toPoint();
        QTest::mouseMove(harness->scene()->window(), pointer);
        for (int step = 0; step < 150; step += 10) {
            harness->eval(QStringLiteral("rowsView.positionViewAtIndex(%1, ListView.Beginning)").arg(step));
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
            QTest::mouseMove(harness->scene()->window(), pointer);
            QStringList lit;
            const QList<QQuickItem *> rows = shownRows(*harness);
            for (QQuickItem *row : rows) {
                QObject *area = findByType(row, "QQuickMouseArea").value(0);
                const bool under = row->contains(row->mapFromScene(pointer));
                if (area->property("containsMouse").toBool() != under) {
                    lit.append(QString::number(row->property("pid").toInt()));
                }
            }
            QVERIFY2(lit.isEmpty(), qPrintable(lit.join(QLatin1Char(','))));
        }
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void overlayListUsesThePanelRows_data()
    {
        QTest::addColumn<QString>("file");
        QTest::newRow("rows") << QStringLiteral("ProcessRow.qml");
        QTest::newRow("list") << QStringLiteral("FullView.qml");
    }

    void overlayListUsesThePanelRows()
    {
        QFETCH(QString, file);
        const QString plasmoids = PlasmoidHarness::widgetsDir() + QStringLiteral("/process-monitor/plasmoids/");
        QFile panel(plasmoids + QStringLiteral("org.devl0rd.procmon.panel/contents/ui/") + file);
        QFile overlay(plasmoids + QStringLiteral("org.devl0rd.procmon.overlay/contents/ui/") + file);
        QVERIFY(panel.open(QIODevice::ReadOnly) && overlay.open(QIODevice::ReadOnly));
        QCOMPARE(overlay.readAll(), panel.readAll());
    }
};

QTEST_MAIN(TestPlasmoidProcmonReuse)

#include "test_plasmoid_procmon_reuse.moc"
