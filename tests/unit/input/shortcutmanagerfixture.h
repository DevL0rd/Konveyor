#pragma once

#include "../dbus/fakekglobalaccel.h"
#include "config/loader.h"
#include "input/shortcutmanager.h"

#include <QTest>

#include <xkbcommon/xkbcommon.h>

#include <memory>

namespace Konveyor::Test
{

inline QList<Config::Bind> binds(const QString &body, const QString &input = QString())
{
    const auto loaded
        = Config::loadString(input + QStringLiteral("binds {\n") + body + QStringLiteral("\n}\n"), QStringLiteral("config.kdl"));
    if (!loaded) {
        qWarning() << loaded.error().toString();
        return {};
    }
    return loaded->config.binds;
}

class ShortcutManagerFixture : public QObject
{
protected:
    std::unique_ptr<PrivateSession> m_session;
    std::unique_ptr<FakeKGlobalAccel> m_fake;
    xkb_context *m_context = nullptr;
    QStringList m_fired;

    std::unique_ptr<ShortcutManager> manager()
    {
        m_fired.clear();
        return std::make_unique<ShortcutManager>(
            [this](const Config::Bind &bind) { m_fired.append(Config::bindKeyLabel(bind) + QLatin1Char(' ') + bind.action.name); });
    }

    xkb_keymap *keymap(const char *layout)
    {
        const xkb_rule_names names {"evdev", "pc105", layout, "", ""};
        return xkb_keymap_new_from_names(m_context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    }

    QList<QKeySequence> keys(const QString &action) const { return m_fake->keys(QStringLiteral("konveyor"), action); }

    bool startFake()
    {
        m_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        m_session = std::make_unique<PrivateSession>();
        m_fake = std::make_unique<FakeKGlobalAccel>();
        return m_context && m_session->start() && m_fake->start(m_session->address());
    }

    void stopFake()
    {
        xkb_context_unref(m_context);
        m_fake.reset();
        m_session.reset();
    }
};

}
