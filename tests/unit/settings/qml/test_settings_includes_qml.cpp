#include "settingsqmlharness.h"

#include "config/forceresizable.h"
#include "config/loader.h"

#include <QSaveFile>

using namespace Konveyor;
using namespace Konveyor::Settings::Testing;

class TestSettingsIncludesQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void laterIncludeWinsAndIsReported();
    void earlierIncludeIsOverridden();
    void nestedIncludeIsReported();
    void toggleAndRemoveAreReported();
    void includedRuleForSameAppIsReported();
    void otherListEntriesAreNotCompared();
    void editsNeverTouchIncludedFiles();
    void includedFileChangeRefreshesValues();
    void fixingABrokenIncludeInASubfolderClearsTheError();
    void newSectionsStayBeforeForceResizableInclude();
    void forceResizableToggleAfterUiEdits();

private:
    struct Session
    {
        std::unique_ptr<QQmlEngine> engine = std::make_unique<QQmlEngine>();
        QObject *store = nullptr;
        std::unique_ptr<QSignalSpy> failed;
    };

    Session open(const QString &main, const QList<std::pair<QString, QString>> &included)
    {
        m_home.resetConfig(main);
        for (const auto &[name, text] : included) {
            QDir(m_home.configDir()).mkpath(QFileInfo(name).path());
            SettingsHome::write(includedPath(name), text);
        }
        Session session;
        m_home.create(*session.engine, "QtObject {}");
        session.store = SettingsHome::store(*session.engine);
        session.failed = std::make_unique<QSignalSpy>(session.store, SIGNAL(editFailed(QString)));
        return session;
    }

    QString includedPath(const QString &name) const { return QDir(m_home.configDir()).filePath(name); }

    static QStringList messages(const Session &session)
    {
        QStringList result;
        for (const QList<QVariant> &arguments : *session.failed) {
            result.append(arguments.first().toString());
        }
        return result;
    }

    static int gaps(const Session &session)
    {
        return call<QVariantMap>(session.store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt();
    }

    Config::Config runtimeConfig() const
    {
        const auto loaded = Config::loadFile(m_home.configPath());
        if (!loaded) {
            qWarning("%s", qPrintable(loaded.error().toString()));
            return {};
        }
        return loaded->config;
    }

    SettingsHome m_home;
};

void TestSettingsIncludesQml::laterIncludeWinsAndIsReported()
{
    Session session = open(QStringLiteral("layout {\n    gaps 4\n}\n\ninclude \"extra.kdl\"\n"),
        {{QStringLiteral("extra.kdl"), QStringLiteral("layout { gaps 9; }\n")}});
    QCOMPARE(gaps(session), 9);
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {5}, QVariantMap {}));
    QCOMPARE(messages(session),
        QStringList {QStringLiteral("gaps is also set in extra.kdl, which is included later, so that value is the one Konveyor uses.")});
    QCOMPARE(gaps(session), 9);
    QVERIFY(saveAndWait(session.store));
    QCOMPARE(SettingsHome::read(m_home.configPath()), QStringLiteral("layout {\n    gaps 5\n}\n\ninclude \"extra.kdl\"\n"));
    QCOMPARE(runtimeConfig().layout.gaps, 9.0);
}

void TestSettingsIncludesQml::earlierIncludeIsOverridden()
{
    Session session = open(QStringLiteral("include \"extra.kdl\"\n\nlayout {\n    gaps 4\n}\n"),
        {{QStringLiteral("extra.kdl"), QStringLiteral("layout { gaps 9; }\n")}});
    QCOMPARE(gaps(session), 4);
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {5}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/struts/left"), QVariantList {5}, QVariantMap {}));
    QCOMPARE(messages(session), QStringList {});
    QCOMPARE(gaps(session), 5);
}

void TestSettingsIncludesQml::nestedIncludeIsReported()
{
    Session session = open(QStringLiteral("input {\n}\ninclude \"a.kdl\"\n"),
        {{QStringLiteral("a.kdl"), QStringLiteral("include \"more/b.kdl\"\n")},
            {QStringLiteral("more/b.kdl"), QStringLiteral("input { focus-follows-mouse; }\n")}});
    QCOMPARE(session.store->property("values").toMap().value(QStringLiteral("input/focus-follows-mouse")).toBool(), true);
    const QString message
        = QStringLiteral("focus-follows-mouse is also set in more/b.kdl, which is included later, so that value is the one Konveyor uses.");
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("input/focus-follows-mouse"), false));
    QCOMPARE(messages(session), QStringList {message});

    Session cycle = open(QStringLiteral("input {\n}\ninclude \"a.kdl\"\n"),
        {{QStringLiteral("a.kdl"), QStringLiteral("include \"more/b.kdl\"\n")},
            {QStringLiteral("more/b.kdl"), QStringLiteral("include \"../a.kdl\"\ninput { focus-follows-mouse; }\n")}});
    QVERIFY(cycle.store->property("configError").toString().contains(QStringLiteral("recursive include")));
    QVERIFY(call<bool>(cycle.store, "setFlag", QStringLiteral("input/focus-follows-mouse"), false));
    QCOMPARE(messages(cycle), QStringList {message});
}

void TestSettingsIncludesQml::toggleAndRemoveAreReported()
{
    Session session = open(QStringLiteral("layout {\n    gaps 4\n    border { off; }\n}\ninclude \"a.kdl\"\ninclude \"b.kdl\"\n"),
        {{QStringLiteral("a.kdl"), QStringLiteral("layout { border { on; }; }\n")},
            {QStringLiteral("b.kdl"), QStringLiteral("layout { gaps 2; border { width 9; }; }\n")}});
    QVERIFY(call<bool>(session.store, "setToggle", QStringLiteral("layout/border"), false));
    QVERIFY(call<bool>(session.store, "remove", QStringLiteral("layout/gaps")));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/center-focused-column"), QVariantList {QStringLiteral("always")},
        QVariantMap {}));
    QCOMPARE(messages(session),
        (QStringList {QStringLiteral("border is also set in a.kdl, which is included later, so that value is the one Konveyor uses."),
            QStringLiteral("gaps is also set in b.kdl, which is included later, so that value is the one Konveyor uses.")}));
}

void TestSettingsIncludesQml::includedRuleForSameAppIsReported()
{
    const QString rule = QStringLiteral("window-rule {\n    match app-id=r#\"^game\\.exe$\"#\n    force-resizable true\n}\n");
    Session session = open(QStringLiteral("window-rule {\n    match app-id=r#\"^game\\.exe$\"#\n}\n\ninclude \"force-resizable.kdl\"\n"),
        {{QStringLiteral("force-resizable.kdl"), rule}});
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("window-rule/force-resizable"), QVariantList {false}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("window-rule/opacity"), QVariantList {0.5}, QVariantMap {}));
    QCOMPARE(messages(session),
        QStringList {QStringLiteral(
            "force-resizable is also set in force-resizable.kdl, which is included later, so that value is the one Konveyor uses.")});
    QVERIFY(saveAndWait(session.store));
    const Config::Config config = runtimeConfig();
    QCOMPARE(config.windowRules.size(), 2);
    QCOMPARE(config.windowRules.last().forceResizable, std::optional(true));
}

void TestSettingsIncludesQml::otherListEntriesAreNotCompared()
{
    Session session = open(QStringLiteral("window-rule {\n    match app-id=\"a\"\n}\noutput \"DP-1\"\ninclude \"x.kdl\"\n"),
        {{QStringLiteral("x.kdl"),
            QStringLiteral("window-rule { match app-id=\"b\"; opacity 0.5; }\noutput \"DP-1\" { layout { gaps 3; }; }\n")}});
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("window-rule/opacity"), QVariantList {0.9}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("output/layout/gaps"), QVariantList {1}, QVariantMap {}));
    QCOMPARE(messages(session), QStringList {});
}

void TestSettingsIncludesQml::editsNeverTouchIncludedFiles()
{
    const QString extra = QStringLiteral("// mine\nlayout {\n    gaps 9\n}\n");
    Session session = open(QStringLiteral("include \"extra.kdl\"\n"), {{QStringLiteral("extra.kdl"), extra}});
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {5}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "remove", QStringLiteral("include")));
    QVERIFY(saveAndWait(session.store));
    QCOMPARE(SettingsHome::read(includedPath(QStringLiteral("extra.kdl"))), extra);
    QCOMPARE(SettingsHome::read(m_home.configPath()), QStringLiteral("layout {\n    gaps 5\n}\n"));
}

void TestSettingsIncludesQml::includedFileChangeRefreshesValues()
{
    Session session
        = open(QStringLiteral("include \"extra.kdl\"\n"), {{QStringLiteral("extra.kdl"), QStringLiteral("layout { gaps 9; }\n")}});
    QCOMPARE(gaps(session), 9);
    QVERIFY(SettingsHome::write(includedPath(QStringLiteral("extra.kdl")), QStringLiteral("layout { gaps 12; }\n")));
    QTRY_COMPARE_WITH_TIMEOUT(gaps(session), 12, SignalTimeoutMs);
    QCOMPARE(session.store->property("needsSave").toBool(), false);
}

void TestSettingsIncludesQml::fixingABrokenIncludeInASubfolderClearsTheError()
{
    Session session = open(
        QStringLiteral("include \"parts/extra.kdl\"\n"), {{QStringLiteral("parts/extra.kdl"), QStringLiteral("layout { gaps 9; }\n")}});
    QCOMPARE(gaps(session), 9);
    const auto replace = [this](const QString &text) {
        QSaveFile file(includedPath(QStringLiteral("parts/extra.kdl")));
        return file.open(QIODevice::WriteOnly) && file.write(text.toUtf8()) >= 0 && file.commit();
    };
    QVERIFY(replace(QStringLiteral("layout { gaps 9\n")));
    QTRY_VERIFY_WITH_TIMEOUT(!session.store->property("configError").toString().isEmpty(), SignalTimeoutMs);
    QVERIFY(replace(QStringLiteral("layout { gaps 12; }\n")));
    QTRY_COMPARE_WITH_TIMEOUT(session.store->property("configError").toString(), QString(), SignalTimeoutMs);
    QCOMPARE(gaps(session), 12);
}

void TestSettingsIncludesQml::newSectionsStayBeforeForceResizableInclude()
{
    Session session = open(QStringLiteral("layout {\n}\n\ninclude \"force-resizable.kdl\"\n"),
        {{QStringLiteral("force-resizable.kdl"),
            QStringLiteral("window-rule {\n    match app-id=r#\"^a$\"#\n    force-resizable true\n}\n")}});
    const QVariantMap rule {{QStringLiteral("name"), QStringLiteral("window-rule")},
        {QStringLiteral("children"),
            QVariantList {QVariantMap {{QStringLiteral("name"), QStringLiteral("match")},
                {QStringLiteral("props"), QVariantMap {{QStringLiteral("app-id"), QStringLiteral("^a$")}}}}}}};
    QCOMPARE(call<QString>(session.store, "append", QString(), rule), QStringLiteral("window-rule"));
    QVERIFY(call<bool>(
        session.store, "setValue", QStringLiteral("gestures/titlebar-drag"), QVariantList {QStringLiteral("move-window")}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("window-rule/force-resizable"), QVariantList {false}, QVariantMap {}));
    QVERIFY(saveAndWait(session.store));
    QCOMPARE(SettingsHome::read(m_home.configPath()),
        QStringLiteral("layout {\n}\n\nwindow-rule {\n    match app-id=\"^a$\"\n    force-resizable false\n}\n\ngestures {\n    "
                       "titlebar-drag \"move-window\"\n}\n\n"
                       "include \"force-resizable.kdl\"\n"));
    QCOMPARE(runtimeConfig().windowRules.last().forceResizable, std::optional(true));
}

void TestSettingsIncludesQml::forceResizableToggleAfterUiEdits()
{
    Session session = open(QStringLiteral("layout {\n}\n"), {});
    session.store->setProperty("autoSave", true);
    QSignalSpy saved(session.store, SIGNAL(saved()));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {3}, QVariantMap {}));
    QVERIFY(saved.wait(SignalTimeoutMs));

    const QString overridePath = includedPath(QStringLiteral("force-resizable.kdl"));
    const auto rule = Config::setForceResizableRule(QString(), overridePath, QStringLiteral("game.exe"), true);
    QVERIFY(rule.has_value());
    QVERIFY(SettingsHome::write(overridePath, *rule));
    const auto main = Config::ensureForceResizableInclude(SettingsHome::read(m_home.configPath()), m_home.configPath());
    QVERIFY(main.has_value());
    QVERIFY(SettingsHome::write(m_home.configPath(), *main));
    QTRY_VERIFY_WITH_TIMEOUT(call<bool>(session.store, "has", QStringLiteral("include")), SignalTimeoutMs);

    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("disable-minimize"), true));
    QVERIFY(saved.wait(SignalTimeoutMs));
    const QString text = SettingsHome::read(m_home.configPath());
    QCOMPARE(text, QStringLiteral("layout {\n    gaps 3\n}\n\ndisable-minimize\n\ninclude \"force-resizable.kdl\"\n"));
    QCOMPARE(Config::ensureForceResizableInclude(text, m_home.configPath()).value(), text);
    const Config::Config config = runtimeConfig();
    QCOMPARE(config.disableMinimize, true);
    QCOMPARE(config.windowRules.size(), 1);
    QCOMPARE(messages(session), QStringList {});
}

QTEST_MAIN(TestSettingsIncludesQml)
#include "test_settings_includes_qml.moc"
