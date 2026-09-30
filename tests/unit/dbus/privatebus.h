#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QDir>
#include <QMap>
#include <QMutex>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>

#include <functional>
#include <memory>

namespace Konveyor::Test
{

inline bool writeFile(const QString &path, const QByteArray &contents)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

struct ProgramResult
{
    int exitCode = -1;
    QString out;
    QString err;
};

class PrivateSession
{
public:
    PrivateSession() = default;
    PrivateSession(const PrivateSession &) = delete;
    PrivateSession &operator=(const PrivateSession &) = delete;

    ~PrivateSession()
    {
        m_daemon.terminate();
        m_daemon.waitForFinished();
    }

    bool start()
    {
        if (!m_root.isValid()) {
            return false;
        }
        const QDir root(m_root.path());
        for (const QString &name : {QStringLiteral("home"), QStringLiteral("config"), QStringLiteral("data"), QStringLiteral("state"),
                 QStringLiteral("cache"), QStringLiteral("runtime")}) {
            root.mkpath(name);
            m_dirs.insert(name, root.filePath(name));
        }
        QFile config(root.filePath(QStringLiteral("bus.conf")));
        if (!config.open(QIODevice::WriteOnly)) {
            return false;
        }
        config.write(QStringLiteral("<busconfig><type>session</type><listen>unix:path=%1</listen><auth>EXTERNAL</auth>"
                                    "<policy context=\"default\"><allow send_destination=\"*\" eavesdrop=\"true\"/>"
                                    "<allow eavesdrop=\"true\"/><allow own=\"*\"/></policy></busconfig>\n")
                .arg(root.filePath(QStringLiteral("bus")))
                .toUtf8());
        config.close();
        const QString daemon = QStandardPaths::findExecutable(QStringLiteral("dbus-daemon"));
        if (daemon.isEmpty()) {
            qWarning("dbus-daemon is not installed");
            return false;
        }
        m_daemon.start(
            daemon, {QStringLiteral("--nofork"), QStringLiteral("--print-address"), QStringLiteral("--config-file"), config.fileName()});
        if (!m_daemon.waitForReadyRead(10000)) {
            return false;
        }
        m_address = QString::fromUtf8(m_daemon.readLine()).trimmed();
        exportTo();
        return !m_address.isEmpty();
    }

    QString address() const { return m_address; }
    QString dir(const QString &name) const { return m_dirs.value(name); }

    QProcessEnvironment environment() const
    {
        QProcessEnvironment environment;
        environment.insert(QStringLiteral("PATH"), qEnvironmentVariable("PATH"));
        environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
        environment.insert(QStringLiteral("QT_FORCE_STDERR_LOGGING"), QStringLiteral("1"));
        environment.insert(QStringLiteral("LANG"), QStringLiteral("C.UTF-8"));
        environment.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"), m_address);
        environment.insert(QStringLiteral("HOME"), dir(QStringLiteral("home")));
        environment.insert(QStringLiteral("XDG_CONFIG_HOME"), dir(QStringLiteral("config")));
        environment.insert(QStringLiteral("XDG_DATA_HOME"), dir(QStringLiteral("data")));
        environment.insert(QStringLiteral("XDG_STATE_HOME"), dir(QStringLiteral("state")));
        environment.insert(QStringLiteral("XDG_CACHE_HOME"), dir(QStringLiteral("cache")));
        environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), dir(QStringLiteral("runtime")));
        return environment;
    }

    ProgramResult run(const QString &program, const QStringList &arguments, const QProcessEnvironment &environment) const
    {
        QProcess process;
        process.setProcessEnvironment(environment);
        process.setWorkingDirectory(dir(QStringLiteral("home")));
        process.start(program, arguments);
        if (!process.waitForFinished(30000)) {
            process.kill();
            process.waitForFinished();
            return {};
        }
        return {process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1, QString::fromUtf8(process.readAllStandardOutput()),
            QString::fromUtf8(process.readAllStandardError())};
    }

    ProgramResult run(const QString &program, const QStringList &arguments) const { return run(program, arguments, environment()); }

private:
    void exportTo() const
    {
        const QProcessEnvironment environment = this->environment();
        const QStringList keys = environment.keys();
        for (const QString &key : keys) {
            if (key != QLatin1String("PATH")) {
                qputenv(key.toUtf8().constData(), environment.value(key).toUtf8());
            }
        }
    }

    QTemporaryDir m_root;
    QProcess m_daemon;
    QString m_address;
    QMap<QString, QString> m_dirs;
};

class FakeService
{
public:
    using Handler = std::function<QDBusMessage(const QDBusMessage &)>;

    FakeService(const QString &name, const QString &path, Handler handler)
        : m_name(name)
        , m_path(path)
        , m_object(std::make_unique<Object>(this, std::move(handler)))
    {
        m_thread.start();
        m_object->moveToThread(&m_thread);
    }

    FakeService(const FakeService &) = delete;
    FakeService &operator=(const FakeService &) = delete;

    ~FakeService()
    {
        QMetaObject::invokeMethod(m_object.get(), [this] { disconnect(); }, Qt::BlockingQueuedConnection);
        m_thread.quit();
        m_thread.wait();
    }

    bool start(const QString &address)
    {
        bool registered = false;
        QMetaObject::invokeMethod(
            m_object.get(),
            [&] {
                QDBusConnection connection = QDBusConnection::connectToBus(address, connectionName());
                registered = connection.isConnected() && connection.registerVirtualObject(m_path, m_object.get(), QDBusConnection::SubPath)
                    && connection.registerService(m_name);
            },
            Qt::BlockingQueuedConnection);
        return registered;
    }

    QList<QDBusMessage> calls(const QString &member) const
    {
        const QMutexLocker locker(&m_mutex);
        QList<QDBusMessage> matching;
        for (const QDBusMessage &message : m_calls) {
            if (message.member() == member) {
                matching.append(message);
            }
        }
        return matching;
    }

private:
    class Object : public QDBusVirtualObject
    {
    public:
        Object(FakeService *service, Handler handler)
            : m_service(service)
            , m_handler(std::move(handler))
        { }

        QString introspect(const QString &) const override { return {}; }

        bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
        {
            if (message.interface() == QLatin1String("org.freedesktop.DBus.Introspectable")) {
                connection.send(message.createReply(QStringLiteral("<node/>")));
                return true;
            }
            {
                const QMutexLocker locker(&m_service->m_mutex);
                m_service->m_calls.append(message);
            }
            connection.send(m_handler(message));
            return true;
        }

    private:
        FakeService *m_service;
        Handler m_handler;
    };

    QString connectionName() const { return QStringLiteral("fake-") + m_name; }

    void disconnect() const
    {
        QDBusConnection connection(connectionName());
        connection.unregisterObject(m_path);
        connection.unregisterService(m_name);
        QDBusConnection::disconnectFromBus(connectionName());
    }

    QString m_name;
    QString m_path;
    QThread m_thread;
    mutable QMutex m_mutex;
    QList<QDBusMessage> m_calls;
    std::unique_ptr<Object> m_object;
};

}
