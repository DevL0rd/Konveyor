#pragma once

#include <KQuickConfigModule>

#include <QVariantList>

namespace Konveyor::Settings
{

class KonveyorKcm : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(QString initialPage READ initialPage CONSTANT)

public:
    KonveyorKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);

    QString initialPage() const;

    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void loadRequested();
    void saveRequested();
    void defaultsRequested();

private:
    QString m_initialPage;
};

}
