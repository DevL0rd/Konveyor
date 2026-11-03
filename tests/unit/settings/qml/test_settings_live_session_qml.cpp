#include "../../dbus/privatebus.h"
#include "ipc/dbusnames.h"
#include "settingsqmlharness.h"

#include <QDBusError>

using namespace Konveyor::Settings::Testing;
using Konveyor::Test::FakeService;
using Konveyor::Test::PrivateSession;

namespace
{

QString desktopFile(const QString &extra)
{
    return QStringLiteral("[Desktop Entry]\nType=Application\nExec=true\n") + extra;
}

}

class TestSettingsLiveSessionQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_bus.start());
        QVERIFY(m_home.setUp());
        const QString applications = QDir(m_home.configDir()).absoluteFilePath(QStringLiteral("../../data/applications"));
        QVERIFY(
            Konveyor::Test::writeFile(applications + QStringLiteral("/fooapp.desktop"), desktopFile("Name=Foo\nIcon=foo-icon\n").toUtf8()));
        QVERIFY(Konveyor::Test::writeFile(applications + QStringLiteral("/plainapp.desktop"), desktopFile("Name=Plain\n").toUtf8()));
        QVERIFY(Konveyor::Test::writeFile(
            applications + QStringLiteral("/hiddenapp.desktop"), desktopFile("Name=Hidden\nNoDisplay=true\n").toUtf8()));
    }

    void readsWindowsOutputsAndWorkspacesFromTheCompositor()
    {
        FakeService service(Konveyor::Ipc::dbusService, Konveyor::Ipc::dbusPath, answering({}));
        QVERIFY(service.start(m_bus.address()));
        QQmlEngine engine;
        QObject *live = liveOf(engine);
        QCOMPARE(live->property("running").toBool(), true);
        QCOMPARE(live->property("windows").toList().size(), 1);
        QCOMPARE(live->property("windows").toList().first().toMap().value(QStringLiteral("app_id")).toString(), QStringLiteral("term"));
        QCOMPARE(live->property("outputs").toList().first().toMap().value(QStringLiteral("name")).toString(), QStringLiteral("DP-1"));
        QCOMPARE(live->property("workspaces").toList().size(), 1);
    }

    void anUnansweredQuestionMeansTheCompositorIsNotThere()
    {
        for (const QString &missing : {QStringLiteral("Windows"), QStringLiteral("Outputs"), QStringLiteral("Workspaces")}) {
            FakeService service(Konveyor::Ipc::dbusService, Konveyor::Ipc::dbusPath, answering({missing}));
            QVERIFY(service.start(m_bus.address()));
            QQmlEngine engine;
            QObject *live = liveOf(engine);
            QCOMPARE(live->property("running").toBool(), false);
            QCOMPARE(live->property("windows").toList().size(), missing == QLatin1String("Windows") ? 0 : 1);
        }
    }

    void withoutACompositorNothingIsRunning()
    {
        QQmlEngine engine;
        QObject *live = liveOf(engine);
        QCOMPARE(live->property("running").toBool(), false);
        QCOMPARE(live->property("windows").toList().size(), 0);
        QCOMPARE(live->property("outputs").toList().size(), 0);
        QCOMPARE(live->property("workspaces").toList().size(), 0);
    }

    void refreshingAsksAgain()
    {
        QQmlEngine engine;
        QObject *live = liveOf(engine);
        QCOMPARE(live->property("running").toBool(), false);
        FakeService service(Konveyor::Ipc::dbusService, Konveyor::Ipc::dbusPath, answering({}));
        QVERIFY(service.start(m_bus.address()));
        QSignalSpy changed(live, SIGNAL(changed()));
        QVERIFY(QMetaObject::invokeMethod(live, "refresh"));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(live->property("running").toBool(), true);
    }

    void namesAndIconsComeFromTheDesktopFiles()
    {
        QQmlEngine engine;
        QObject *live = liveOf(engine);
        QCOMPARE(call<QString>(live, "iconFor", QStringLiteral("fooapp")), QStringLiteral("foo-icon"));
        QCOMPARE(call<QString>(live, "iconFor", QStringLiteral("FooApp")), QStringLiteral("foo-icon"));
        QCOMPARE(call<QString>(live, "iconFor", QStringLiteral("plainapp")), QStringLiteral("plainapp"));
        QCOMPARE(call<QString>(live, "iconFor", QStringLiteral("ghost")), QStringLiteral("ghost"));
        QCOMPARE(call<QString>(live, "nameFor", QStringLiteral("fooapp")), QStringLiteral("Foo"));
        QCOMPARE(call<QString>(live, "nameFor", QStringLiteral("ghost")), QStringLiteral("ghost"));
    }

    void listsTheApplicationsThatAreShownOnce()
    {
        QQmlEngine engine;
        QObject *live = liveOf(engine);
        const QVariantList applications = live->property("applications").toList();
        QStringList ids;
        for (const QVariant &application : applications) {
            ids.append(application.toMap().value(QStringLiteral("appId")).toString());
        }
        QVERIFY(ids.contains(QStringLiteral("fooapp")));
        QVERIFY(ids.contains(QStringLiteral("plainapp")));
        QVERIFY(!ids.contains(QStringLiteral("hiddenapp")));
        QVERIFY(ids.indexOf(QStringLiteral("fooapp")) < ids.indexOf(QStringLiteral("plainapp")));
        const QVariantMap foo = applications.at(ids.indexOf(QStringLiteral("fooapp"))).toMap();
        QCOMPARE(foo.value(QStringLiteral("name")).toString(), QStringLiteral("Foo"));
        QCOMPARE(foo.value(QStringLiteral("icon")).toString(), QStringLiteral("foo-icon"));
        QCOMPARE(foo.value(QStringLiteral("exec")).toString(), QStringLiteral("true"));
        QCOMPARE(live->property("applications").toList(), applications);
    }

private:
    static FakeService::Handler answering(const QStringList &failing)
    {
        return [failing](const QDBusMessage &message) {
            if (failing.contains(message.member())) {
                return message.createErrorReply(QDBusError::Failed, QStringLiteral("no"));
            }
            const QHash<QString, QString> answers {
                {QStringLiteral("Windows"), QStringLiteral(R"([{"id":1,"app_id":"term"}])")},
                {QStringLiteral("Outputs"), QStringLiteral(R"([{"name":"DP-1"}])")},
                {QStringLiteral("Workspaces"), QStringLiteral(R"([{"id":1,"output":"DP-1"}])")},
            };
            return message.createReply(answers.value(message.member()));
        };
    }

    QObject *liveOf(QQmlEngine &engine)
    {
        m_home.resetConfig(std::nullopt);
        m_home.create(engine, "QtObject {}");
        return SettingsHome::store(engine)->property("live").value<QObject *>();
    }

    PrivateSession m_bus;
    SettingsHome m_home;
};

QTEST_MAIN(TestSettingsLiveSessionQml)
#include "test_settings_live_session_qml.moc"
