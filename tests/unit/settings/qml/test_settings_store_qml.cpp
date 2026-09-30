#include "settingsqmlharness.h"

#include <QKeySequence>

using namespace Konveyor::Settings::Testing;

class TestSettingsStoreQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }
    void init() { QVERIFY(m_home.installDefaults()); }

    void undoRestoresPreviousText();
    void undoStepsBackOneEditAtATime();
    void undoSavesWhenAutoSaving();
    void isDefaultComparesWithShippedConfig();
    void resetToDefaultWritesShippedNode();
    void defaultsRestoresShippedText();
    void missingShippedConfigIsReported();
    void setToggleReplacesOnAndOff();
    void setFlagAndSetValue();
    void failedEditsAreReported();
    void appendMoveAndRead();
    void saveIsBlockedWhileConfigHasError();
    void reloadsWhenFileChangesUnderneath();
    void externalEditWinsOverUnsavedEdits();
    void pendingAutoSaveNeverOverwritesAnExternalEdit();
    void loadsAFixFromDiskOverEditsBlockedByAConfigError();
    void neverSavesOverAConfigItCannotRead();
    void scopeFollowsRuntimeNames();
    void helpersForPages();

private:
    struct Session
    {
        std::unique_ptr<QQmlEngine> engine = std::make_unique<QQmlEngine>();
        QObject *store = nullptr;
    };

    Session open(const std::optional<QString> &text)
    {
        m_home.resetConfig(text);
        Session session;
        m_home.create(*session.engine, "QtObject {}");
        session.store = SettingsHome::store(*session.engine);
        return session;
    }

    QString saved(QObject *store)
    {
        if (!saveAndWait(store)) {
            return QStringLiteral("<not saved>");
        }
        return SettingsHome::read(m_home.configPath());
    }

    QString droppedMessage() const
    {
        return m_home.configPath()
            + QStringLiteral(" changed on disk, so Settings loaded it. Your changes here that were not saved yet were not applied.");
    }

    static constexpr int SaveSettleMs = 700;

    SettingsHome m_home;
};

void TestSettingsStoreQml::undoRestoresPreviousText()
{
    const QString text = QStringLiteral("layout {\n    gaps 4 // mine\n}\n");
    Session session = open(text);
    QCOMPARE(session.store->property("canUndo").toBool(), false);
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {8}, QVariantMap {}));
    QCOMPARE(session.store->property("canUndo").toBool(), true);
    QCOMPARE(session.store->property("needsSave").toBool(), true);
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QCOMPARE(session.store->property("canUndo").toBool(), false);
    QCOMPARE(session.store->property("needsSave").toBool(), false);
    QCOMPARE(saved(session.store), text);
}

void TestSettingsStoreQml::undoStepsBackOneEditAtATime()
{
    Session session = open(QStringLiteral("layout {\n    gaps 4\n}\n"));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {8}, QVariantMap {}));
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("layout/float-child-windows"), true));
    QCOMPARE(saved(session.store), QStringLiteral("layout {\n    gaps 4\n    float-child-windows\n}\n"));
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QCOMPARE(saved(session.store), QStringLiteral("layout {\n    gaps 4\n}\n"));
}

void TestSettingsStoreQml::undoSavesWhenAutoSaving()
{
    Session session = open(QStringLiteral("layout {\n    gaps 4\n}\n"));
    session.store->setProperty("autoSave", true);
    QSignalSpy savedSpy(session.store, SIGNAL(saved()));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {9}, QVariantMap {}));
    QVERIFY(savedSpy.wait(SignalTimeoutMs));
    QCOMPARE(SettingsHome::read(m_home.configPath()), QStringLiteral("layout {\n    gaps 9\n}\n"));
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QVERIFY(savedSpy.wait(SignalTimeoutMs));
    QCOMPARE(SettingsHome::read(m_home.configPath()), QStringLiteral("layout {\n    gaps 4\n}\n"));
}

void TestSettingsStoreQml::isDefaultComparesWithShippedConfig()
{
    Session session = open(std::nullopt);
    QCOMPARE(session.store->property("representsDefaults").toBool(), true);
    QVERIFY(call<bool>(session.store, "isDefault", QStringLiteral("layout/gaps")));
    QVERIFY(call<bool>(session.store, "isDefault", QStringLiteral("layout/always-center-single-column")));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {16.0}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "isDefault", QStringLiteral("layout/gaps")));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {3}, QVariantMap {}));
    QVERIFY(!call<bool>(session.store, "isDefault", QStringLiteral("layout/gaps")));
    QCOMPARE(session.store->property("representsDefaults").toBool(), false);
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("layout/always-center-single-column"), true));
    QVERIFY(!call<bool>(session.store, "isDefault", QStringLiteral("layout/always-center-single-column")));
    QVERIFY(call<bool>(session.store, "remove", QStringLiteral("layout/float-child-windows")));
    QVERIFY(!call<bool>(session.store, "isDefault", QStringLiteral("layout/float-child-windows")));
}

void TestSettingsStoreQml::resetToDefaultWritesShippedNode()
{
    Session session = open(QStringLiteral("layout {\n    gaps 3\n    always-center-single-column\n    focus-ring { off; }\n}\n"));
    QVERIFY(call<bool>(session.store, "resetToDefault", QStringLiteral("layout/gaps")));
    QVERIFY(call<bool>(session.store, "resetToDefault", QStringLiteral("layout/always-center-single-column")));
    QVERIFY(call<bool>(session.store, "resetToDefault", QStringLiteral("layout/focus-ring")));
    QVERIFY(call<bool>(session.store, "resetToDefault", QStringLiteral("layout/float-child-windows")));
    QCOMPARE(saved(session.store),
        QStringLiteral("layout {\n    gaps 16\n    focus-ring {\n        width 4\n        active-color \"accent\"\n        inactive-color "
                       "\"#505050\"\n    }\n"
                       "    float-child-windows false\n}\n"));
    for (const QString &path : {QStringLiteral("layout/gaps"), QStringLiteral("layout/always-center-single-column"),
             QStringLiteral("layout/focus-ring"), QStringLiteral("layout/float-child-windows")}) {
        QVERIFY2(call<bool>(session.store, "isDefault", path), qPrintable(path));
    }
}

void TestSettingsStoreQml::defaultsRestoresShippedText()
{
    Session session = open(QStringLiteral("layout {\n    gaps 3\n}\n"));
    QCOMPARE(session.store->property("representsDefaults").toBool(), false);
    QVERIFY(QMetaObject::invokeMethod(session.store, "defaults"));
    QCOMPARE(session.store->property("representsDefaults").toBool(), true);
    QCOMPARE(saved(session.store), SettingsHome::read(m_home.dataPath()));
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QCOMPARE(saved(session.store), QStringLiteral("layout {\n    gaps 3\n}\n"));
}

void TestSettingsStoreQml::missingShippedConfigIsReported()
{
    QVERIFY(QFile::remove(m_home.dataPath()));
    Session session = open(QStringLiteral("layout {\n}\n"));
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    QVERIFY(QMetaObject::invokeMethod(session.store, "load"));
    QCOMPARE(failed.count(), 1);
    QCOMPARE(
        failed.takeFirst().first().toString(), QStringLiteral("The default Konveyor config is not installed, so Defaults is unavailable."));
    QVERIFY(QMetaObject::invokeMethod(session.store, "defaults"));
    QCOMPARE(failed.takeFirst().first().toString(), QStringLiteral("The default Konveyor config is not installed."));
    QCOMPARE(session.store->property("representsDefaults").toBool(), false);
    QVERIFY(!call<bool>(session.store, "isDefault", QStringLiteral("layout")));
    QVERIFY(call<bool>(session.store, "resetToDefault", QStringLiteral("layout")));
    QCOMPARE(saved(session.store), QString());
}

void TestSettingsStoreQml::setToggleReplacesOnAndOff()
{
    Session session = open(
        QStringLiteral("layout {\n    border { off; width 4; }\n    focus-ring {\n        on\n        off\n        width 2\n    }\n}\n"));
    QVERIFY(!session.store->property("configError").toString().isEmpty());
    QVERIFY(call<bool>(session.store, "setToggle", QStringLiteral("layout/border"), true));
    QVERIFY(call<bool>(session.store, "setToggle", QStringLiteral("layout/focus-ring"), false));
    QVERIFY(call<bool>(session.store, "setToggle", QStringLiteral("layout/insert-hint"), false));
    QVERIFY(call<bool>(session.store, "setToggle", QStringLiteral("animations"), false));
    QCOMPARE(saved(session.store),
        QStringLiteral("layout {\n    border {\n        width 4\n        on\n    }\n    focus-ring {\n        width 2\n        off\n    }\n"
                       "    insert-hint {\n        off\n    }\n}\n\nanimations {\n    off\n}\n"));
    QVERIFY(QMetaObject::invokeMethod(session.store, "undo"));
    QVERIFY(session.store->property("configError").toString().contains(QStringLiteral("off")));
    QCOMPARE(session.store->property("needsSave").toBool(), true);
    QVERIFY(QMetaObject::invokeMethod(session.store, "save"));
    QVERIFY(SettingsHome::read(m_home.configPath()).contains(QStringLiteral("insert-hint")));
}

void TestSettingsStoreQml::setFlagAndSetValue()
{
    Session session = open(QStringLiteral("input {\n    focus-follows-mouse\n}\n"));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("input/focus-follows-mouse"), false));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("input/focus-follows-mouse"), false));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("input/workspace-auto-back-and-forth"), true));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("input/warp-mouse-to-focus"), QVariantList {},
        QVariantMap {{QStringLiteral("mode"), QStringLiteral("center-xy")}}));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("workspace#0"), QVariantList {QStringLiteral("mail")}, QVariantMap {}));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("workspace#1"), QVariantList {QStringLiteral("chat")}, QVariantMap {}));
    QCOMPARE(saved(session.store),
        QStringLiteral("input {\n    workspace-auto-back-and-forth\n    warp-mouse-to-focus mode=\"center-xy\"\n}\n\nworkspace "
                       "\"mail\"\n\nworkspace \"chat\"\n"));
    const QVariantMap values = session.store->property("values").toMap();
    QCOMPARE(values.value(QStringLiteral("input/focus-follows-mouse")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("input/warp-mouse-to-focus/mode")).toString(), QStringLiteral("center-xy"));
}

void TestSettingsStoreQml::failedEditsAreReported()
{
    Session session = open(QStringLiteral("workspace \"a\"\n"));
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    const int revision = session.store->property("revision").toInt();
    QVERIFY(!call<bool>(session.store, "setValue", QStringLiteral("workspace#3"), QVariantList {QStringLiteral("x")}, QVariantMap {}));
    QCOMPARE(failed.takeFirst().first().toString(), QStringLiteral("cannot create workspace#3: earlier siblings are missing"));
    QVERIFY(!call<bool>(session.store, "remove", QStringLiteral("a#b")));
    QCOMPARE(failed.takeFirst().first().toString(), QStringLiteral("invalid index in path segment a#b"));
    QCOMPARE(call<QString>(session.store, "move", QStringLiteral("workspace"), -1), QString());
    QCOMPARE(failed.takeFirst().first().toString(), QStringLiteral("cannot move workspace further"));
    QCOMPARE(call<QString>(session.store, "append", QStringLiteral("x#2"), QVariantMap {{QStringLiteral("name"), QStringLiteral("y")}}),
        QString());
    QCOMPARE(failed.takeFirst().first().toString(), QStringLiteral("cannot create x#2: earlier siblings are missing"));
    QCOMPARE(session.store->property("revision").toInt(), revision);
    QCOMPARE(session.store->property("canUndo").toBool(), false);
}

void TestSettingsStoreQml::appendMoveAndRead()
{
    Session session = open(QStringLiteral("workspace \"a\"\n"));
    const QVariantMap workspace {
        {QStringLiteral("name"), QStringLiteral("workspace")}, {QStringLiteral("args"), QVariantList {QStringLiteral("b")}}};
    QCOMPARE(call<QString>(session.store, "append", QString(), workspace), QStringLiteral("workspace#1"));
    QCOMPARE(call<QString>(session.store, "move", QStringLiteral("workspace#1"), -1), QStringLiteral("workspace"));
    QVERIFY(call<bool>(session.store, "has", QStringLiteral("workspace#1")));
    QVERIFY(!call<bool>(session.store, "has", QStringLiteral("workspace#2")));
    QCOMPARE(
        call<QVariantMap>(session.store, "node", QStringLiteral("workspace")).value(QStringLiteral("args")).toList().first().toString(),
        QStringLiteral("b"));
    QVERIFY(call<QVariantMap>(session.store, "node", QStringLiteral("workspace#2")).isEmpty());
    QCOMPARE(call<QVariantList>(session.store, "children", QString(), QStringLiteral("workspace")).size(), 2);
    QCOMPARE(saved(session.store), QStringLiteral("workspace \"b\"\n\nworkspace \"a\"\n"));
}

void TestSettingsStoreQml::saveIsBlockedWhileConfigHasError()
{
    const QString text = QStringLiteral("layout {\n    gaps \"wide\"\n}\n");
    Session session = open(text);
    QVERIFY(!session.store->property("configError").toString().isEmpty());
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("layout/float-child-windows"), true));
    QVERIFY(QMetaObject::invokeMethod(session.store, "save"));
    QVERIFY(failed.last().first().toString().startsWith(QStringLiteral("Not saved, the config has an error: ")));
    QCOMPARE(SettingsHome::read(m_home.configPath()), text);
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {5}, QVariantMap {}));
    QVERIFY(session.store->property("configError").toString().isEmpty());
    QCOMPARE(saved(session.store), QStringLiteral("layout {\n    gaps 5\n    float-child-windows\n}\n"));

    Session broken = open(QStringLiteral("layout {\n"));
    QVERIFY(!broken.store->property("configError").toString().isEmpty());
    QVERIFY(!call<bool>(broken.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {5}, QVariantMap {}));
}

void TestSettingsStoreQml::reloadsWhenFileChangesUnderneath()
{
    Session session = open(QStringLiteral("layout {\n    gaps 4\n}\n"));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {6}, QVariantMap {}));
    QCOMPARE(saved(session.store), QStringLiteral("layout {\n    gaps 6\n}\n"));
    QSignalSpy changed(session.store, SIGNAL(documentChanged()));
    QVERIFY(SettingsHome::write(m_home.configPath(), QStringLiteral("layout {\n    gaps 11\n}\n")));
    QTRY_VERIFY_WITH_TIMEOUT(call<QVariantMap>(session.store, "node", QStringLiteral("layout/gaps")).value(QStringLiteral("args")).toList()
            == QVariantList {qint64(11)},
        SignalTimeoutMs);
    QCOMPARE(session.store->property("canUndo").toBool(), false);
    QCOMPARE(session.store->property("needsSave").toBool(), false);
}

void TestSettingsStoreQml::externalEditWinsOverUnsavedEdits()
{
    const QString external = QStringLiteral("layout {\n    gaps 11\n}\n");
    Session session = open(QStringLiteral("layout {\n    gaps 4\n}\n"));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {6}, QVariantMap {}));
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    QVERIFY(SettingsHome::write(m_home.configPath(), external));
    QVERIFY(failed.wait(SignalTimeoutMs));
    QCOMPARE(failed.first().first().toString(), droppedMessage());
    QCOMPARE(call<QVariantMap>(session.store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt(), 11);
    QCOMPARE(session.store->property("needsSave").toBool(), false);
    QCOMPARE(session.store->property("canUndo").toBool(), false);
    QCOMPARE(SettingsHome::read(m_home.configPath()), external);
}

void TestSettingsStoreQml::pendingAutoSaveNeverOverwritesAnExternalEdit()
{
    const QString external = QStringLiteral("layout {\n    gaps 11\n}\n");
    Session session = open(QStringLiteral("layout {\n    gaps 4\n}\n"));
    session.store->setProperty("autoSave", true);
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    QSignalSpy savedSpy(session.store, SIGNAL(saved()));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {6}, QVariantMap {}));
    QVERIFY(SettingsHome::write(m_home.configPath(), external));
    QVERIFY(QMetaObject::invokeMethod(session.store, "save"));
    QCOMPARE(savedSpy.count(), 0);
    QCOMPARE(SettingsHome::read(m_home.configPath()), external);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().first().toString(), droppedMessage());
    QCOMPARE(call<QVariantMap>(session.store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt(), 11);
    QCOMPARE(session.store->property("needsSave").toBool(), false);
    QTest::qWait(SaveSettleMs);
    QCOMPARE(SettingsHome::read(m_home.configPath()), external);
    QCOMPARE(savedSpy.count(), 0);
}

void TestSettingsStoreQml::loadsAFixFromDiskOverEditsBlockedByAConfigError()
{
    Session session = open(QStringLiteral("layout {\n    gaps \"wide\"\n}\n"));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("layout/float-child-windows"), true));
    QVERIFY(!session.store->property("configError").toString().isEmpty());
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    QVERIFY(SettingsHome::write(m_home.configPath(), QStringLiteral("layout {\n    gaps 11\n}\n")));
    QVERIFY(failed.wait(SignalTimeoutMs));
    QCOMPARE(failed.last().first().toString(), droppedMessage());
    QCOMPARE(session.store->property("configError").toString(), QString());
    QCOMPARE(call<QVariantMap>(session.store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt(), 11);
    QCOMPARE(session.store->property("needsSave").toBool(), false);
    QCOMPARE(session.store->property("canUndo").toBool(), false);
}

void TestSettingsStoreQml::neverSavesOverAConfigItCannotRead()
{
    const QString text = QStringLiteral("layout {\n    gaps 4\n}\n");
    m_home.resetConfig(text);
    const QFileDevice::Permissions readable = QFile::permissions(m_home.configPath());
    QVERIFY(QFile::setPermissions(m_home.configPath(), QFileDevice::WriteOwner));
    Session session;
    m_home.create(*session.engine, "QtObject {}");
    session.store = SettingsHome::store(*session.engine);
    const QString unreadable = QStringLiteral("Could not read %1, so Settings will not save over it.").arg(m_home.configPath());
    QCOMPARE(session.store->property("configError").toString(), unreadable);
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {6}, QVariantMap {}));
    QSignalSpy failed(session.store, SIGNAL(editFailed(QString)));
    QVERIFY(QMetaObject::invokeMethod(session.store, "save"));
    QCOMPARE(failed.last().first().toString(), QStringLiteral("Not saved, the config has an error: ") + unreadable);
    QVERIFY(QFile::setPermissions(m_home.configPath(), readable));
    QCOMPARE(SettingsHome::read(m_home.configPath()), text);
    QTRY_COMPARE_WITH_TIMEOUT(session.store->property("configError").toString(), QString(), SignalTimeoutMs);
    QCOMPARE(call<QVariantMap>(session.store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt(), 4);
}

void TestSettingsStoreQml::scopeFollowsRuntimeNames()
{
    Session session = open(QStringLiteral(
        "layout { gaps 1; }\noutput \"dp-1\" { layout { gaps 2; }; }\nworkspace \"a\"\nworkspace \"Mail\" { layout { gaps 3; }; }\n"
        "monitor-profile \"p\" { layout { gaps 4; }; }\n"));
    const auto gaps
        = [&](const QString &path) { return call<QVariantMap>(session.store, "scope", path).value(QStringLiteral("gaps")).toInt(); };
    QCOMPARE(gaps(QStringLiteral("layout")), 1);
    QCOMPARE(gaps(QStringLiteral("output/layout")), 2);
    QCOMPARE(gaps(QStringLiteral("workspace#1/layout")), 3);
    QCOMPARE(gaps(QStringLiteral("workspace/layout")), 1);
    QCOMPARE(gaps(QStringLiteral("monitor-profile/layout")), 4);
    QCOMPARE(gaps(QStringLiteral("output#4/layout")), 1);
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("layout/gaps"), QVariantList {QStringLiteral("x")}, QVariantMap {}));
    QVERIFY(!session.store->property("configError").toString().isEmpty());
    QCOMPARE(gaps(QStringLiteral("layout")), 1);
    QCOMPARE(gaps(QStringLiteral("output/layout")), 2);
    Session broken = open(QStringLiteral("layout { gaps \"x\"; }\ninput { focus-follows-mouse; }\n"));
    QCOMPARE(call<QVariantMap>(broken.store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt(), 16);
    QCOMPARE(broken.store->property("values").toMap().value(QStringLiteral("input/focus-follows-mouse")), QVariant(false));
}

void TestSettingsStoreQml::helpersForPages()
{
    Session session = open(QStringLiteral("monitor-profile \"tall\" { match aspect-ratio-below=1.0; }\n"));
    const QVariantMap output {{QStringLiteral("name"), QStringLiteral("DP-1")},
        {QStringLiteral("logical"), QVariantMap {{QStringLiteral("width"), 1080}, {QStringLiteral("height"), 1920}}}};
    QCOMPARE(call<QString>(session.store, "profileForOutput", output), QStringLiteral("tall"));
    QCOMPARE(call<QString>(session.store, "keyName", QKeySequence(QStringLiteral("Meta+T"))), QStringLiteral("Mod+T"));
    QCOMPARE(call<QString>(session.store, "keyName", QKeySequence(QStringLiteral("Ctrl+Alt+Shift+Left"))),
        QStringLiteral("Ctrl+Alt+Shift+Left"));
    QCOMPARE(call<QString>(session.store, "keyName", QKeySequence()), QString());
    const QVariantMap rule {{QStringLiteral("name"), QStringLiteral("window-rule")},
        {QStringLiteral("children"),
            QVariantList {QVariantMap {{QStringLiteral("name"), QStringLiteral("match")},
                {QStringLiteral("props"), QVariantMap {{QStringLiteral("app-id"), QStringLiteral("x")}}}}}}};
    QCOMPARE(call<QVariantMap>(session.store, "checkRule", rule).value(QStringLiteral("windows")).toList().size(), 0);
    QVERIFY(call<QVariantMap>(session.store, "checkRule", QVariantMap {{QStringLiteral("name"), QStringLiteral("layout")}})
            .contains(QStringLiteral("error")));
    QCOMPARE(session.store->property("configPath").toString(), m_home.configPath());
    QCOMPARE(session.store->property("live").value<QObject *>()->property("running").toBool(), false);
}

QTEST_MAIN(TestSettingsStoreQml)
#include "test_settings_store_qml.moc"
