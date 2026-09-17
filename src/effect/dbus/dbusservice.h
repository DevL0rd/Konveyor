#pragma once

#include <QJsonDocument>
#include <QObject>

#include <functional>

namespace Konveyor
{

struct DBusHandlers
{
    std::function<QJsonDocument()> windows;
    std::function<QJsonDocument()> workspaces;
    std::function<QJsonDocument()> outputs;
    std::function<QJsonDocument()> focusedWindow;
    std::function<QJsonDocument()> focusedOutput;
    std::function<QJsonDocument()> binds;
    std::function<QString(const QString &)> action;
    std::function<QString(const QString &)> loadConfigFile;
    std::function<bool()> overviewOpen;
    std::function<QJsonDocument()> lastBind;
    std::function<QJsonDocument()> gestures;
};

class DBusService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konveyor")

public:
    explicit DBusService(DBusHandlers handlers, QObject *parent = nullptr);
    ~DBusService() override;

    bool registerService();

public Q_SLOTS:
    Q_SCRIPTABLE QString Version() const;
    Q_SCRIPTABLE QString Windows() const;
    Q_SCRIPTABLE QString Workspaces() const;
    Q_SCRIPTABLE QString Outputs() const;
    Q_SCRIPTABLE QString FocusedWindow() const;
    Q_SCRIPTABLE QString FocusedOutput() const;
    Q_SCRIPTABLE QString Binds() const;
    Q_SCRIPTABLE QString OverviewState() const;
    Q_SCRIPTABLE QString Action(const QString &json);
    Q_SCRIPTABLE QString LoadConfigFile(const QString &path);
    Q_SCRIPTABLE QString LastBind() const;
    Q_SCRIPTABLE QString Gestures() const;
    Q_SCRIPTABLE bool MultiTouchActive() const;

    void setMultiTouchActive(bool active);

Q_SIGNALS:
    Q_SCRIPTABLE void MultiTouchChanged(bool active);

private:
    static QString compact(const QJsonDocument &document);

    bool m_multiTouchActive = false;

    DBusHandlers m_handlers;
    bool m_registered = false;
};

}
