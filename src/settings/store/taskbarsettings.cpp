#include "store/taskbarsettings.h"

#include <KConfigGroup>
#include <KConfigLoader>
#include <KConfigPropertyMap>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace Konveyor::Settings
{

TaskbarSettings::TaskbarSettings(QObject *parent)
    : QObject(parent)
    , m_path(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/konveyor/taskbarrc"))
    , m_config(KSharedConfig::openConfig(m_path, KConfig::SimpleConfig))
{
    QFile schema(QStringLiteral(":/konveyor/taskbar.kcfg"));
    m_loader = std::make_unique<KConfigLoader>(m_config, &schema);
    m_values = new KConfigPropertyMap(m_loader.get(), this);
    connect(m_values, &QQmlPropertyMap::valueChanged, this, [this] {
        m_values->writeConfig();
        watch();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &TaskbarSettings::reload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &TaskbarSettings::reload);
    watch();
}

TaskbarSettings::~TaskbarSettings() = default;

QQmlPropertyMap *TaskbarSettings::values() const
{
    return m_values;
}

QString TaskbarSettings::path() const
{
    return m_path;
}

bool TaskbarSettings::isDefault(const QString &key) const
{
    const KConfigSkeletonItem *item = m_loader->findItemByName(key);
    return !item || item->isDefault();
}

QVariant TaskbarSettings::defaultValue(const QString &key) const
{
    KConfigSkeletonItem *item = m_loader->findItemByName(key);
    if (!item) {
        return {};
    }
    const QVariant current = item->property();
    item->setDefault();
    const QVariant fallback = item->property();
    item->setProperty(current);
    return fallback;
}

void TaskbarSettings::reset(const QString &key)
{
    if (KConfigSkeletonItem *item = m_loader->findItemByName(key)) {
        m_config->group(item->group()).deleteEntry(item->key());
        m_config->sync();
        reload();
    }
}

void TaskbarSettings::watch()
{
    const QString directory = QFileInfo(m_path).absolutePath();
    QDir().mkpath(directory);
    if (!m_watcher.directories().contains(directory)) {
        m_watcher.addPath(directory);
    }
    if (QFile::exists(m_path) && !m_watcher.files().contains(m_path)) {
        m_watcher.addPath(m_path);
    }
}

void TaskbarSettings::reload()
{
    m_loader->load();
    for (const KConfigSkeletonItem *item : m_loader->items()) {
        if (m_values->value(item->name()) != item->property()) {
            m_values->insert(item->name(), item->property());
        }
    }
    watch();
}

}
