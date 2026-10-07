#include "module/konveyorkcm.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(Konveyor::Settings::KonveyorKcm, "kcm_konveyor.json")

namespace Konveyor::Settings
{

KonveyorKcm::KonveyorKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : KQuickConfigModule(parent, metaData)
    , m_initialPage(args.value(0).toString())
    , m_initialApp(args.value(1).toString())
{
    setButtons(NoAdditionalButton);
    connect(this, &KAbstractConfigModule::activationRequested, this, [this](const QVariantList &arguments) {
        m_initialPage = arguments.value(0).toString();
        m_initialApp = arguments.value(1).toString();
        Q_EMIT initialPageChanged();
    });
}

QString KonveyorKcm::initialPage() const
{
    return m_initialPage;
}

QString KonveyorKcm::initialApp() const
{
    return m_initialApp;
}

}

#include "konveyorkcm.moc"
