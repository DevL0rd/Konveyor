#include "decorations/accentcolor.h"

#include <KColorScheme>
#include <KSharedConfig>

namespace Konveyor
{

AccentColor::AccentColor(QObject *parent)
    : QObject(parent)
    , m_watcher(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kdeglobals"))))
{
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group) {
        if (group.name() == QLatin1String("General") || group.name().startsWith(QLatin1String("Colors:"))) {
            reload();
        }
    });
    reload();
}

QColor AccentColor::colorFor(Config::ColorSource source) const
{
    return m_colors.value(static_cast<int>(source), m_color);
}

void AccentColor::reload()
{
    const KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    config->reparseConfiguration();
    const KColorScheme selection(QPalette::Active, KColorScheme::Selection, config);
    const KColorScheme window(QPalette::Active, KColorScheme::Window, config);
    const KColorScheme decoration(QPalette::Active, KColorScheme::Window, config);
    QHash<int, QColor> colors;
    colors.insert(static_cast<int>(Config::ColorSource::SystemAccent), selection.background(KColorScheme::NormalBackground).color());
    colors.insert(static_cast<int>(Config::ColorSource::SystemFocus), decoration.decoration(KColorScheme::FocusColor).color());
    colors.insert(static_cast<int>(Config::ColorSource::SystemHover), decoration.decoration(KColorScheme::HoverColor).color());
    colors.insert(static_cast<int>(Config::ColorSource::SystemWindow), window.background(KColorScheme::NormalBackground).color());
    colors.insert(static_cast<int>(Config::ColorSource::SystemWindowText), window.foreground(KColorScheme::NormalText).color());
    colors.insert(static_cast<int>(Config::ColorSource::SystemInactiveText), window.foreground(KColorScheme::InactiveText).color());
    if (colors == m_colors) {
        return;
    }
    m_colors = colors;
    m_color = colors.value(static_cast<int>(Config::ColorSource::SystemAccent));
    Q_EMIT changed();
}

}
