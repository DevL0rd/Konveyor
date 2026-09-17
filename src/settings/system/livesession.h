#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace Konveyor::Settings
{

class LiveSession : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QVariantList windows READ windows NOTIFY changed)
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY changed)
    Q_PROPERTY(QVariantList workspaces READ workspaces NOTIFY changed)
    Q_PROPERTY(QVariantList applications READ applications CONSTANT)

public:
    explicit LiveSession(QObject *parent = nullptr);

    bool running() const;
    QVariantList windows() const;
    QVariantList outputs() const;
    QVariantList workspaces() const;
    QVariantList applications() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString iconFor(const QString &appId) const;
    Q_INVOKABLE QString nameFor(const QString &appId) const;

Q_SIGNALS:
    void changed();

private:
    QVariantList m_windows;
    QVariantList m_outputs;
    QVariantList m_workspaces;
    mutable QVariantList m_applications;
    bool m_running = false;
};

}
