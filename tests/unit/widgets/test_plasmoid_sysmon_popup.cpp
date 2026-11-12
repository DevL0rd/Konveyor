#include "sysmonfixture.h"

#include <QJsonDocument>

using Sysmon::feed;
using Sysmon::hybridSnapshot;
using Sysmon::overlayHost;
using Sysmon::shell;
using Sysmon::snapshot;
using Sysmon::started;

class TestPlasmoidSysmonPopup : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(Sysmon::dataPath()); }

    void searchFiltersTheCards_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<int>("matches");
        QTest::newRow("empty") << QString() << -1;
        QTest::newRow("gpu") << QStringLiteral("gpu") << 1;
        QTest::newRow("temperature") << QStringLiteral("temp") << 2;
        QTest::newRow("core") << QStringLiteral("core 3") << 1;
        QTest::newRow("swap") << QStringLiteral("swap") << 1;
        QTest::newRow("model name") << QStringLiteral("rtx") << 1;
        QTest::newRow("network metric") << QStringLiteral("upload") << 1;
        QTest::newRow("interface") << QStringLiteral("enp5s0") << 1;
        QTest::newRow("nothing") << QStringLiteral("zzz") << 0;
    }

    void searchFiltersTheCards()
    {
        QFETCH(QString, query);
        QFETCH(int, matches);
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        popup->setProperty("searchText", query);
        QCOMPARE(popup->property("matchCount").toInt(), matches);
        QCOMPARE(visibleTexts(harness->scene()).contains(QStringLiteral("No matches")), matches == 0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void collapsesCardsIntoTheConfiguration()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("collapsedCards"), QStringLiteral("gpu")}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        QVERIFY(harness->eval(QStringLiteral("isCollapsed('gpu')"), popup).toBool());
        harness->eval(QStringLiteral("toggleCollapsed('cpu')"), popup);
        QCOMPARE(harness->config(QStringLiteral("collapsedCards")).toString(), QStringLiteral("gpu,cpu"));
        harness->eval(QStringLiteral("toggleCollapsed('gpu')"), popup);
        QCOMPARE(harness->config(QStringLiteral("collapsedCards")).toString(), QStringLiteral("cpu"));
        popup->setProperty("searchText", QStringLiteral("cpu"));
        QVERIFY(!harness->eval(QStringLiteral("isCollapsed('cpu')"), popup).toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsHybridCoreGroups()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, hybridSnapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        const QStringList texts = visibleTexts(harness->scene());
        QVERIFY2(texts.contains(QStringLiteral("2 P · 1 E")), qPrintable(texts.join(QLatin1Char('|'))));
        QVERIFY(texts.contains(QStringLiteral("Performance cores")));
        popup->setProperty("searchText", QStringLiteral("e-core 1"));
        QCOMPARE(harness->eval(QStringLiteral("coreQuery"), popup).toMap(),
            (QVariantMap {{QStringLiteral("group"), QStringLiteral("e")}, {QStringLiteral("index"), 1}}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void coreBarsFollowTheirCore()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        const auto bars = [&] {
            QList<QQuickItem *> found;
            const QList<QQuickItem *> items = visibleItems(harness->scene(), "QQuickItem");
            std::copy_if(items.cbegin(), items.cend(), std::back_inserter(found),
                [](QQuickItem *item) { return item->property("marked").isValid(); });
            return found;
        };
        const auto marked = [&] {
            QStringList out;
            for (QQuickItem *bar : bars()) {
                if (bar->property("marked").toBool()) {
                    out.append(QStringLiteral("%1:%2").arg(bar->property("index").toInt()).arg(bar->property("value").toDouble()));
                }
            }
            return out;
        };
        const auto withCores = [](const QVariantList &cores) {
            QVariantMap map = QJsonDocument::fromJson(snapshot).toVariant().toMap();
            QVariantMap cpu = map.value(QStringLiteral("cpu")).toMap();
            cpu.insert(QStringLiteral("cores"), cores);
            map.insert(QStringLiteral("cpu"), cpu);
            return QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
        };
        popup->setProperty("searchText", QStringLiteral("core 3"));
        QTRY_COMPARE(marked(), QStringList {QStringLiteral("2:90")});
        QVERIFY(feed(*harness, withCores({5, 95, 15, 60, 70, 80})));
        QTRY_COMPARE(bars().size(), 6);
        QCOMPARE(marked(), QStringList {QStringLiteral("2:15")});
        popup->setProperty("searchText", QStringLiteral("core 6"));
        QCOMPARE(marked(), QStringList {QStringLiteral("5:80")});
        QVERIFY(feed(*harness, snapshot));
        QTRY_COMPARE(bars().size(), 4);
        QCOMPARE(marked(), QStringList());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void opensTheSettingsFromThePopup()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        const QList<QObject *> buttons = findByType(popup, "ToolButton");
        for (QObject *button : buttons) {
            if (button->property("text").toString() == QStringLiteral("Configure…")) {
                QMetaObject::invokeMethod(button, "clicked");
            }
        }
        QVERIFY(harness->plasmoid()->action(QStringLiteral("configure")));
        QCOMPARE(harness->plasmoid()->action(QStringLiteral("configure"))->triggered, 1);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void overlayHostStaysHiddenUntilAWindowAsks()
    {
        auto harness = started(Form::Planar, {}, overlayHost);
        QVERIFY(harness);
        QCOMPARE(harness->plasmoid()->status, 6);
        QVERIFY(!harness->root()->property("dataWanted").toBool());
        QObject *root = harness->root();
        QVERIFY(harness->overlay(R"([{"key": "app", "pid": 5, "windowId": 3, "x": 0, "y": 0, "width": 900, "height": 600}])", true));
        QVERIFY(root->property("dataWanted").toBool());
        QVERIFY(feed(*harness, snapshot));
        QVERIFY(harness->overlay("[]", false));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidSysmonPopup)

#include "test_plasmoid_sysmon_popup.moc"
