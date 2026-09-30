#include "plugin/notifications.h"

#include <KNotification>

namespace Konveyor
{

void sendNotification(const QString &title, const QString &text)
{
    auto *notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout);
    notification->setComponentName(QStringLiteral("plasma_workspace"));
    notification->setTitle(title);
    notification->setText(text);
    notification->setIconName(QStringLiteral("preferences-system-windows-effect"));
    notification->sendEvent();
}

}
