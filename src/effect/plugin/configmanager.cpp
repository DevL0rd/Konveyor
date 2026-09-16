#include "plugin/configmanager.h"

#include "config/loader.h"

#include <KNotification>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace Konveyor
{

namespace
{

constexpr int reloadDebounceMs = 50;

QString bundledDefaultConfig()
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konveyor/default-config.kdl"));
}

void sendNotification(const QString &title, const QString &text)
{
    auto *notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout);
    notification->setComponentName(QStringLiteral("plasma_workspace"));
    notification->setTitle(title);
    notification->setText(text);
    notification->setIconName(QStringLiteral("preferences-system-windows-effect"));
    notification->sendEvent();
}

}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_config(Config::defaultConfig())
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(reloadDebounceMs);
    connect(&m_debounce, &QTimer::timeout, this, [this]() { load(); });
    const auto schedule = [this]() { m_debounce.start(); };
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, schedule);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, schedule);
}

void ConfigManager::start()
{
    m_path = Config::configPath();
    ensureConfigFileExists();
    load();
}

QString ConfigManager::load(const QString &path)
{
    const QString target = path.isEmpty() ? m_path : path;
    auto result = Config::loadFile(target);
    if (!result) {
        const QString message = result.error().toString();
        notifyFailure(message);
        watch({target});
        if (!m_loadedOnce) {
            Q_EMIT configChanged(m_config);
        }
        Q_EMIT configLoaded(true);
        return message;
    }
    m_loadedOnce = true;
    watch(result->files);
    notifyWarnings(result->warnings);
    m_config = std::move(result->config);
    Q_EMIT configChanged(m_config);
    Q_EMIT configLoaded(false);
    return QString();
}

const Config::Config &ConfigManager::config() const
{
    return m_config;
}

void ConfigManager::ensureConfigFileExists() const
{
    if (QFileInfo::exists(m_path)) {
        return;
    }
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    const QString source = bundledDefaultConfig();
    if (source.isEmpty() || !QFile::copy(source, m_path)) {
        notifyFailure(QStringLiteral("Could not create %1 from the bundled default config.").arg(m_path));
        return;
    }
    QFile::setPermissions(m_path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
}

void ConfigManager::watch(const QStringList &files)
{
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
    }
    QStringList directories;
    for (const QString &file : files) {
        directories.append(QFileInfo(file).absolutePath());
    }
    directories.removeDuplicates();
    m_watcher.addPaths(directories);
    const QStringList existing = [&files]() {
        QStringList result;
        std::ranges::copy_if(files, std::back_inserter(result), [](const QString &file) { return QFileInfo::exists(file); });
        return result;
    }();
    if (!existing.isEmpty()) {
        m_watcher.addPaths(existing);
    }
}

void ConfigManager::notifyFailure(const QString &message) const
{
    if (m_config.configNotificationDisableFailed) {
        return;
    }
    sendNotification(QStringLiteral("Konveyor: failed to load config"), message);
}

void ConfigManager::notifyWarnings(const QStringList &warnings) const
{
    if (!warnings.isEmpty()) {
        qInfo().noquote() << "konveyor: config warnings:\n" << warnings.join(QLatin1Char('\n'));
    }
}

}
