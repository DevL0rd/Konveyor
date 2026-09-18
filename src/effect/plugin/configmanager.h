#pragma once

#include "config/types.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

#include <expected>

namespace Konveyor
{

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);

    void start();
    QString load(const QString &path = QString());
    const Config::Config &config() const;
    std::expected<void, QString> setForceResizable(const QString &appId, bool enabled);

Q_SIGNALS:
    void configChanged(const Config::Config &config);
    void configLoaded(bool failed);

private:
    void ensureConfigFileExists() const;
    void watch(const QStringList &files);
    void notifyFailure(const QString &message) const;
    void notifyWarnings(const QStringList &warnings) const;

    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    Config::Config m_config;
    bool m_loadedOnce = false;
    QString m_path;
};

}
