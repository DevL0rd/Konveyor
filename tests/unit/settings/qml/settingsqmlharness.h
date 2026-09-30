#pragma once

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace Konveyor::Settings::Testing
{

constexpr int SignalTimeoutMs = 30000;

class SettingsHome
{
public:
    bool setUp()
    {
        const QDir home(m_home.path());
        if (!m_home.isValid() || !home.mkpath(QStringLiteral("config/konveyor")) || !home.mkpath(QStringLiteral("data/konveyor"))
            || !home.mkpath(QStringLiteral("qml/org/kde/konveyor"))) {
            return false;
        }
        const bool linked = QFile::link(QStringLiteral(KONVEYOR_BINARY_DIR "/bin/org/kde/konveyor/settings"),
                                home.filePath(QStringLiteral("qml/org/kde/konveyor/settings")))
            && QFile::link(
                QStringLiteral(KONVEYOR_SOURCE_DIR "/src/components"), home.filePath(QStringLiteral("qml/org/kde/konveyor/components")));
        qputenv("HOME", m_home.path().toUtf8());
        qputenv("XDG_CONFIG_HOME", home.filePath(QStringLiteral("config")).toUtf8());
        qputenv("XDG_DATA_DIRS", home.filePath(QStringLiteral("data")).toUtf8());
        qunsetenv("KONVEYOR_CONFIG");
        return linked && installDefaults();
    }

    bool installDefaults() const
    {
        const QString target = dataPath();
        QFile::remove(target);
        return QFile::copy(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"), target);
    }

    QString dataPath() const { return QDir(m_home.path()).filePath(QStringLiteral("data/konveyor/default-config.kdl")); }
    QString configDir() const { return QDir(m_home.path()).filePath(QStringLiteral("config/konveyor")); }
    QString configPath() const { return QDir(configDir()).filePath(QStringLiteral("config.kdl")); }
    QString sourcePath(const QString &relative) const { return QStringLiteral(KONVEYOR_SOURCE_DIR "/src/settings/qml/") + relative; }

    static QString read(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    static bool write(const QString &path, const QString &text)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(text.toUtf8()) == text.toUtf8().size();
    }

    void resetConfig(const std::optional<QString> &text) const
    {
        const QDir dir(configDir());
        for (const QString &entry : dir.entryList(QDir::Files)) {
            QFile::remove(dir.filePath(entry));
        }
        if (text) {
            write(configPath(), *text);
        }
    }

    std::unique_ptr<QObject> create(QQmlEngine &engine, const QByteArray &body, QString *error = nullptr) const
    {
        engine.addImportPath(QDir(m_home.path()).filePath(QStringLiteral("qml")));
        QQmlComponent component(&engine);
        component.setData(
            "import QtQuick\nimport org.kde.konveyor.settings\n" + body, QUrl::fromLocalFile(sourcePath(QStringLiteral("inline.qml"))));
        if (component.isLoading()) {
            QSignalSpy ready(&component, &QQmlComponent::statusChanged);
            ready.wait(SignalTimeoutMs);
        }
        std::unique_ptr<QObject> object(component.isReady() ? component.create() : nullptr);
        if (error) {
            *error = component.errorString();
        }
        return object;
    }

    static QObject *store(QQmlEngine &engine)
    {
        return engine.singletonInstance<QObject *>(QStringLiteral("org.kde.konveyor.settings"), QStringLiteral("SettingsStore"));
    }

private:
    QTemporaryDir m_home;
};

class WarningLog
{
public:
    WarningLog()
    {
        s_current = this;
        m_previous = qInstallMessageHandler(&WarningLog::handle);
    }

    ~WarningLog()
    {
        qInstallMessageHandler(m_previous);
        s_current = nullptr;
    }

    WarningLog(const WarningLog &) = delete;
    WarningLog &operator=(const WarningLog &) = delete;

    QStringList take() { return std::exchange(m_messages, {}); }

private:
    static void handle(QtMsgType type, const QMessageLogContext &context, const QString &message)
    {
        static const QString kf6I18nThreadWarning
            = QStringLiteral("QObject::installEventFilter(): Cannot filter events for objects in a different thread.");
        if (s_current && type != QtDebugMsg && type != QtInfoMsg && message != kf6I18nThreadWarning) {
            s_current->m_messages.append(message);
        }
        if (s_current && s_current->m_previous) {
            s_current->m_previous(type, context, message);
        }
    }

    static inline WarningLog *s_current = nullptr;
    QtMessageHandler m_previous = nullptr;
    QStringList m_messages;
};

template<typename Result, typename... Args> Result call(QObject *object, const char *method, Args &&...arguments)
{
    Result result {};
    if (!QMetaObject::invokeMethod(object, method, qReturnArg(result), std::forward<Args>(arguments)...)) {
        qWarning("could not call %s", method);
    }
    return result;
}

inline bool saveAndWait(QObject *store)
{
    QSignalSpy saved(store, SIGNAL(saved()));
    QMetaObject::invokeMethod(store, "save");
    return saved.count() > 0 || saved.wait(SignalTimeoutMs);
}

constexpr QByteArrayView ScriptHost = R"(
import "catalog/Kdl.js" as Kdl
import "catalog/RuleSummary.js" as RuleSummary
import "catalog/MotionMath.js" as MotionMath
import "sections/LayoutKeys.js" as LayoutKeys
import "sections/CornerRule.js" as CornerRule
import "components/rules/RulePaint.js" as RulePaint
import "components/monitors/MonitorSummary.js" as MonitorSummary
QtObject {
    readonly property var libraries: ({ Kdl: Kdl, RuleSummary: RuleSummary, MotionMath: MotionMath, LayoutKeys: LayoutKeys,
        CornerRule: CornerRule, RulePaint: RulePaint, MonitorSummary: MonitorSummary })
    function run(library, name, args) {
        const result = libraries[library][name].apply(null, args.map(arg => arg === "@store" ? SettingsStore : arg));
        return JSON.stringify(result === undefined ? null : result);
    }
    function color(text) { return Qt.color(text) }
}
)";

inline QVariant script(QObject *host, const QString &library, const QString &name, const QVariantList &arguments)
{
    QVariant json;
    QMetaObject::invokeMethod(
        host, "run", Q_RETURN_ARG(QVariant, json), Q_ARG(QVariant, library), Q_ARG(QVariant, name), Q_ARG(QVariant, arguments));
    return QJsonDocument::fromJson(QByteArray("[") + json.toString().toUtf8() + "]").array().first().toVariant();
}

}
