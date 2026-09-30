#include "plasmadoubles.h"

#include <QQmlEngine>

QPointer<PlasmoidDouble> PlasmoidAttachedType::current;

PlasmoidDouble::PlasmoidDouble(const QString &pluginId, const QString &iconName, QObject *parent)
    : QObject(parent)
    , pluginName(pluginId)
    , metaData({{QStringLiteral("pluginId"), pluginId}, {QStringLiteral("iconName"), iconName}})
    , m_configuration(new ConfigDouble(this))
{ }

QObject *PlasmoidDouble::internalAction(const QString &name)
{
    ActionDouble *&action = m_actions[name];
    if (!action) {
        action = new ActionDouble(this);
    }
    return action;
}

PlasmoidDouble *PlasmoidAttachedType::qmlAttachedProperties(QObject *)
{
    return current.data();
}

DataSourceDouble::DataSourceDouble(QObject *parent)
    : QObject(parent)
{
    instances().append(this);
}

DataSourceDouble::~DataSourceDouble()
{
    instances().removeAll(this);
}

QList<DataSourceDouble *> &DataSourceDouble::instances()
{
    static QList<DataSourceDouble *> list;
    return list;
}

QStringList &DataSourceDouble::commands()
{
    static QStringList list;
    return list;
}

void DataSourceDouble::setConnectedSources(const QStringList &sources)
{
    m_sources.clear();
    for (const QString &source : sources) {
        connectSource(source);
    }
    Q_EMIT connectedSourcesChanged();
}

void DataSourceDouble::connectSource(const QString &source)
{
    if (m_sources.contains(source)) {
        return;
    }
    m_sources.append(source);
    commands().append(source);
    Q_EMIT connectedSourcesChanged();
}

void DataSourceDouble::disconnectSource(const QString &source)
{
    if (m_sources.removeAll(source) > 0) {
        Q_EMIT connectedSourcesChanged();
    }
}

void DataSourceDouble::reply(const QString &source, const QString &out, int exitCode, const QString &err)
{
    Q_EMIT newData(source,
        {{QStringLiteral("stdout"), out}, {QStringLiteral("stderr"), err}, {QStringLiteral("exit code"), exitCode},
            {QStringLiteral("exit status"), 0}});
}

TasksModelDouble::TasksModelDouble(QObject *parent)
    : QAbstractListModel(parent)
{
    instances().append(this);
}

TasksModelDouble::~TasksModelDouble()
{
    instances().removeAll(this);
}

QList<TasksModelDouble *> &TasksModelDouble::instances()
{
    static QList<TasksModelDouble *> list;
    return list;
}

int TasksModelDouble::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_tasks.size());
}

QVariant TasksModelDouble::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.size()) {
        return {};
    }
    const auto &task = m_tasks.at(index.row());
    if (role == AbstractTasksModelDouble::AppPid) {
        return task.first;
    }
    if (role == AbstractTasksModelDouble::AppName || role == Qt::DisplayRole) {
        return task.second;
    }
    return {};
}

QModelIndex TasksModelDouble::activeTask() const
{
    return m_active < 0 ? QModelIndex() : index(m_active, 0);
}

void TasksModelDouble::activate(int pid, const QString &name)
{
    beginInsertRows(QModelIndex(), int(m_tasks.size()), int(m_tasks.size()));
    m_tasks.append({pid, name});
    endInsertRows();
    m_active = int(m_tasks.size()) - 1;
    Q_EMIT countChanged();
    Q_EMIT activeTaskChanged();
}

void TasksModelDouble::clearActive()
{
    m_active = -1;
    Q_EMIT activeTaskChanged();
    Q_EMIT countChanged();
}

void SessionBusDouble::asyncCall(const QVariantMap &message)
{
    calls().append(message);
}

QList<QVariantMap> &SessionBusDouble::calls()
{
    static QList<QVariantMap> list;
    return list;
}

SignalWatcherDouble::SignalWatcherDouble(QObject *parent)
    : QObject(parent)
{
    instances().append(this);
}

SignalWatcherDouble::~SignalWatcherDouble()
{
    instances().removeAll(this);
}

QList<SignalWatcherDouble *> &SignalWatcherDouble::instances()
{
    static QList<SignalWatcherDouble *> list;
    return list;
}

void registerPlasmaDoubles()
{
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    const char *plasmoid = "org.kde.plasma.plasmoid";
    qmlRegisterType<PlasmoidItemDouble>(plasmoid, 2, 0, "PlasmoidItem");
    qmlRegisterUncreatableType<PlasmoidAttachedType>(plasmoid, 2, 0, "Plasmoid", QStringLiteral("attached only"));
    qmlProtectModule(plasmoid, 2);

    const char *support = "org.kde.plasma.plasma5support";
    qmlRegisterType<DataSourceDouble>(support, 2, 0, "DataSource");
    qmlProtectModule(support, 2);

    const char *taskmanager = "org.kde.taskmanager";
    qmlRegisterType<TasksModelDouble>(taskmanager, 0, 1, "TasksModel");
    qmlRegisterUncreatableType<AbstractTasksModelDouble>(taskmanager, 0, 1, "AbstractTasksModel", QStringLiteral("enums only"));
    qmlProtectModule(taskmanager, 0);

    const char *dbus = "org.kde.plasma.workspace.dbus";
    qmlRegisterSingletonType<SessionBusDouble>(dbus, 1, 0, "SessionBus", [](QQmlEngine *, QJSEngine *) { return new SessionBusDouble; });
    qmlRegisterType<SignalWatcherDouble>(dbus, 1, 0, "SignalWatcher");
    qmlRegisterUncreatableMetaObject(BusTypeDouble::staticMetaObject, dbus, 1, 0, "BusType", QStringLiteral("enums only"));
    qmlProtectModule(dbus, 1);
}
