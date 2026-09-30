#pragma once

#include "privatebus.h"

#include <KGlobalShortcutInfo>

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QKeySequence>
#include <QRegularExpression>

namespace Konveyor::Test
{

struct FakeShortcut
{
    QStringList actionId;
    QList<QKeySequence> keys;
};

inline QDBusArgument &operator<<(QDBusArgument &argument, const FakeShortcut &shortcut)
{
    const QStringList &id = shortcut.actionId;
    argument.beginStructure();
    argument << id.value(1) << id.value(3) << id.value(0) << id.value(2) << QStringLiteral("default") << QStringLiteral("Default Context");
    argument.beginArray(QMetaType::fromType<int>());
    for (const QKeySequence &key : shortcut.keys) {
        argument << key[0].toCombined();
    }
    argument.endArray();
    argument.beginArray(QMetaType::fromType<int>());
    argument.endArray();
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, FakeShortcut &shortcut)
{
    QString unique, friendly, component, componentFriendly, context, contextFriendly;
    QList<int> keys, defaults;
    argument.beginStructure();
    argument >> unique >> friendly >> component >> componentFriendly >> context >> contextFriendly >> keys >> defaults;
    argument.endStructure();
    shortcut.actionId = {component, unique, componentFriendly, friendly};
    shortcut.keys.clear();
    for (const int key : keys) {
        shortcut.keys.append(QKeySequence(key));
    }
    return argument;
}

class FakeKGlobalAccel
{
public:
    FakeKGlobalAccel()
        : m_service(
              QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/"), [this](const QDBusMessage &message) { return answer(message); })
    {
        qDBusRegisterMetaType<QKeySequence>();
        qDBusRegisterMetaType<QList<QKeySequence>>();
        qDBusRegisterMetaType<FakeShortcut>();
        qDBusRegisterMetaType<QList<FakeShortcut>>();
    }

    bool start(const QString &address) { return m_service.start(address); }

    void add(const QStringList &actionId, const QList<QKeySequence> &keys)
    {
        const QMutexLocker locker(&m_mutex);
        m_shortcuts.append({actionId, keys});
    }

    QList<QKeySequence> keys(const QString &component, const QString &action) const
    {
        const QMutexLocker locker(&m_mutex);
        const FakeShortcut *shortcut = find(component, action);
        return shortcut ? shortcut->keys : QList<QKeySequence> {};
    }

    QList<QDBusMessage> calls(const QString &member) const { return m_service.calls(member); }

    static QList<QKeySequence> keysOf(const QVariant &argument)
    {
        QList<QKeySequence> keys;
        argument.value<QDBusArgument>() >> keys;
        return keys;
    }

private:
    FakeShortcut *find(const QString &component, const QString &action)
    {
        for (FakeShortcut &shortcut : m_shortcuts) {
            if (shortcut.actionId.value(0) == component && shortcut.actionId.value(1) == action) {
                return &shortcut;
            }
        }
        return nullptr;
    }

    const FakeShortcut *find(const QString &component, const QString &action) const
    {
        return const_cast<FakeKGlobalAccel *>(this)->find(component, action);
    }

    QList<FakeShortcut> holders(const QKeySequence &key) const
    {
        QList<FakeShortcut> found;
        for (const FakeShortcut &shortcut : m_shortcuts) {
            if (shortcut.keys.contains(key)) {
                found.append(shortcut);
            }
        }
        return found;
    }

    void store(const QStringList &actionId, const QList<QKeySequence> &keys)
    {
        if (FakeShortcut *shortcut = find(actionId.value(0), actionId.value(1))) {
            shortcut->keys = keys;
        } else {
            m_shortcuts.append({actionId, keys});
        }
    }

    QDBusMessage answer(const QDBusMessage &message)
    {
        const QMutexLocker locker(&m_mutex);
        const QString member = message.member();
        const QVariantList arguments = message.arguments();
        const QStringList actionId = arguments.value(0).toStringList();
        if (member == QLatin1String("doRegister") || member == QLatin1String("setInactive") || member == QLatin1String("unRegister")
            || member == QLatin1String("activateGlobalShortcutContext")) {
            return message.createReply();
        }
        if (member == QLatin1String("getComponent")) {
            QString name = arguments.value(0).toString();
            name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]")), QStringLiteral("_"));
            return message.createReply(QVariant::fromValue(QDBusObjectPath(QStringLiteral("/component/") + name)));
        }
        if (member == QLatin1String("shortcutKeys") || member == QLatin1String("defaultShortcutKeys")) {
            const FakeShortcut *shortcut = find(actionId.value(0), actionId.value(1));
            return message.createReply(QVariant::fromValue(shortcut ? shortcut->keys : QList<QKeySequence> {}));
        }
        if (member == QLatin1String("setShortcutKeys")) {
            const QList<QKeySequence> keys = keysOf(arguments.value(1));
            store(actionId, keys);
            return message.createReply(QVariant::fromValue(keys));
        }
        if (member == QLatin1String("setForeignShortcutKeys")) {
            store(actionId, keysOf(arguments.value(1)));
            return message.createReply();
        }
        return answerByKey(message);
    }

    QDBusMessage answerByKey(const QDBusMessage &message) const
    {
        QKeySequence key;
        message.arguments().value(0).value<QDBusArgument>() >> key;
        const QList<FakeShortcut> found = holders(key);
        if (message.member() == QLatin1String("actionList")) {
            return message.createReply(found.isEmpty() ? QStringList {} : found.first().actionId);
        }
        if (message.member() == QLatin1String("globalShortcutsByKey")) {
            return message.createReply(QVariant::fromValue(found));
        }
        return message.createErrorReply(QDBusError::UnknownMethod, message.member());
    }

    mutable QMutex m_mutex;
    QList<FakeShortcut> m_shortcuts;
    FakeService m_service;
};

}
