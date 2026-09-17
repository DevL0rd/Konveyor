#include "module/konveyorkcm.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(Konveyor::Settings::KonveyorKcm, "kcm_konveyor.json")

namespace Konveyor::Settings
{

KonveyorKcm::KonveyorKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : KQuickConfigModule(parent, metaData)
    , m_initialPage(args.value(0).toString())
{
    setButtons(Apply | Default);
}

QString KonveyorKcm::initialPage() const
{
    return m_initialPage;
}

void KonveyorKcm::load()
{
    Q_EMIT loadRequested();
}

void KonveyorKcm::save()
{
    Q_EMIT saveRequested();
}

void KonveyorKcm::defaults()
{
    Q_EMIT defaultsRequested();
}

}

#include "konveyorkcm.moc"
