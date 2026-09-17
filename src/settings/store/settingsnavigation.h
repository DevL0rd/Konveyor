#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace Konveyor::Settings
{

class SettingsNavigation : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY depthChanged)

public:
    using QObject::QObject;

    int depth() const;
    void setDepth(int depth);

    Q_INVOKABLE void push(const QString &page, const QVariantMap &properties = {});
    Q_INVOKABLE void pop();

Q_SIGNALS:
    void depthChanged();
    void pushRequested(const QString &page, const QVariantMap &properties);
    void popRequested();

private:
    int m_depth = 1;
};

}
