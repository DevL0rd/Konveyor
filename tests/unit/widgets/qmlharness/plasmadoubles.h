#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QVariantMap>

class ActionDouble : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    Q_INVOKABLE void trigger() { ++triggered; }
    int triggered = 0;
};

class ConfigDouble : public QQmlPropertyMap
{
    Q_OBJECT

public:
    explicit ConfigDouble(QObject *parent)
        : QQmlPropertyMap(this, parent)
    { }
    Q_INVOKABLE void writeConfig() { ++writes; }
    int writes = 0;
};

class PlasmoidDouble : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap *configuration READ configuration CONSTANT)
    Q_PROPERTY(int formFactor MEMBER formFactor NOTIFY formFactorChanged)
    Q_PROPERTY(int location MEMBER location NOTIFY locationChanged)
    Q_PROPERTY(QString pluginName MEMBER pluginName CONSTANT)
    Q_PROPERTY(int id MEMBER id CONSTANT)
    Q_PROPERTY(QVariantMap metaData MEMBER metaData CONSTANT)
    Q_PROPERTY(QString icon MEMBER icon NOTIFY iconChanged)
    Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
    Q_PROPERTY(int status MEMBER status NOTIFY statusChanged)
    Q_PROPERTY(bool busy MEMBER busy NOTIFY busyChanged)

public:
    explicit PlasmoidDouble(const QString &pluginId, const QString &iconName, QObject *parent = nullptr);
    ConfigDouble *configuration() const { return m_configuration; }
    Q_INVOKABLE QObject *internalAction(const QString &name);
    ActionDouble *action(const QString &name) const { return m_actions.value(name); }

    int formFactor = 0;
    int location = 0;
    QString pluginName;
    int id = 7;
    QVariantMap metaData;
    QString icon;
    QString title;
    int status = 0;
    bool busy = false;

Q_SIGNALS:
    void formFactorChanged();
    void locationChanged();
    void iconChanged();
    void titleChanged();
    void statusChanged();
    void busyChanged();

private:
    ConfigDouble *m_configuration;
    QHash<QString, ActionDouble *> m_actions;
};

class PlasmoidAttachedType : public QObject
{
    Q_OBJECT
    QML_ATTACHED(PlasmoidDouble)

public:
    static PlasmoidDouble *qmlAttachedProperties(QObject *object);
    static QPointer<PlasmoidDouble> current;
};

class PlasmoidItemDouble : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *compactRepresentation MEMBER compactRepresentation NOTIFY compactRepresentationChanged)
    Q_PROPERTY(QQmlComponent *fullRepresentation MEMBER fullRepresentation NOTIFY fullRepresentationChanged)
    Q_PROPERTY(QQmlComponent *preferredRepresentation MEMBER preferredRepresentation NOTIFY preferredRepresentationChanged)
    Q_PROPERTY(bool expanded MEMBER expanded NOTIFY expandedChanged)
    Q_PROPERTY(QString toolTipMainText MEMBER toolTipMainText NOTIFY toolTipChanged)
    Q_PROPERTY(QString toolTipSubText MEMBER toolTipSubText NOTIFY toolTipChanged)
    Q_PROPERTY(int toolTipTextFormat MEMBER toolTipTextFormat NOTIFY toolTipChanged)
    Q_PROPERTY(QQuickItem *toolTipItem MEMBER toolTipItem NOTIFY toolTipChanged)
    Q_PROPERTY(bool hideOnWindowDeactivate MEMBER hideOnWindowDeactivate NOTIFY representationsChanged)
    Q_PROPERTY(bool activationTogglesExpanded MEMBER activationTogglesExpanded NOTIFY representationsChanged)
    Q_PROPERTY(int switchWidth MEMBER switchWidth NOTIFY representationsChanged)
    Q_PROPERTY(int switchHeight MEMBER switchHeight NOTIFY representationsChanged)

public:
    using QQuickItem::QQuickItem;

    QQmlComponent *compactRepresentation = nullptr;
    QQmlComponent *fullRepresentation = nullptr;
    QQmlComponent *preferredRepresentation = nullptr;
    bool expanded = false;
    QString toolTipMainText;
    QString toolTipSubText;
    int toolTipTextFormat = 0;
    QQuickItem *toolTipItem = nullptr;
    bool hideOnWindowDeactivate = true;
    bool activationTogglesExpanded = true;
    int switchWidth = 0;
    int switchHeight = 0;

Q_SIGNALS:
    void compactRepresentationChanged();
    void fullRepresentationChanged();
    void preferredRepresentationChanged();
    void representationsChanged();
    void expandedChanged();
    void toolTipChanged();
};

class DataSourceDouble : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString engine MEMBER engine NOTIFY engineChanged)
    Q_PROPERTY(QStringList connectedSources READ connectedSources WRITE setConnectedSources NOTIFY connectedSourcesChanged)
    Q_PROPERTY(int interval MEMBER interval NOTIFY intervalChanged)
    Q_PROPERTY(QVariantMap data MEMBER data NOTIFY dataChanged)

public:
    explicit DataSourceDouble(QObject *parent = nullptr);
    ~DataSourceDouble() override;
    QStringList connectedSources() const { return m_sources; }
    void setConnectedSources(const QStringList &sources);
    Q_INVOKABLE void connectSource(const QString &source);
    Q_INVOKABLE void disconnectSource(const QString &source);
    void reply(const QString &source, const QString &out, int exitCode = 0, const QString &err = QString());

    static QList<DataSourceDouble *> &instances();
    static QStringList &commands();

    QString engine;
    int interval = 0;
    QVariantMap data;

Q_SIGNALS:
    void newData(const QString &sourceName, const QVariantMap &data);
    void engineChanged();
    void connectedSourcesChanged();
    void intervalChanged();
    void dataChanged();

private:
    QStringList m_sources;
};

class TasksModelDouble : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QModelIndex activeTask READ activeTask NOTIFY activeTaskChanged)
    Q_PROPERTY(int groupMode MEMBER groupMode)
    Q_PROPERTY(bool filterByVirtualDesktop MEMBER filterByVirtualDesktop)
    Q_PROPERTY(bool filterByActivity MEMBER filterByActivity)
    Q_PROPERTY(bool filterByScreen MEMBER filterByScreen)
    Q_PROPERTY(bool filterNotMinimized MEMBER filterNotMinimized)

public:
    enum GroupMode
    {
        GroupDisabled = 0,
        GroupApplications
    };
    Q_ENUM(GroupMode)
    explicit TasksModelDouble(QObject *parent = nullptr);
    ~TasksModelDouble() override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int count() const { return rowCount(); }
    QModelIndex activeTask() const;
    void activate(int pid, const QString &name);
    void clearActive();

    static QList<TasksModelDouble *> &instances();

    int groupMode = 0;
    bool filterByVirtualDesktop = true;
    bool filterByActivity = true;
    bool filterByScreen = true;
    bool filterNotMinimized = false;

Q_SIGNALS:
    void countChanged();
    void activeTaskChanged();

private:
    QList<QPair<int, QString>> m_tasks;
    int m_active = -1;
};

class AbstractTasksModelDouble : public QObject
{
    Q_OBJECT

public:
    enum AdditionalRoles
    {
        AppId = Qt::UserRole + 1,
        AppName,
        GenericName,
        AppPid
    };
    Q_ENUM(AdditionalRoles)
};

class SessionBusDouble : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    Q_INVOKABLE void asyncCall(const QVariantMap &message);
    static QList<QVariantMap> &calls();
};

class SignalWatcherDouble : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY enabledChanged)
    Q_PROPERTY(int busType MEMBER busType NOTIFY changed)
    Q_PROPERTY(QString service MEMBER service NOTIFY changed)
    Q_PROPERTY(QString path MEMBER path NOTIFY changed)
    Q_PROPERTY(QString iface MEMBER iface NOTIFY changed)

public:
    explicit SignalWatcherDouble(QObject *parent = nullptr);
    ~SignalWatcherDouble() override;
    static QList<SignalWatcherDouble *> &instances();

    bool enabled = true;
    int busType = 0;
    QString service;
    QString path;
    QString iface;

Q_SIGNALS:
    void enabledChanged();
    void changed();
};

namespace BusTypeDouble
{
Q_NAMESPACE
enum Type
{
    Session,
    System
};
Q_ENUM_NS(Type)
}

void registerPlasmaDoubles();
