#pragma once

#include "config/types.h"
#include "shortcutconflicts.h"

#include <QHash>
#include <QObject>

#include <xkbcommon/xkbcommon.h>

#include <functional>

class QAction;

namespace Konveyor
{

class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<void(const Config::Bind &)>;

    ShortcutManager(Handler handler, QObject *parent = nullptr);
    ~ShortcutManager() override;

    void setBinds(const QList<Config::Bind> &binds);
    const QList<Config::Bind> &binds() const;
    bool triggerPointerBind(
        Config::BindTrigger trigger, Config::BindModifiers modifiers, Config::MouseButton button, Config::ScrollDirection direction);
    bool triggerKeyPosition(quint32 keycode, Config::BindModifiers modifiers, bool repeat, xkb_keymap *keymap, xkb_layout_index_t layout);

private:
    static QString actionName(const Config::Bind &bind);
    static bool registersWithKde(const Config::Bind &bind);
    static bool keyMatches(const Config::Bind &bind, quint32 keycode, xkb_keymap *keymap, xkb_layout_index_t layout);
    static QList<QKeySequence> keySequences(const Config::Bind &bind);
    void registerKeyBind(const Config::Bind &bind);
    void unregisterAll();
    bool isOnCooldown(const QString &name, int cooldownMs);
    void invoke(const Config::Bind &bind);

    Handler m_handler;
    QList<Config::Bind> m_binds;
    QHash<QString, QAction *> m_actions;
    QHash<QString, qint64> m_lastTriggered;
    QList<ReleasedShortcut> m_released;
};

}
