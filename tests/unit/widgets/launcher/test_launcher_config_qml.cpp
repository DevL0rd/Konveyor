#include "launcherharness.h"

#include <QMetaProperty>
#include <QXmlStreamReader>

using LauncherTest::response;
using LauncherTest::source;

namespace
{

const QStringList portalOnly {
    QStringLiteral("icon"), QStringLiteral("popupWidth"), QStringLiteral("popupHeight"), QStringLiteral("showFriendsBadge")};

QStringList schemaKeys(const char *schema)
{
    QFile file(source(schema));
    QStringList keys;
    if (!file.open(QIODevice::ReadOnly)) {
        return keys;
    }
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QLatin1String("entry")) {
            keys.append(xml.attributes().value(QLatin1String("name")).toString());
        }
    }
    return keys;
}

QStringList cfgProperties(const QObject *form)
{
    QStringList names;
    const QMetaObject *meta = form->metaObject();
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        const QString name = QString::fromLatin1(meta->property(i).name());
        if (name.startsWith(QLatin1String("cfg_")) && !name.endsWith(QLatin1String("Default"))) {
            names.append(name.mid(4));
        }
    }
    return names;
}

}

class TestLauncherConfigQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QObject *general(const QVariantMap &initial) { return m_harness.create(QStringLiteral("configGeneral.qml"), initial); }

    static QString items(const QString &condition)
    {
        return QStringLiteral("(function walk(i, out) { out.push(i); for (const c of i.children) walk(c, out); return out })(form, [])"
                              ".filter(c => %1)")
            .arg(condition);
    }

    QVariant form(const QString &expression) { return m_harness.evalIn(qmlContext(m_harness.root()), m_harness.root(), expression); }

    QVariantMap initialFromSchema(const char *schemaFile, bool portal)
    {
        KConfigPropertyMap *config = m_harness.loadConfig(source(schemaFile), QStringLiteral("config/formrc"));
        QVariantMap initial {{QStringLiteral("portal"), portal}};
        for (const QString &key : config->keys()) {
            initial.insert(QStringLiteral("cfg_") + key, config->value(key));
        }
        return initial;
    }

private Q_SLOTS:
    void everySettingIsStored()
    {
        QObject *root = general({});
        QVERIFY(root);
        const QStringList properties = cfgProperties(root);
        const QStringList portal = schemaKeys("widgets/portals/plasmoids/org.devl0rd.portal/contents/config/main.xml");
        const QStringList kontrolPanel = schemaKeys("widgets/portals/kontrol-panel/config/main.xml");
        for (const QString &name : properties) {
            QVERIFY2(portal.contains(name), qPrintable(name));
            QVERIFY2(kontrolPanel.contains(name) != portalOnly.contains(name), qPrintable(name));
        }
    }

    void thePortalIsRecognisedByItsPluginId()
    {
        QObject *root = general({});
        QVERIFY(root);
        QVERIFY(root->property("portal").toBool());
        m_harness.singleton("org.kde.plasma.plasmoid", "Plasmoid")
            ->setProperty("metaData", QVariantMap {{QStringLiteral("pluginId"), QStringLiteral("other")}});
        QVERIFY(!root->property("portal").toBool());
    }

    void settingsRoundTripUnchanged_data()
    {
        QTest::addColumn<QString>("schema");
        QTest::addColumn<bool>("portal");
        QTest::newRow("Kontrol Panel") << QStringLiteral("widgets/portals/kontrol-panel/config/main.xml") << false;
        QTest::newRow("App Portal") << QStringLiteral("widgets/portals/plasmoids/org.devl0rd.portal/contents/config/main.xml") << true;
    }

    void settingsRoundTripUnchanged()
    {
        QFETCH(QString, schema);
        QFETCH(bool, portal);
        QVariantMap initial = initialFromSchema(qPrintable(schema), portal);
        const QStringList known = cfgProperties(general({}));
        m_harness.reset();
        LauncherTest::captureWarnings();
        for (auto it = initial.begin(); it != initial.end();) {
            it = it.key().startsWith(QLatin1String("cfg_")) && !known.contains(it.key().mid(4)) ? initial.erase(it) : std::next(it);
        }
        QObject *root = general(initial);
        QVERIFY(root);
        for (auto it = initial.cbegin(); it != initial.cend(); ++it) {
            QCOMPARE(root->property(qPrintable(it.key())), it.value());
        }
    }

    void controlsWriteTheirSettings()
    {
        QObject *root = general({{QStringLiteral("portal"), false}, {QStringLiteral("cfg_defaultPage"), QStringLiteral("games")},
            {QStringLiteral("cfg_appsSort"), QStringLiteral("recent")}, {QStringLiteral("cfg_appsView"), QStringLiteral("list")},
            {QStringLiteral("cfg_showGames"), true}, {QStringLiteral("cfg_cardWidth"), 90}});
        QVERIFY(root);
        const QString combos = items(QStringLiteral("c.valueRole === 'value'"));
        QCOMPARE(form(combos + QStringLiteral(".map(c => c.currentValue)")).toStringList(),
            QStringList({QStringLiteral("games"), QStringLiteral("recent"), QStringLiteral("list")}));
        form(combos + QStringLiteral("[0].currentIndex = 4"));
        form(combos + QStringLiteral("[0].activated(4)"));
        QCOMPARE(root->property("cfg_defaultPage").toString(), QStringLiteral("friends"));
        form(QStringLiteral("showGames.toggle()"));
        QCOMPARE(root->property("cfg_showGames").toBool(), false);
        form(QStringLiteral("widthBox.value = 500"));
        QCOMPARE(root->property("cfg_cardWidth").toInt(), 160);
        form(QStringLiteral("dimSlider.value = 0.3"));
        QCOMPARE(root->property("cfg_dimStrength").toDouble(), 0.3);
    }

    void buttonCheckboxesWriteTheirSettings_data()
    {
        QTest::addColumn<QString>("key");
        for (const char *key : {"showTopBarFriends", "showTopBarClock", "showTopBarSettings", "showTopBarPower", "showSidebarFriends",
                 "showSidebarSystem", "showSidebarSettings", "showHomeDate"}) {
            QTest::newRow(key) << QString::fromLatin1(key);
        }
    }

    void buttonCheckboxesWriteTheirSettings()
    {
        QFETCH(QString, key);
        QObject *root = general({{QStringLiteral("portal"), false}, {QStringLiteral("cfg_showFriends"), true}});
        QVERIFY(root);
        const bool before = root->property(qPrintable(QStringLiteral("cfg_") + key)).toBool();
        QVERIFY(form(key + QStringLiteral(".visible && ") + key + QStringLiteral(".enabled")).toBool());
        form(key + QStringLiteral(".toggle()"));
        QCOMPARE(root->property(qPrintable(QStringLiteral("cfg_") + key)).toBool(), !before);
    }

    void friendsButtonsNeedFriends()
    {
        QObject *root = general({{QStringLiteral("portal"), false}, {QStringLiteral("cfg_showFriends"), true}});
        QVERIFY(root);
        QVERIFY(form(QStringLiteral("showTopBarFriends.enabled && showSidebarFriends.enabled")).toBool());
        form(QStringLiteral("showFriends.toggle()"));
        QVERIFY(!form(QStringLiteral("showTopBarFriends.enabled || showSidebarFriends.enabled")).toBool());
        QVERIFY(form(QStringLiteral("showSidebarSystem.enabled && showTopBarPower.enabled")).toBool());
    }

    void theAppPortalOnlyOffersButtonsItShows()
    {
        QObject *root = general({{QStringLiteral("portal"), true}});
        QVERIFY(root);
        for (const char *shown :
            {"showTopBarFriends", "showTopBarSettings", "showSidebarFriends", "showSidebarSystem", "showSidebarSettings"}) {
            QVERIFY2(form(QLatin1String(shown) + QStringLiteral(".visible")).toBool(), shown);
        }
        for (const char *hidden : {"showTopBarClock", "showTopBarPower", "showHomeDate"}) {
            QVERIFY2(!form(QLatin1String(hidden) + QStringLiteral(".visible")).toBool(), hidden);
        }
    }

    void resultOrderMovesGroups()
    {
        QObject *root = general({{QStringLiteral("cfg_searchOrder"), QStringLiteral("apps,bogus,games")}});
        QVERIFY(root);
        QCOMPARE(form(QStringLiteral("orderList.slice(0, 3)")).toStringList(),
            QStringList({QStringLiteral("apps"), QStringLiteral("games"), QStringLiteral("answer")}));
        form(QStringLiteral("moveGroup(1, -1)"));
        QVERIFY(root->property("cfg_searchOrder").toString().startsWith(QStringLiteral("games,apps,answer,")));
        const QString before = root->property("cfg_searchOrder").toString();
        form(QStringLiteral("moveGroup(0, -1)"));
        form(QStringLiteral("moveGroup(8, 1)"));
        QCOMPARE(root->property("cfg_searchOrder").toString(), before);
    }

    void forgettingAndIconDefaults()
    {
        QObject *root = general(
            {{QStringLiteral("cfg_learnedRanking"), QStringLiteral("{\"a\":{}}")}, {QStringLiteral("cfg_icon"), QStringLiteral("kde")}});
        QVERIFY(root);
        const QString buttons = items(QStringLiteral("c.text === '%1'")) + QStringLiteral("[0]");
        QVERIFY(form(buttons.arg(QStringLiteral("Use the default icon")) + QStringLiteral(".enabled")).toBool());
        form(buttons.arg(QStringLiteral("Use the default icon")) + QStringLiteral(".clicked()"));
        QCOMPARE(root->property("cfg_icon").toString(), QStringLiteral("view-app-grid-symbolic"));
        QVERIFY(!form(buttons.arg(QStringLiteral("Use the default icon")) + QStringLiteral(".enabled")).toBool());
        form(items(QStringLiteral("c.text === 'Forget what I usually open'")) + QStringLiteral("[0].clicked()"));
        QCOMPARE(root->property("cfg_learnedRanking").toString(), QString());
    }

    void hiddenAppsCanBeShownAgain()
    {
        m_harness.respond({response(QStringLiteral("portal-games --hidden"), "[\"org.kde.konsole\", \"it's\"]")});
        QObject *root = general({});
        QVERIFY(root);
        TRY_COMPARE(root->property("hiddenApps").toStringList(), QStringList({QStringLiteral("org.kde.konsole"), QStringLiteral("it's")}));
        m_harness.respond(
            {response(QStringLiteral("--unhide"), ""), response(QStringLiteral("portal-games --hidden"), R"(["org.kde.konsole"])")});
        form(QStringLiteral("unhide(\"it's\")"));
        QVERIFY(m_harness.commands().contains(QStringLiteral("$HOME/.local/bin/portal-games --unhide 'it'\\''s'")));
        TRY_COMPARE(root->property("hiddenApps").toStringList(), QStringList {QStringLiteral("org.kde.konsole")});
        m_harness.respond({response(QStringLiteral("portal-games --hidden"), "broken")});
        form(QStringLiteral("loadHidden()"));
        TRY_COMPARE(root->property("hiddenApps").toStringList(), QStringList());
    }

    void launcherButtonSettings()
    {
        const QString path = source("widgets/portals/plasmoids/org.devl0rd.portal.launcher/contents/ui/configButton.qml");
        QObject *root = m_harness.create(path,
            {{QStringLiteral("cfg_icon"), QStringLiteral("start-here-kde-plasma-symbolic")},
                {QStringLiteral("cfg_label"), QStringLiteral("Start")}, {QStringLiteral("cfg_showLabel"), false}});
        QVERIFY(root);
        QCOMPARE(cfgProperties(root), schemaKeys("widgets/portals/plasmoids/org.devl0rd.portal.launcher/contents/config/main.xml"));
        QVERIFY(!form(QStringLiteral("labelField.enabled")).toBool());
        form(QStringLiteral("showLabel.toggle()"));
        QVERIFY(form(QStringLiteral("labelField.enabled")).toBool());
        QCOMPARE(root->property("cfg_showLabel").toBool(), true);
        form(QStringLiteral("labelField.text = 'Apps'"));
        QCOMPARE(root->property("cfg_label").toString(), QStringLiteral("Apps"));
        const QString buttons = items(QStringLiteral("c.text === '%1'")) + QStringLiteral("[0]");
        QVERIFY(!form(buttons.arg(QStringLiteral("Use the default Plasma icon")) + QStringLiteral(".enabled")).toBool());
        form(buttons.arg(QStringLiteral("Kontrol Panel settings…")) + QStringLiteral(".clicked()"));
        QVERIFY(m_harness.commands().last().startsWith(
            QStringLiteral("busctl --user call org.devl0rd.KontrolPanel /KontrolPanel org.devl0rd.KontrolPanel Configure # ")));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherConfigQml)
#include "test_launcher_config_qml.moc"
