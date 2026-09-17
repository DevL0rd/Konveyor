#include "store/settingsnavigation.h"

namespace Konveyor::Settings
{

int SettingsNavigation::depth() const
{
    return m_depth;
}

void SettingsNavigation::setDepth(int depth)
{
    if (m_depth == depth) {
        return;
    }
    m_depth = depth;
    Q_EMIT depthChanged();
}

void SettingsNavigation::push(const QString &page, const QVariantMap &properties)
{
    Q_EMIT pushRequested(page, properties);
}

void SettingsNavigation::pop()
{
    Q_EMIT popRequested();
}

}
