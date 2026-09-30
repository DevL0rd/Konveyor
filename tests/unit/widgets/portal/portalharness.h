#pragma once

#include "fakeplasmoid.h"

#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QQmlContext>
#include <QTemporaryDir>

#include <cstdio>
#include <memory>

namespace Konveyor::Test
{

inline QStringList &portalWarnings()
{
    static QStringList warnings;
    return warnings;
}

inline void collectPortalWarnings(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (type != QtDebugMsg && type != QtInfoMsg && message != QLatin1String("Could not find any platform plugin")) {
        portalWarnings().append(message);
    }
}

class PortalHarness
{
public:
    enum FormFactor
    {
        Planar = 0,
        Horizontal = 2,
        Vertical = 3,
    };

    PortalHarness()
    {
        qputenv("QML_XHR_ALLOW_FILE_READ", "1");
        KLocalizedString::setApplicationDomain("plasma_applet_org.devl0rd.portal");
        registerFakePlasmoid();
        portalWarnings().clear();
        m_previousHandler = qInstallMessageHandler(collectPortalWarnings);
    }

    PortalHarness(const PortalHarness &) = delete;
    PortalHarness &operator=(const PortalHarness &) = delete;

    ~PortalHarness()
    {
        m_root.reset();
        m_engine.reset();
        FakePlasmoidAttached::instance().config = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

    bool load(const QString &plasmoid, FormFactor formFactor, const QVariantMap &settings = {})
    {
        const QString source = QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/plasmoids/") + plasmoid + QStringLiteral("/contents");
        const QDir ui(m_directory.filePath(QStringLiteral("ui")));
        if (!copyTree(source + QStringLiteral("/ui"), ui.path())
            || !copyTree(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/shared/common"), ui.filePath(QStringLiteral("lib")))
            || !copyTree(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/shared/lib"), ui.filePath(QStringLiteral("lib")))) {
            return false;
        }
        for (auto it = m_stubs.cbegin(); it != m_stubs.cend(); ++it) {
            QFile stub(ui.filePath(it.key()));
            if (!stub.open(QIODevice::WriteOnly) || stub.write(it.value()) != it.value().size()) {
                return false;
            }
        }
        m_schema = std::make_unique<QFile>(source + QStringLiteral("/config/main.xml"));
        m_loader = std::make_unique<KConfigLoader>(
            KSharedConfig::openConfig(m_directory.filePath(QStringLiteral("plasmoidrc")), KConfig::SimpleConfig), m_schema.get());
        m_config = std::make_unique<KConfigPropertyMap>(m_loader.get());
        for (auto it = settings.cbegin(); it != settings.cend(); ++it) {
            m_config->insert(it.key(), it.value());
        }
        FakePlasmoidAttached &attached = FakePlasmoidAttached::instance();
        attached.config = m_config.get();
        attached.formFactor = formFactor;
        attached.requestedActions.clear();
        attached.action.triggered = false;
        m_engine = std::make_unique<QQmlEngine>();
        m_engine->addImportPath(QStringLiteral(KONVEYOR_SOURCE_DIR "/tests/unit/widgets/portal/doubles"));
        m_engine->rootContext()->setContextObject(new KLocalizedQmlContext(m_engine.get()));
        QQmlComponent component(m_engine.get(), QUrl::fromLocalFile(ui.filePath(QStringLiteral("main.qml"))));
        m_root.reset(component.create());
        if (!m_root) {
            std::fputs(qPrintable(component.errorString()), stderr);
        }
        return m_root != nullptr;
    }

    void stub(const QString &file, const QByteArray &contents) { m_stubs.insert(file, contents); }

    QObject *root() const { return m_root.get(); }
    KConfigPropertyMap *config() const { return m_config.get(); }
    QString runtimeFile(const QString &name) const { return m_directory.filePath(name); }

    QList<QObject *> dataSources() const
    {
        QList<QObject *> sources;
        const QList<QObject *> children = m_root->findChildren<QObject *>(QStringLiteral("dataSource"));
        for (QObject *child : children) {
            sources.append(child);
        }
        return sources;
    }

    QStringList requested() const
    {
        QStringList all;
        const QList<QObject *> sources = dataSources();
        for (QObject *source : sources) {
            all.append(source->property("requested").toStringList());
        }
        return all;
    }

    static bool answer(QObject *source, const QString &name, const QString &output)
    {
        const QVariant data = QVariantMap {{QStringLiteral("exit code"), 0}, {QStringLiteral("stdout"), output}};
        return QMetaObject::invokeMethod(source, "newData", Q_ARG(QString, name), Q_ARG(QVariant, data));
    }

    QVariant call(const char *function, const QVariant &argument = {}) const
    {
        QVariant result;
        if (argument.isValid()) {
            QMetaObject::invokeMethod(m_root.get(), function, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, argument));
        } else {
            QMetaObject::invokeMethod(m_root.get(), function, Q_RETURN_ARG(QVariant, result));
        }
        return result;
    }

private:
    static bool copyTree(const QString &from, const QString &to)
    {
        QDirIterator it(from, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            const QString target = to + QLatin1Char('/') + QDir(from).relativeFilePath(file);
            if (!QDir().mkpath(QFileInfo(target).path()) || !QFile::copy(file, target)) {
                return false;
            }
        }
        return true;
    }

    QtMessageHandler m_previousHandler = nullptr;
    QTemporaryDir m_directory;
    QMap<QString, QByteArray> m_stubs;
    std::unique_ptr<QFile> m_schema;
    std::unique_ptr<KConfigLoader> m_loader;
    std::unique_ptr<KConfigPropertyMap> m_config;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_root;
};

}
