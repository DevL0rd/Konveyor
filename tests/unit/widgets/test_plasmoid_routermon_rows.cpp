#include "routerfixture.h"

#include <QPointer>

using Router::changed;
using Router::feed;
using Router::runtime;
using Router::started;

namespace
{

QString macOf(int number)
{
    return QStringLiteral("aa:bb:cc:00:01:%1").arg(number, 2, 10, QLatin1Char('0'));
}

QByteArray clientsWith(int count, int trafficShift)
{
    QVariantList leases;
    for (int number = 0; number < count; ++number) {
        leases.append(QVariantMap {{QStringLiteral("mac"), macOf(number)},
            {QStringLiteral("ip"), QStringLiteral("192.168.1.%1").arg(number + 20)},
            {QStringLiteral("name"), QStringLiteral("device%1").arg(number)}, {QStringLiteral("connected"), number % 4 != 0},
            {QStringLiteral("blocked"), number % 7 == 0}, {QStringLiteral("traffic_bps"), ((number * 13 + trafficShift) % count) * 1000}});
    }
    return changed({{QStringLiteral("clients"), QVariantMap {{QStringLiteral("count"), count}, {QStringLiteral("leases"), leases}}}});
}

QQuickItem *view(PlasmoidHarness &harness)
{
    return harness.listOf(QStringLiteral("clients"));
}

RowCheck showsItsClient(PlasmoidHarness &harness)
{
    return [&harness](QQuickItem *row, int index) {
        const QString mac = harness.eval(QStringLiteral("clients.get(%1).mac").arg(index)).toString();
        const QString name = harness.eval(QStringLiteral("clients.get(%1).name").arg(index)).toString();
        if (row->property("mac").toString() == mac && plainTexts(row).contains(name)) {
            return QString();
        }
        return QStringLiteral("client %1 should be %2 but shows %3").arg(index).arg(name, plainTexts(row).join(QLatin1Char('|')));
    };
}

QString mismatch(PlasmoidHarness &harness)
{
    return rowMismatch(view(harness), showsItsClient(harness));
}

bool settled(PlasmoidHarness &harness)
{
    return rowsMatch(view(harness), showsItsClient(harness));
}

QQuickItem *rowFor(PlasmoidHarness &harness, const QString &mac)
{
    return delegateWith(view(harness), "mac", mac);
}

std::unique_ptr<PlasmoidHarness> onClients(int count, const QVariantMap &config = {})
{
    auto harness = started(Form::Planar, config);
    if (!harness || !feed(*harness, clientsWith(count, 0))) {
        return {};
    }
    harness->root()->setProperty("tabKey", QStringLiteral("clients"));
    return QTest::qWaitFor([&] { return view(*harness) && !shownDelegates(view(*harness)).isEmpty(); }) ? std::move(harness) : nullptr;
}

void trigger(PlasmoidHarness &harness, const QString &text)
{
    QMetaObject::invokeMethod(withText(harness.findAll("MenuItem"), text), "triggered");
}

}

class TestPlasmoidRoutermonRows : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(runtime() + QStringLiteral("/data.json")); }

    void rowsShowTheirOwnClientThroughEveryChange()
    {
        auto harness = onClients(40);
        QVERIFY(harness);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        for (const char *order : {"name", "ip", "signal", "traffic"}) {
            harness->setConfig(QStringLiteral("sortBy"), QLatin1String(order));
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        }
        for (int shift = 1; shift < 4; ++shift) {
            QVERIFY(feed(*harness, clientsWith(40 - shift * 5, shift * 11)));
            QTRY_COMPARE(harness->eval(QStringLiteral("clients.count")).toInt(), 40 - shift * 5);
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        }
        for (const char *filter : {"online", "blocked", "wired", "all"}) {
            harness->setConfig(QStringLiteral("clientFilter"), QLatin1String(filter));
            QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        }
        harness->eval(QStringLiteral("togglePin('%1')").arg(macOf(20)));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(QMetaObject::invokeMethod(view(*harness), "positionViewAtEnd"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(feed(*harness, clientsWith(40, 7)));
        QTRY_COMPARE(harness->eval(QStringLiteral("clients.count")).toInt(), 40);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void menuActsOnTheRightClickedClientAfterTheListChanges()
    {
        auto harness = onClients(40, {{QStringLiteral("sortBy"), QStringLiteral("name")}});
        QVERIFY(harness);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QPointer<QQuickItem> clicked = rowFor(*harness, macOf(1));
        QVERIFY(clicked);
        clickAt(clicked, Qt::RightButton);
        QTRY_VERIFY(withText(harness->findAll("MenuItem"), QStringLiteral("Copy MAC")));
        for (int index = 39; clicked && clicked->property("mac").toString() == macOf(1) && index >= 0; index -= 3) {
            QMetaObject::invokeMethod(view(*harness), "positionViewAtIndex", Q_ARG(int, index), Q_ARG(int, 0));
        }
        QVERIFY(!clicked || clicked->property("mac").toString() != macOf(1));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        trigger(*harness, QStringLiteral("Copy MAC"));
        QCOMPARE(harness->root()->property("message").toString(), QStringLiteral("Copied %1").arg(macOf(1)));
        trigger(*harness, QStringLiteral("Block internet"));
        QCOMPARE(harness->command(QStringLiteral("$HOME/.local/bin/routermon-ctl")),
            QStringLiteral("$HOME/.local/bin/routermon-ctl block %1").arg(macOf(1)));
        trigger(*harness, QStringLiteral("Pin to top"));
        QCOMPARE(harness->config(QStringLiteral("pinnedMacs")).toString(), macOf(1));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void menuKeepsItsClientWhenTheClientLeavesTheList()
    {
        auto harness = onClients(12, {{QStringLiteral("sortBy"), QStringLiteral("name")}});
        QVERIFY(harness);
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        clickAt(rowFor(*harness, macOf(4)), Qt::RightButton);
        QTRY_VERIFY(withText(harness->findAll("MenuItem"), QStringLiteral("Copy IP")));
        harness->setConfig(QStringLiteral("clientFilter"), QStringLiteral("online"));
        QVERIFY2(settled(*harness), qPrintable(mismatch(*harness)));
        QVERIFY(!rowFor(*harness, macOf(4)));
        trigger(*harness, QStringLiteral("Copy IP"));
        QCOMPARE(harness->root()->property("message").toString(), QStringLiteral("Copied 192.168.1.24"));
        trigger(*harness, QStringLiteral("Rename…"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void openStationListsStayWithTheirRadio()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->root()->setProperty("tabKey", QStringLiteral("wifi"));
        const auto card = [&](const QString &band) {
            const QList<QQuickItem *> cards = visibleItems(harness->scene(), "PopCard");
            const auto found
                = std::find_if(cards.cbegin(), cards.cend(), [&](QQuickItem *item) { return item->property("title").toString() == band; });
            return found == cards.cend() ? nullptr : *found;
        };
        QTRY_VERIFY(card(QStringLiteral("5GHz-1")));
        QVERIFY(!plainTexts(card(QStringLiteral("5GHz-1"))).contains(QStringLiteral("laptop")));
        auto *header = qobject_cast<QQuickItem *>(
            withText(card(QStringLiteral("5GHz-1"))->findChildren<QObject *>(), QStringLiteral("1 connected device")));
        QVERIFY(header);
        clickAt(header, Qt::LeftButton);
        QTRY_VERIFY(plainTexts(card(QStringLiteral("5GHz-1"))).contains(QStringLiteral("laptop")));
        QVariantMap wifi = QJsonDocument::fromJson(Router::snapshot()).toVariant().toMap().value(QStringLiteral("wifi")).toMap();
        QVariantList radios = wifi.value(QStringLiteral("radios")).toList();
        std::reverse(radios.begin(), radios.end());
        wifi.insert(QStringLiteral("radios"), radios);
        QVERIFY(feed(*harness, changed({{QStringLiteral("wifi"), wifi}})));
        QTRY_COMPARE(harness->eval(QStringLiteral("radios[0].band")).toString(), QStringLiteral("2.4GHz"));
        QTRY_VERIFY(card(QStringLiteral("5GHz-1")) && plainTexts(card(QStringLiteral("5GHz-1"))).contains(QStringLiteral("laptop")));
        QVERIFY(!plainTexts(card(QStringLiteral("2.4GHz"))).contains(QStringLiteral("laptop")));
        harness->root()->setProperty("tabKey", QStringLiteral("clients"));
        harness->root()->setProperty("tabKey", QStringLiteral("wifi"));
        QTRY_VERIFY(card(QStringLiteral("5GHz-1")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidRoutermonRows)

#include "test_plasmoid_routermon_rows.moc"
