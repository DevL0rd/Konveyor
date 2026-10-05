#include "plugin/configmanager.h"

#include "config/forceresizable.h"
#include "config/loader.h"
#include "config/log.h"
#include "plugin/notifications.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

#include <memory>
#include <vector>

namespace Konveyor
{

namespace
{

constexpr int reloadDebounceMs = 50;
constexpr QLatin1StringView forceResizableFileName("force-resizable.kdl");

QString bundledDefaultConfig()
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konveyor/default-config.kdl"));
}

std::expected<QString, QString> readText(const QString &path, bool allowMissing)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (allowMissing && !file.exists()) {
            return QString();
        }
        return std::unexpected(QStringLiteral("Could not read %1: %2").arg(path, file.errorString()));
    }
    return QString::fromUtf8(file.readAll());
}

std::expected<void, QString> writeTexts(const QList<std::pair<QString, QString>> &files)
{
    std::vector<std::unique_ptr<QSaveFile>> pending;
    for (const auto &[path, text] : files) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        auto file = std::make_unique<QSaveFile>(path);
        if (!file->open(QIODevice::WriteOnly | QIODevice::Text) || file->write(text.toUtf8()) < 0) {
            return std::unexpected(QStringLiteral("Could not write %1: %2").arg(path, file->errorString()));
        }
        pending.push_back(std::move(file));
    }
    for (const std::unique_ptr<QSaveFile> &file : pending) {
        if (!file->commit()) {
            return std::unexpected(QStringLiteral("Could not write %1: %2").arg(file->fileName(), file->errorString()));
        }
    }
    return {};
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
        qCInfo(lcKonveyor).noquote() << "konveyor: config" << target << "failed to load:" << message;
        notifyFailure(message);
        if (target == m_path) {
            watch(result.error().files);
        }
        if (!m_loadedOnce) {
            Q_EMIT configChanged(m_config);
        }
        Q_EMIT configLoaded(true);
        return message;
    }
    m_loadedOnce = true;
    qCInfo(lcKonveyor).noquote() << "konveyor: config loaded from" << target << "files:" << result->files.join(QStringLiteral(", "));
    m_path = target;
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

std::expected<void, QString> ConfigManager::setForceResizable(const QString &appId, bool enabled)
{
    const QString overridePath = QDir(QFileInfo(m_path).absolutePath()).filePath(forceResizableFileName);
    const auto overrideText = readText(overridePath, true);
    if (!overrideText) {
        return std::unexpected(overrideText.error());
    }
    const auto updatedOverride = Config::setForceResizableRule(*overrideText, overridePath, appId, enabled);
    if (!updatedOverride) {
        return std::unexpected(updatedOverride.error());
    }
    const auto mainText = readText(m_path, false);
    if (!mainText) {
        return std::unexpected(mainText.error());
    }
    const auto updatedMain = Config::ensureForceResizableInclude(*mainText, m_path);
    if (!updatedMain) {
        return std::unexpected(updatedMain.error());
    }
    QList<std::pair<QString, QString>> files {{overridePath, *updatedOverride}};
    if (*updatedMain != *mainText) {
        files.append({m_path, *updatedMain});
    }
    if (const auto written = writeTexts(files); !written) {
        return written;
    }
    const QString error = load();
    if (!error.isEmpty()) {
        return std::unexpected(error);
    }
    return {};
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
    QStringList wanted;
    for (const QString &file : files) {
        wanted.append(QFileInfo(file).absolutePath());
    }
    std::ranges::copy_if(files, std::back_inserter(wanted), [](const QString &file) { return QFileInfo::exists(file); });
    wanted.removeDuplicates();
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    QStringList unwanted;
    std::ranges::copy_if(watched, std::back_inserter(unwanted), [&wanted](const QString &path) { return !wanted.contains(path); });
    QStringList missing;
    std::ranges::copy_if(wanted, std::back_inserter(missing), [&watched](const QString &path) { return !watched.contains(path); });
    if (!unwanted.isEmpty()) {
        m_watcher.removePaths(unwanted);
    }
    if (!missing.isEmpty()) {
        m_watcher.addPaths(missing);
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
