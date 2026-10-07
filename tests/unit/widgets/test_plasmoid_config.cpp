#include "plasmoidharness.h"

#include <QDirIterator>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QTest>

namespace
{

PlasmoidSpec spec(const char *ui, const char *id, QStringList libs = {})
{
    return {QLatin1String(ui), QLatin1String(id), QStringLiteral("configure"), std::move(libs)};
}

struct ConfigPage
{
    PlasmoidSpec plasmoid;
    QString file;
};

ConfigPage page(const char *ui, const char *id, QStringList libs = {}, const char *file = "configGeneral.qml")
{
    return {spec(ui, id, std::move(libs)), QLatin1String(file)};
}

void plasmoidRows()
{
    QTest::addColumn<PlasmoidSpec>("plasmoid");
    QTest::newRow("system monitor") << spec("system-monitor/plasmoids/org.devl0rd.sysmon.panel", "org.devl0rd.sysmon.panel");
    QTest::newRow("process monitor") << spec("process-monitor/plasmoids/org.devl0rd.procmon.panel", "org.devl0rd.procmon.panel");
    QTest::newRow("router monitor") << spec("router-monitor/plasmoids/org.devl0rd.routermon.panel", "org.devl0rd.routermon.panel",
        {QStringLiteral("router-monitor/shared/lib")});
    QTest::newRow("router dns") << spec(
        "router-monitor/plasmoids/org.devl0rd.routermon.panel", "org.devl0rd.routermon.dns", {QStringLiteral("router-monitor/shared/lib")});
    QTest::newRow("system log") << spec(
        "system-log/plasmoids/org.devl0rd.logmon.journal", "org.devl0rd.logmon.journal", {QStringLiteral("system-log/shared/lib")});
    QTest::newRow("taskbar") << spec("taskbar/plasmoids/org.devl0rd.taskbar", "org.devl0rd.taskbar");
}

void pageRows()
{
    QTest::addColumn<ConfigPage>("configPage");
    QTest::newRow("system monitor") << page("system-monitor/plasmoids/org.devl0rd.sysmon.panel", "org.devl0rd.sysmon.panel");
    QTest::newRow("process monitor") << page("process-monitor/plasmoids/org.devl0rd.procmon.panel", "org.devl0rd.procmon.panel");
    QTest::newRow("router monitor") << page("router-monitor/plasmoids/org.devl0rd.routermon.panel", "org.devl0rd.routermon.panel",
        {QStringLiteral("router-monitor/shared/lib")});
    QTest::newRow("router dns") << page(
        "router-monitor/plasmoids/org.devl0rd.routermon.panel", "org.devl0rd.routermon.dns", {QStringLiteral("router-monitor/shared/lib")});
    QTest::newRow("system log") << page(
        "system-log/plasmoids/org.devl0rd.logmon.journal", "org.devl0rd.logmon.journal", {QStringLiteral("system-log/shared/lib")});
    QTest::newRow("friends") << page("portals/plasmoids/org.devl0rd.portal.friends", "org.devl0rd.portal.friends");
    QTest::newRow("kontrol panel button") << page(
        "portals/plasmoids/org.devl0rd.portal.launcher", "org.devl0rd.portal.launcher", {}, "configButton.qml");
    for (const char *file : {"configAppearance.qml", "configBehavior.qml", "configWorkspaces.qml", "configPins.qml"}) {
        QTest::newRow(file) << page("taskbar/plasmoids/org.devl0rd.taskbar", "org.devl0rd.taskbar", {}, file);
    }
}

QStringList pageSettings(QObject *page)
{
    QStringList settings;
    const QMetaObject *meta = page->metaObject();
    for (int i = 0; i < meta->propertyCount(); ++i) {
        const QString name = QLatin1String(meta->property(i).name());
        if (name.startsWith(QLatin1String("cfg_")) && !name.endsWith(QLatin1String("Default"))) {
            settings.append(name.mid(4));
        }
    }
    settings.sort();
    return settings;
}

QStringList usedSettings(const QString &directory)
{
    static const QRegularExpression use(QStringLiteral("Plasmoid\\.configuration\\.([A-Za-z]+)"));
    QStringList used;
    QDirIterator it(directory, {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        auto matches = use.globalMatch(QString::fromUtf8(file.readAll()));
        while (matches.hasNext()) {
            used.append(matches.next().captured(1));
        }
    }
    used.removeDuplicates();
    used.removeAll(QStringLiteral("writeConfig"));
    used.sort();
    return used;
}

QObject *openPage(PlasmoidHarness &harness, const QString &file = QStringLiteral("configGeneral.qml"))
{
    harness.setUp(Form::Planar);
    return harness.create(QStringLiteral("contents/ui/") + file, configPageProperties(harness, QStringLiteral("General")));
}

bool finished(QObject *page, const QString &result, bool error)
{
    return page->property("connectionResult").toString() == result && page->property("connectionError").toBool() == error
        && !page->property("connectionBusy").toBool();
}

const PlasmoidSpec router = spec(
    "router-monitor/plasmoids/org.devl0rd.routermon.panel", "org.devl0rd.routermon.panel", {QStringLiteral("router-monitor/shared/lib")});
const QString config = QStringLiteral("$HOME/.local/bin/routermon-config ");

}

Q_DECLARE_METATYPE(ConfigPage)

class TestPlasmoidConfig : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void pagesOfferEverySetting_data() { pageRows(); }

    void pagesOfferEverySetting()
    {
        QFETCH(ConfigPage, configPage);
        PlasmoidHarness harness(configPage.plasmoid);
        QObject *page = openPage(harness, configPage.file);
        QVERIFY2(page, qPrintable(harness.error));
        QStringList keys = harness.plasmoid()->configuration()->keys();
        keys.sort();
        QCOMPARE(pageSettings(page), keys);
        for (const QString &key : std::as_const(keys)) {
            QCOMPARE(page->property(qPrintable(QStringLiteral("cfg_") + key)).toString(), harness.config(key).toString());
        }
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void widgetsOnlyReadDeclaredSettings_data() { plasmoidRows(); }

    void widgetsOnlyReadDeclaredSettings()
    {
        QFETCH(PlasmoidSpec, plasmoid);
        PlasmoidHarness harness(plasmoid);
        harness.setUp(Form::Planar);
        const QStringList keys = harness.plasmoid()->configuration()->keys();
        const QStringList used = usedSettings(harness.stagedPath(QStringLiteral("contents")));
        QVERIFY(!used.isEmpty());
        for (const QString &setting : used) {
            QVERIFY2(keys.contains(setting), qPrintable(setting + QStringLiteral(" is not in main.xml")));
        }
    }

    void routerPageLoadsTheConnection()
    {
        PlasmoidHarness harness(router);
        QObject *page = openPage(harness);
        QVERIFY2(page, qPrintable(harness.error));
        QVERIFY(page->property("tabbed").toBool());
        QVERIFY(harness.command(config + QStringLiteral("get")).startsWith(config + QStringLiteral("get # ")));
        QVERIFY(harness.reply(config + QStringLiteral("get"),
            QStringLiteral("{\"host\":\"router\",\"user\":\"root\",\"ssh_key\":\"~/.ssh/key\",\"remote_script\":\"/jffs/x.sh\"}")));
        QVERIFY(page->property("connectionLoaded").toBool());
        QVERIFY(!page->property("connectionBusy").toBool());
        QMetaObject::invokeMethod(page, "runConnectionAction", Q_ARG(QVariant, QStringLiteral("test")), Q_ARG(QVariant, QVariant()));
        QVERIFY(page->property("connectionBusy").toBool());
        QVERIFY(harness.command(config + QStringLiteral("test"))
                .startsWith(config + QStringLiteral("test 'router' 'root' '~/.ssh/key' '/jffs/x.sh' # ")));
        QVERIFY(harness.reply(config + QStringLiteral("test"), QString(), 1, QStringLiteral("Permission denied (publickey)\n")));
        QVERIFY(finished(page, QStringLiteral("Permission denied (publickey)"), true));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void routerPageConnectsWithTheEncodedPassword()
    {
        PlasmoidHarness harness(router);
        QObject *page = openPage(harness);
        QVERIFY2(page, qPrintable(harness.error));
        QVERIFY(harness.reply(config + QStringLiteral("get"), QStringLiteral("{\"host\":\"it's\"}")));
        QMetaObject::invokeMethod(
            page, "runConnectionAction", Q_ARG(QVariant, QStringLiteral("connect")), Q_ARG(QVariant, QStringLiteral("pa ss")));
        QVERIFY(harness.command(config + QStringLiteral("connect"))
                .startsWith(
                    config + QStringLiteral("connect 'it'\\''s' 'admin' '~/.ssh/id_ed25519' '/jffs/lrm-collect.sh' 'cGEgc3M=' # ")));
        QVERIFY(harness.reply(config + QStringLiteral("connect"), QString()));
        QVERIFY(finished(page, QStringLiteral("Done"), false));
        const QStringList encoded {
            QStringLiteral("cA=="), QStringLiteral("cGE="), QStringLiteral("cGEgc3Mgw6k="), QStringLiteral("cMOkc3N3w7ZyZA=="), QString()};
        QCOMPARE(harness.eval(QStringLiteral("['p', 'pa', 'pa ss é', 'pässwörd', ''].map(base64)"), page).toStringList(), encoded);
        QMetaObject::invokeMethod(page, "openRouterSettings");
        QCOMPARE(harness.command(QStringLiteral("xdg-open")), QStringLiteral("xdg-open 'http://it'\\''s'"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void routerPageSavesTheAdGuardLogin()
    {
        PlasmoidHarness harness(router);
        QObject *page = openPage(harness);
        QVERIFY2(page, qPrintable(harness.error));
        QVERIFY(harness.reply(config + QStringLiteral("get"),
            QStringLiteral("{\"host\":\"router\",\"adguard_url\":\"http://agh:3000\",\"adguard_username\":\"admin\","
                           "\"adguard_password_set\":true}")));
        QCOMPARE(harness.eval(QStringLiteral("[adguardUrl.text, adguardUser.text]"), page).toStringList(),
            (QStringList {QStringLiteral("http://agh:3000"), QStringLiteral("admin")}));
        QVERIFY(page->property("adguardPasswordSet").toBool());
        harness.eval(QStringLiteral("adguardPassword.text = 'pa ss'"), page);
        QMetaObject::invokeMethod(page, "saveAdguard");
        QVERIFY(harness.command(config + QStringLiteral("adguard"))
                .startsWith(config + QStringLiteral("adguard 'http://agh:3000' 'admin' 'cGEgc3M=' # ")));
        QCOMPARE(harness.eval(QStringLiteral("adguardPassword.text"), page).toString(), QString());
        QVERIFY(harness.reply(config + QStringLiteral("adguard"), QStringLiteral("AdGuard Home login saved and working\n")));
        QVERIFY(finished(page, QStringLiteral("AdGuard Home login saved and working"), false));
        QMetaObject::invokeMethod(page, "saveAdguard");
        QVERIFY(harness.command(config + QStringLiteral("adguard"))
                .startsWith(config + QStringLiteral("adguard 'http://agh:3000' 'admin' '' # ")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void routerPageShowsAFailedRead()
    {
        PlasmoidHarness harness(router);
        QObject *page = openPage(harness);
        QVERIFY2(page, qPrintable(harness.error));
        QVERIFY(harness.reply(config + QStringLiteral("get"), QString(), 127, QStringLiteral("sh: routermon-config: not found\n")));
        QVERIFY(!page->property("connectionLoaded").toBool());
        QVERIFY(finished(page, QStringLiteral("sh: routermon-config: not found"), true));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidConfig)

#include "test_plasmoid_config.moc"
