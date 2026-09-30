#pragma once

#define TRY_VERIFY(expression) QTRY_VERIFY_WITH_TIMEOUT(expression, 30000)
#define TRY_COMPARE(actual, expected) QTRY_COMPARE_WITH_TIMEOUT(actual, expected, 30000)

#include "launchersupport.h"

#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <memory>

namespace LauncherTest
{

class UrlRecorder : public QObject
{
    Q_OBJECT

public:
    QList<QUrl> opened;

public Q_SLOTS:
    void open(const QUrl &url) { opened.append(url); }
};

inline UrlRecorder &urls()
{
    static UrlRecorder recorder;
    return recorder;
}

class Harness
{
public:
    bool stage()
    {
        if (!m_root.isValid()) {
            return false;
        }
        const QDir root(m_root.path());
        for (const char *name : {"home", "config", "data", "state", "cache", "runtime", "dirs/konveyor", "qml/org/kde/konveyor"}) {
            if (!root.mkpath(QLatin1String(name))) {
                return false;
            }
        }
        qputenv("HOME", root.filePath(QStringLiteral("home")).toUtf8());
        qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/konveyor-test-bus");
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("XDG_CONFIG_HOME", root.filePath(QStringLiteral("config")).toUtf8());
        qputenv("XDG_DATA_HOME", root.filePath(QStringLiteral("data")).toUtf8());
        qputenv("XDG_STATE_HOME", root.filePath(QStringLiteral("state")).toUtf8());
        qputenv("XDG_CACHE_HOME", root.filePath(QStringLiteral("cache")).toUtf8());
        qputenv("XDG_RUNTIME_DIR", root.filePath(QStringLiteral("runtime")).toUtf8());
        qputenv("XDG_DATA_DIRS", root.filePath(QStringLiteral("dirs")).toUtf8() + ":/usr/local/share:/usr/share");
        const QString ui = root.filePath(QStringLiteral("ui"));
        const QString lib = QDir(ui).filePath(QStringLiteral("lib"));
        return QFile::copy(source("data/default-config.kdl"), root.filePath(QStringLiteral("dirs/konveyor/default-config.kdl")))
            && QFile::link(QStringLiteral(KONVEYOR_BINARY_DIR "/bin/org/kde/konveyor/settings"),
                root.filePath(QStringLiteral("qml/org/kde/konveyor/settings")))
            && QFile::link(source("src/components"), root.filePath(QStringLiteral("qml/org/kde/konveyor/components")))
            && copyTree(source("widgets/portals/shared/launcher"), ui) && copyTree(source("widgets/portals/shared/lib"), lib)
            && copyTree(source("widgets/shared/common"), lib)
            && QFile::copy(source("widgets/shared/MonitorOverlay.qml"), QDir(lib).filePath(QStringLiteral("MonitorOverlay.qml")))
            && copyTree(source("widgets/portals/kontrol-panel"), ui) && copyTree(source("tests/unit/widgets/launcher"), ui);
    }

    QString path(const QString &relative) const { return QDir(m_root.path()).filePath(relative); }

    QString uiFile(const QString &name) const { return path(QStringLiteral("ui/") + name); }

    QQmlEngine &engine()
    {
        if (!m_engine) {
            m_engine = std::make_unique<QQmlEngine>();
            m_engine->addImportPath(source("tests/unit/widgets/launcher/doubles"));
            m_engine->addImportPath(path(QStringLiteral("qml")));
            KLocalizedString::setApplicationDomain("plasma_applet_org.devl0rd.portal.launcher");
            m_engine->rootContext()->setContextObject(new KLocalizedQmlContext(m_engine.get()));
        }
        return *m_engine;
    }

    KConfigPropertyMap *loadConfig(const QString &schema, const QString &file)
    {
        m_schema = std::make_unique<QFile>(schema);
        m_loader = std::make_unique<KConfigLoader>(KSharedConfig::openConfig(path(file)), m_schema.get());
        m_config = std::make_unique<KConfigPropertyMap>(m_loader.get());
        m_config->setNotify(true);

        QObject::connect(m_config.get(), &QQmlPropertyMap::valueChanged, m_config.get(), [this] { m_config->writeConfig(); });
        return m_config.get();
    }

    QObject *create(const QString &file, const QVariantMap &properties)
    {
        QQmlComponent component(&engine(), QUrl::fromLocalFile(QDir::isAbsolutePath(file) ? file : uiFile(file)));
        std::unique_ptr<QObject> object(component.createWithInitialProperties(properties));
        if (!object) {
            warnings().append(component.errorString());
            return nullptr;
        }
        if (auto *item = qobject_cast<QQuickItem *>(object.get())) {
            m_window = std::make_unique<QQuickWindow>();
            m_window->resize(qRound(item->width()), qRound(item->height()));
            item->setParentItem(m_window->contentItem());
            m_window->show();
        }
        m_object = std::move(object);

        return m_object.get();
    }

    QObject *openHost(bool portal, const QVariantMap &settings = {})
    {
        QObject *root = host(portal, settings);
        if (!root) {
            return nullptr;
        }
        root->setProperty("open", true);
        if (!QTest::qWaitFor([this] { return view()->property("progress").toDouble() == 1.0; }, 30000)
            || !QTest::qWaitForWindowActive(m_window.get())) {
            return nullptr;
        }
        return root;
    }

    QQuickWindow *window() const { return m_window.get(); }

    QObject *host(bool portal, const QVariantMap &settings = {})
    {
        const QString schema = portal ? source("widgets/portals/plasmoids/org.devl0rd.portal/contents/config/main.xml")
                                      : source("widgets/portals/kontrol-panel/config/main.xml");
        KConfigPropertyMap *config
            = loadConfig(schema, portal ? QStringLiteral("config/portalrc") : QStringLiteral("config/kontrolpanelrc"));
        for (auto it = settings.cbegin(); it != settings.cend(); ++it) {
            config->insert(it.key(), it.value());
        }
        return create(QStringLiteral("LauncherHost.qml"),
            {{QStringLiteral("config"), QVariant::fromValue<QObject *>(config)}, {QStringLiteral("portal"), portal}});
    }

    QObject *root() const { return m_object.get(); }

    QObject *view() const { return m_object ? m_object->property("view").value<QObject *>() : nullptr; }

    QObject *data() const { return find(QStringLiteral("LauncherData")); }

    QObject *find(const QString &typePrefix) const
    {
        const QList<QObject *> all = m_object ? m_object->findChildren<QObject *>() : QList<QObject *>();
        for (QObject *object : all) {
            if (QString::fromLatin1(object->metaObject()->className()).startsWith(typePrefix + QLatin1Char('_'))) {
                return object;
            }
        }
        return nullptr;
    }

    QObject *singleton(const char *uri, const char *name)
    {
        return engine().singletonInstance<QObject *>(QString::fromLatin1(uri), QString::fromLatin1(name));
    }

    QStringList commands() { return singleton("org.kde.plasma.plasma5support", "Commands")->property("log").toStringList(); }

    QString friendsPath() const { return path(QStringLiteral("runtime/Plasma-App-Portal/friends.json")); }

    bool writeFile(const QString &file, const QByteArray &contents)
    {
        QDir().mkpath(QFileInfo(file).absolutePath());
        QFile out(file + QStringLiteral(".tmp"));
        return out.open(QIODevice::WriteOnly) && out.write(contents) == contents.size() && out.flush()
            && (!QFile::exists(file) || QFile::remove(file)) && out.rename(file);
    }

    void respondWithLibrary(const QVariantList &extra = {})
    {
        QVariantList responses = extra;
        responses.append(response(QStringLiteral("^\\$HOME/\\.local/bin/portal-games # "), fixture("games.json")));
        responses.append(response(QStringLiteral("^printf "), friendsPath().toUtf8()));
        respond(responses);
    }

    bool waitHandled(const QString &prefix, int times = 1)
    {
        QObject *commands = singleton("org.kde.plasma.plasma5support", "Commands");
        return QTest::qWaitFor(
            [commands, &prefix, times] {
                const QStringList handled = commands->property("handled").toStringList();
                return std::count_if(handled.cbegin(), handled.cend(), [&prefix](const QString &command) {
                    return command.startsWith(prefix);
                }) >= times;
            },
            30000);
    }

    void respond(const QVariantList &responses)
    {
        singleton("org.kde.plasma.plasma5support", "Commands")->setProperty("responses", responses);
    }

    QJSValue call(QObject *object, const char *function, const QJSValueList &arguments = {})
    {
        QJSValue callable = engine().toScriptValue(object).property(QLatin1String(function));
        QJSValue result = callable.callWithInstance(engine().toScriptValue(object), arguments);
        if (result.isError()) {
            warnings().append(result.toString());
        }
        return result;
    }

    QVariant eval(const QString &expression) { return evalIn(qmlContext(data()), view(), expression); }

    QVariant evalData(const QString &expression)
    {
        const QList<QObject *> children = data()->children();
        return children.isEmpty() ? QVariant() : evalIn(qmlContext(children.first()), data(), expression);
    }

    QVariant evalIn(QQmlContext *context, QObject *scope, const QString &expression)
    {
        QQmlExpression script(context, scope, expression);
        const QVariant result = script.evaluate();
        if (script.hasError()) {
            warnings().append(script.error().toString());
        }
        return result.canConvert<QJSValue>() ? result.value<QJSValue>().toVariant() : result;
    }

    KConfigPropertyMap *config() const { return m_config.get(); }

    void reset()
    {
        m_object.reset();
        m_window.reset();
        m_engine.reset();
        m_config.reset();
        m_loader.reset();
        m_schema.reset();
        for (const char *file : {"config/portalrc", "config/kontrolpanelrc"}) {
            QFile::remove(path(QLatin1String(file)));
        }
        QDir(path(QStringLiteral("data/Plasma-App-Portal"))).removeRecursively();
        QDir(path(QStringLiteral("runtime/Plasma-App-Portal"))).removeRecursively();
        urls().opened.clear();
    }

private:
    QTemporaryDir m_root {QDir::tempPath() + QStringLiteral("/konveyor-launcher-XXXXXX")};
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QFile> m_schema;
    std::unique_ptr<KConfigLoader> m_loader;
    std::unique_ptr<KConfigPropertyMap> m_config;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<QObject> m_object;
};

inline Harness &harness()
{
    static Harness instance;
    return instance;
}

class TestCase : public QObject
{
    Q_OBJECT

protected:
    Harness &m_harness = harness();

    QVariant eval(const QString &expression) { return m_harness.eval(expression); }

    QVariant page(const QString &expression) { return m_harness.eval(QStringLiteral("launcher.currentView().") + expression); }

    QObject *openLibrary(bool portal, const QVariantMap &settings = {})
    {
        m_harness.respondWithLibrary();
        if (!m_harness.writeFile(m_harness.friendsPath(), fixture("friends.json"))) {
            return nullptr;
        }
        QObject *host = m_harness.openHost(portal, settings);
        if (host
            && !QTest::qWaitFor(
                [this] { return eval(QStringLiteral("launcherData.games.length + launcherData.friends.length")).toInt() == 8; }, 30000)) {
            return nullptr;
        }
        return host;
    }

    bool goTo(const QString &key)
    {
        eval(QStringLiteral("launcher.goToPage('%1')").arg(key));
        return QTest::qWaitFor([this] { return eval(QStringLiteral("launcher.currentView() !== null")).toBool(); }, 30000);
    }

private Q_SLOTS:
    void init() { captureWarnings(); }

    void cleanup()
    {
        m_harness.reset();
        QVERIFY2(warnings().isEmpty(), qPrintable(warnings().join(QLatin1Char('\n'))));
    }
};

}

#define LAUNCHER_TEST_MAIN(TestObject)                                                                                                     \
    int main(int argc, char *argv[])                                                                                                       \
    {                                                                                                                                      \
        if (!LauncherTest::harness().stage()) {                                                                                            \
            return 1;                                                                                                                      \
        }                                                                                                                                  \
        QGuiApplication application(argc, argv);                                                                                           \
        for (const char *scheme : {"steam", "http", "https", "file", "trash", "applications"}) {                                           \
            QDesktopServices::setUrlHandler(QLatin1String(scheme), &LauncherTest::urls(), "open");                                         \
        }                                                                                                                                  \
        TestObject test;                                                                                                                   \
        return QTest::qExec(&test, argc, argv);                                                                                            \
    }
