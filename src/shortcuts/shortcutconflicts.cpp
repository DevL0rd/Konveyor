#include "shortcutconflicts.h"

#include "kglobalaccelinterface.h"

#include <KConfigGroup>
#include <KGlobalAccel>
#include <KGlobalShortcutInfo>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QDBusMetaType>

#include <algorithm>

namespace Konveyor
{

namespace
{

constexpr QLatin1StringView stateFile("konveyorstaterc");
constexpr QLatin1StringView releasedGroup("ReleasedShortcuts");

QStringList keysToStrings(const QList<QKeySequence> &keys)
{
    QStringList strings;
    for (const QKeySequence &key : keys) {
        strings.append(key.toString(QKeySequence::PortableText));
    }
    return strings;
}

QList<QKeySequence> keysFromStrings(const QStringList &strings)
{
    QList<QKeySequence> keys;
    for (const QString &string : strings) {
        keys.append(QKeySequence(string, QKeySequence::PortableText));
    }
    return keys;
}

const QList<std::pair<QString, QString>> &superseded()
{
    static const QList<std::pair<QString, QString>> actions {
        {QStringLiteral("kwin"), QStringLiteral("Edit Tiles")},
    };
    return actions;
}

bool isWanted(const ReleasedShortcut &shortcut, const QList<QKeySequence> &wanted)
{
    return superseded().contains({shortcut.component, shortcut.action})
        || std::ranges::any_of(shortcut.keys, [&wanted](const QKeySequence &key) { return wanted.contains(key); });
}

bool giveBack(const ReleasedShortcut &shortcut)
{
    qDBusRegisterMetaType<QKeySequence>();
    qDBusRegisterMetaType<QList<QKeySequence>>();
    const QList<QKeySequence> current = KGlobalAccel::self()->globalShortcut(shortcut.component, shortcut.action);
    if (!std::ranges::all_of(current, [&shortcut](const QKeySequence &key) { return shortcut.keys.contains(key); })) {
        qInfo() << "konveyor: leaving" << shortcut.component << shortcut.action << "on the keys it was given since";
        return true;
    }
    OrgKdeKGlobalAccelInterface kglobalaccel(
        QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/kglobalaccel"), QDBusConnection::sessionBus());
    const QStringList actionId {shortcut.component, shortcut.action, shortcut.componentFriendlyName, shortcut.actionFriendlyName};
    QDBusPendingReply<> reply = kglobalaccel.setForeignShortcutKeys(actionId, shortcut.keys);
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "konveyor: could not restore" << shortcut.component << shortcut.action << reply.error().message();
        return false;
    }
    return true;
}

}

QList<ReleasedShortcut> ShortcutConflicts::takeOver(const QList<QKeySequence> &wanted, const QString &ownComponent)
{
    QList<ReleasedShortcut> released;
    for (const QKeySequence &key : wanted) {
        const QList<KGlobalShortcutInfo> holders = KGlobalAccel::globalShortcutsByKey(key, KGlobalAccel::MatchType::Equal);
        bool foreignHolder = false;
        for (const KGlobalShortcutInfo &holder : holders) {
            if (holder.componentUniqueName() == ownComponent) {
                continue;
            }
            foreignHolder = true;
            merge(released,
                {{holder.componentUniqueName(), holder.uniqueName(), holder.componentFriendlyName(), holder.friendlyName(),
                    holder.keys()}});
        }
        if (foreignHolder) {
            KGlobalAccel::stealShortcutSystemwide(key);
        }
    }
    return released;
}

QList<ReleasedShortcut> ShortcutConflicts::releaseSuperseded()
{
    QList<ReleasedShortcut> released;
    for (const auto &[component, action] : superseded()) {
        const QList<QKeySequence> keys = KGlobalAccel::self()->globalShortcut(component, action);
        if (keys.isEmpty()) {
            continue;
        }
        released.append({component, action, component, action, keys});
        for (const QKeySequence &key : keys) {
            KGlobalAccel::stealShortcutSystemwide(key);
        }
    }
    return released;
}

QList<ReleasedShortcut> ShortcutConflicts::restore(const QList<ReleasedShortcut> &released)
{
    QList<ReleasedShortcut> failed;
    for (const ReleasedShortcut &entry : released) {
        if (!giveBack(entry)) {
            failed.append(entry);
        }
    }
    return failed;
}

QList<ReleasedShortcut> ShortcutConflicts::restoreUnwanted(const QList<ReleasedShortcut> &released, const QList<QKeySequence> &wanted)
{
    QList<ReleasedShortcut> kept;
    for (const ReleasedShortcut &entry : released) {
        if (isWanted(entry, wanted) || !giveBack(entry)) {
            kept.append(entry);
        }
    }
    return kept;
}

QList<ReleasedShortcut> ShortcutConflicts::load()
{
    const KConfigGroup group = KSharedConfig::openStateConfig(stateFile)->group(releasedGroup);
    QList<ReleasedShortcut> released;
    const QStringList names = group.groupList();
    for (const QString &name : names) {
        const KConfigGroup entry = group.group(name);
        released.append({entry.readEntry("Component"), entry.readEntry("Action"), entry.readEntry("ComponentFriendlyName"),
            entry.readEntry("ActionFriendlyName"), keysFromStrings(entry.readEntry("Keys", QStringList()))});
    }
    return released;
}

void ShortcutConflicts::save(const QList<ReleasedShortcut> &released)
{
    const KSharedConfig::Ptr config = KSharedConfig::openStateConfig(stateFile);
    KConfigGroup group = config->group(releasedGroup);
    group.deleteGroup();
    for (const ReleasedShortcut &shortcut : released) {
        KConfigGroup entry = group.group(shortcut.component + QLatin1Char('/') + shortcut.action);
        entry.writeEntry("Component", shortcut.component);
        entry.writeEntry("Action", shortcut.action);
        entry.writeEntry("ComponentFriendlyName", shortcut.componentFriendlyName);
        entry.writeEntry("ActionFriendlyName", shortcut.actionFriendlyName);
        entry.writeEntry("Keys", keysToStrings(shortcut.keys));
    }
    config->sync();
}

void ShortcutConflicts::merge(QList<ReleasedShortcut> &into, const QList<ReleasedShortcut> &extra)
{
    for (const ReleasedShortcut &entry : extra) {
        const bool known = std::ranges::any_of(into, [&entry](const ReleasedShortcut &existing) {
            return existing.component == entry.component && existing.action == entry.action;
        });
        if (!known) {
            into.append(entry);
        }
    }
}

}
