#pragma once

#include <KQuickConfigModule>

#include <QVariantList>

namespace Konveyor::Settings
{

class KonveyorKcm : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(QString initialPage READ initialPage NOTIFY initialPageChanged)
    Q_PROPERTY(QString initialApp READ initialApp NOTIFY initialPageChanged)

public:
    KonveyorKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);

    QString initialPage() const;
    QString initialApp() const;

Q_SIGNALS:
    void initialPageChanged();

private:
    QString m_initialPage;
    QString m_initialApp;
};

}
