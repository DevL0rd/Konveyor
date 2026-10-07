#pragma once

#include <KSharedConfig>

#include <QFileSystemWatcher>
#include <QObject>
#include <QQmlPropertyMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

class KConfigLoader;
class KConfigPropertyMap;

namespace Konveyor::Settings
{

class TaskbarSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QQmlPropertyMap *values READ values CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)

public:
    explicit TaskbarSettings(QObject *parent = nullptr);
    ~TaskbarSettings() override;

    QQmlPropertyMap *values() const;
    QString path() const;

    Q_INVOKABLE bool isDefault(const QString &key) const;
    Q_INVOKABLE QVariant defaultValue(const QString &key) const;
    Q_INVOKABLE void reset(const QString &key);

private:
    void watch();
    void reload();

    QString m_path;
    KSharedConfigPtr m_config;
    std::unique_ptr<KConfigLoader> m_loader;
    KConfigPropertyMap *m_values = nullptr;
    QFileSystemWatcher m_watcher;
};

}
