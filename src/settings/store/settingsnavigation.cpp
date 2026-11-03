#include "store/settingsnavigation.h"

#include <QQmlEngine>

namespace Konveyor::Settings
{

namespace
{
constexpr int IncubationSliceMs = 5;
}

SettingsNavigation::SectionIncubator::SectionIncubator()
{
    QObject::connect(&m_slice, &QTimer::timeout, [this] { incubateFor(IncubationSliceMs); });
}

void SettingsNavigation::SectionIncubator::incubatingObjectCountChanged(int count)
{
    if (count > 0) {
        m_slice.start();
    } else {
        m_slice.stop();
    }
}

void SettingsNavigation::loadSectionsInBackground()
{
    QQmlEngine *engine = qmlEngine(this);
    if (engine && !engine->incubationController()) {
        engine->setIncubationController(&m_incubator);
    }
}

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
