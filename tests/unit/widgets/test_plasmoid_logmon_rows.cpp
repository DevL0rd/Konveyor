#include "logmonfixture.h"

#include <QPointer>
#include <QTest>

using Logmon::feed;
using Logmon::journalOf;
using Logmon::lineAt;
using Logmon::logPath;
using Logmon::started;

namespace
{

QByteArray numbered(int first, int count)
{
    static const qint64 start = (QDateTime::currentMSecsSinceEpoch() - 60000) * 1000;
    QJsonArray lines;
    for (int number = first; number < first + count; ++number) {
        lines.append(lineAt(
            start + number * 10000, number % 3 ? 6 : 4, QStringLiteral("app%1").arg(number % 5), QStringLiteral("message %1").arg(number)));
    }
    return journalOf(lines);
}

QQuickItem *view(PlasmoidHarness &harness)
{
    return harness.listOf(QStringLiteral("rows"));
}

RowCheck showsItsLine(PlasmoidHarness &harness)
{
    return [&harness](QQuickItem *row, int index) {
        const QVariantMap line
            = harness.eval(QStringLiteral("(r => ({app: r.app, msg: r.msg, expanded: r.expanded}))(rows.get(%1))").arg(index)).toMap();
        const QStringList texts = plainTexts(row);
        const QString app = line.value(QStringLiteral("app")).toString();
        const QString message = line.value(QStringLiteral("msg")).toString();
        if (texts.contains(message) && texts.contains(app) && row->property("expanded") == line.value(QStringLiteral("expanded"))) {
            return QString();
        }
        return QStringLiteral("row %1 should show %2 %3 but shows %4").arg(index).arg(app, message, texts.join(QLatin1Char('|')));
    };
}

QString mismatch(PlasmoidHarness &harness)
{
    return rowMismatch(view(harness), showsItsLine(harness));
}

bool settled(PlasmoidHarness &harness)
{
    return rowsMatch(view(harness), showsItsLine(harness));
}

QQuickItem *rowShowing(PlasmoidHarness &harness, const QString &message)
{
    return delegateWith(view(harness), "msg", message);
}

}

class TestPlasmoidLogmonRows : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(logPath()); }

    void rowsShowTheirOwnLineThroughEveryChange()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("maxRows"), 120}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, numbered(0, 100)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 100);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(feed(*harness, numbered(0, 160)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.get(rows.count - 1).msg")).toString(), QStringLiteral("message 159"));
        QCOMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 120);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("toggleExpand(rows.count - 2)"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("muteApp('app1')"));
        QTRY_VERIFY(!Logmon::rowField(*harness, "app").contains(QStringLiteral("app1")));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(QMetaObject::invokeMethod(view(*harness), "positionViewAtBeginning"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("unmuteApp('app1')"));
        QTRY_VERIFY(Logmon::rowField(*harness, "app").contains(QStringLiteral("app1")));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        harness->eval(QStringLiteral("clearLog()"));
        QVERIFY(feed(*harness, numbered(100, 80)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 20);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void menuActsOnTheRightClickedLineAfterNewLinesArrive()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, numbered(0, 100)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 100);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QTRY_VERIFY(rowShowing(*harness, QStringLiteral("message 90")));
        QPointer<QQuickItem> clicked = rowShowing(*harness, QStringLiteral("message 90"));
        clickAt(clicked, Qt::RightButton);
        QTRY_VERIFY(withText(findByType(clicked, "MenuItem"), QStringLiteral("Ask Claude")));
        const QList<QObject *> menuItems = findByType(clicked, "MenuItem");
        QVERIFY(feed(*harness, numbered(0, 200)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.get(rows.count - 1).msg")).toString(), QStringLiteral("message 199"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        for (int index = 199; clicked && clicked->property("msg").toString() == QLatin1String("message 90") && index >= 0; index -= 5) {
            QMetaObject::invokeMethod(view(*harness), "positionViewAtIndex", Q_ARG(int, index), Q_ARG(int, 1));
        }
        QVERIFY(clicked);
        QVERIFY(clicked->property("msg").toString() != QLatin1String("message 90"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QObject *ask = withText(menuItems, QStringLiteral("Ask Claude"));
        QVERIFY(ask);
        QVERIFY(withText(menuItems, QStringLiteral("Only \"app0\"")));
        QMetaObject::invokeMethod(ask, "triggered");
        QVERIFY2(harness->command(QStringLiteral("konsole")).contains(QLatin1String("message: message 90")),
            qPrintable(harness->command(QStringLiteral("konsole"))));
        QMetaObject::invokeMethod(withText(menuItems, QStringLiteral("Copy line")), "triggered");
        QVERIFY(harness->eval(QStringLiteral("clip.text")).toString().endsWith(QLatin1String("app0[12]: message 90")));
        QMetaObject::invokeMethod(withText(menuItems, QStringLiteral("Mute \"app0\"")), "triggered");
        QCOMPARE(harness->config(QStringLiteral("mutedApps")).toString(), QStringLiteral("app0"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void clickExpandsTheClickedLine()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, numbered(0, 100)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 100);
        QTRY_VERIFY(harness->root()->property("atBottom").toBool() && rowShowing(*harness, QStringLiteral("message 99")));
        for (int index : {60, 5, 95, 30}) {
            QVERIFY(QMetaObject::invokeMethod(view(*harness), "positionViewAtIndex", Q_ARG(int, index), Q_ARG(int, 1)));
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
            const QString message = QStringLiteral("message %1").arg(index);
            QQuickItem *row = rowShowing(*harness, message);
            QVERIFY(row);
            clickAt(row, Qt::LeftButton, QPointF(row->width() / 2, 4));
            QVERIFY(harness->eval(QStringLiteral("rows.get(%1).expanded").arg(index)).toBool());
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
            QTRY_VERIFY(rowShowing(*harness, message));
            row = rowShowing(*harness, message);
            QVERIFY(row->property("expanded").toBool());
            QTRY_VERIFY(visibleTexts(row).contains(QStringLiteral("Level")));
            clickAt(row, Qt::LeftButton, QPointF(row->width() / 2, 4));
            QVERIFY(!harness->eval(QStringLiteral("rows.get(%1).expanded").arg(index)).toBool());
        }
        QCOMPARE(harness
                     ->eval(QStringLiteral(
                         "(function() { let n = 0; for (let i = 0; i < rows.count; ++i) n += rows.get(i).expanded; return n })()"))
                     .toInt(),
            0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void chipsAndTheMutedMenuFollowTheirApps()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, numbered(0, 100)));
        harness->eval(QStringLiteral("refreshSummary()"));
        QTRY_COMPARE(harness->eval(QStringLiteral("topSources.count")).toInt(), 5);
        const auto chip = [&](const QString &app) {
            const QList<QQuickItem *> chips = visibleItems(harness->scene(), "SourceChip");
            const auto found
                = std::find_if(chips.cbegin(), chips.cend(), [&](QQuickItem *item) { return item->property("text").toString() == app; });
            return found == chips.cend() ? nullptr : *found;
        };
        QTRY_VERIFY(chip(QStringLiteral("app3")));
        QCOMPARE(chip(QStringLiteral("app3"))->property("count").toString(), QStringLiteral("20"));
        clickAt(chip(QStringLiteral("app3")), Qt::LeftButton);
        QTRY_COMPARE(harness->root()->property("search").toString(), QStringLiteral("app3"));
        QVERIFY(chip(QStringLiteral("app3"))->property("active").toBool());
        clickAt(chip(QStringLiteral("app3")), Qt::LeftButton);
        QTRY_COMPARE(harness->root()->property("search").toString(), QString());
        harness->eval(QStringLiteral("muteApp('app0'); muteApp('app2'); refreshSummary()"));
        QTRY_VERIFY(withText(harness->findAll("MenuItem"), QStringLiteral("Unmute \"app2\"")));
        QVERIFY(!chip(QStringLiteral("app2")));
        QMetaObject::invokeMethod(withText(harness->findAll("MenuItem"), QStringLiteral("Unmute \"app2\"")), "triggered");
        QCOMPARE(harness->config(QStringLiteral("mutedApps")).toString(), QStringLiteral("app0"));
        harness->eval(QStringLiteral("muteApp('app4')"));
        QTRY_VERIFY(!withText(harness->findAll("MenuItem"), QStringLiteral("Unmute \"app2\"")));
        QMetaObject::invokeMethod(withText(harness->findAll("MenuItem"), QStringLiteral("Unmute \"app4\"")), "triggered");
        QCOMPARE(harness->config(QStringLiteral("mutedApps")).toString(), QStringLiteral("app0"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidLogmonRows)

#include "test_plasmoid_logmon_rows.moc"
