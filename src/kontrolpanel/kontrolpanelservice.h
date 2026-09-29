#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QRegion>
#include <QStringList>

#include <Plasma/Theme>

class QWindow;

namespace Konveyor
{

class KontrolPanelService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.devl0rd.KontrolPanel")
    Q_PROPERTY(bool open READ isOpen NOTIFY openChanged)

public:
    static constexpr auto serviceName = "org.devl0rd.KontrolPanel";
    static constexpr auto objectPath = "/KontrolPanel";

    explicit KontrolPanelService(QObject *parent = nullptr);

    bool registerOn(QDBusConnection bus);
    bool isOpen() const { return m_open; }

    Q_INVOKABLE void setOpen(bool open);
    Q_INVOKABLE void applyBackgroundEffects(QWindow *window, const QRegion &region);
    Q_INVOKABLE bool konveyorRunning() const;

public Q_SLOTS:
    Q_SCRIPTABLE void Toggle();
    Q_SCRIPTABLE void Open(const QString &page);
    Q_SCRIPTABLE void Hide();
    Q_SCRIPTABLE void Pin(const QStringList &files);
    Q_SCRIPTABLE void Configure();
    Q_SCRIPTABLE bool IsOpen() const;

Q_SIGNALS:
    void toggleRequested();
    void openRequested(const QString &page);
    void hideRequested();
    void pinRequested(const QStringList &files);
    void configureRequested();
    void openChanged();

private:
    Plasma::Theme m_theme;
    bool m_open = false;
};

}
