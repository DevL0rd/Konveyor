#include "input/shortcutmanager.h"

#include "config/loader.h"

#include <KGlobalAccel>

#include <QAction>
#include <QDateTime>
#include <QHash>
#include <QKeySequence>

namespace Konveyor
{

namespace
{

Qt::KeyboardModifiers toQtModifiers(Config::BindModifiers modifiers)
{
    Qt::KeyboardModifiers result;
    const QList<std::pair<Config::BindModifier, Qt::KeyboardModifier>> mapping {
        {Config::BindModifier::Ctrl, Qt::ControlModifier},
        {Config::BindModifier::Shift, Qt::ShiftModifier},
        {Config::BindModifier::Alt, Qt::AltModifier},
        {Config::BindModifier::Super, Qt::MetaModifier},
    };
    for (const auto &[bindModifier, qtModifier] : mapping) {
        if (modifiers.testFlag(bindModifier)) {
            result |= qtModifier;
        }
    }
    return result;
}

int shiftedKey(const Config::Bind &bind)
{
    static const QHash<int, int> pairs {{Qt::Key_1, Qt::Key_Exclam}, {Qt::Key_2, Qt::Key_At}, {Qt::Key_3, Qt::Key_NumberSign},
        {Qt::Key_4, Qt::Key_Dollar}, {Qt::Key_5, Qt::Key_Percent}, {Qt::Key_6, Qt::Key_AsciiCircum}, {Qt::Key_7, Qt::Key_Ampersand},
        {Qt::Key_8, Qt::Key_Asterisk}, {Qt::Key_9, Qt::Key_ParenLeft}, {Qt::Key_0, Qt::Key_ParenRight}, {Qt::Key_Minus, Qt::Key_Underscore},
        {Qt::Key_Equal, Qt::Key_Plus}, {Qt::Key_BracketLeft, Qt::Key_BraceLeft}, {Qt::Key_BracketRight, Qt::Key_BraceRight},
        {Qt::Key_Backslash, Qt::Key_Bar}, {Qt::Key_Semicolon, Qt::Key_Colon}, {Qt::Key_Apostrophe, Qt::Key_QuoteDbl},
        {Qt::Key_Comma, Qt::Key_Less}, {Qt::Key_Period, Qt::Key_Greater}, {Qt::Key_Slash, Qt::Key_Question},
        {Qt::Key_QuoteLeft, Qt::Key_AsciiTilde}};
    if (!bind.resolvedModifiers.testFlag(Config::BindModifier::Shift)) {
        return 0;
    }
    return pairs.value(bind.key, 0);
}

QString describe(const Config::Bind &bind)
{
    if (bind.hotkeyOverlayTitle) {
        return *bind.hotkeyOverlayTitle;
    }
    QStringList words {bind.action.name};
    words.append(bind.action.arguments);
    return QStringLiteral("Konveyor: %1").arg(words.join(QLatin1Char(' ')));
}

}

ShortcutManager::ShortcutManager(Handler handler, QObject *parent)
    : QObject(parent)
    , m_handler(std::move(handler))
{ }

ShortcutManager::~ShortcutManager()
{
    unregisterAll();
    ShortcutConflicts::restore(m_released);
}

void ShortcutManager::setBinds(const QList<Config::Bind> &binds)
{
    unregisterAll();
    m_binds = binds;
    QList<QKeySequence> wanted;
    for (const Config::Bind &bind : m_binds) {
        if (bind.trigger == Config::BindTrigger::Key && bind.key != 0) {
            wanted.append(keySequences(bind));
        }
    }
    ShortcutConflicts::merge(m_released, ShortcutConflicts::load());
    ShortcutConflicts::merge(m_released, ShortcutConflicts::releaseSuperseded());
    ShortcutConflicts::merge(m_released, ShortcutConflicts::takeOver(wanted, QStringLiteral("konveyor")));
    ShortcutConflicts::save(m_released);
    for (const Config::Bind &bind : m_binds) {
        if (bind.trigger == Config::BindTrigger::Key) {
            registerKeyBind(bind);
        }
    }
}

const QList<Config::Bind> &ShortcutManager::binds() const
{
    return m_binds;
}

bool ShortcutManager::triggerPointerBind(
    Config::BindTrigger trigger, Qt::KeyboardModifiers modifiers, Config::MouseButton button, Config::ScrollDirection direction)
{
    for (const Config::Bind &bind : m_binds) {
        const bool matchesTrigger = bind.trigger == trigger && toQtModifiers(bind.resolvedModifiers) == modifiers;
        const bool matchesButton = trigger != Config::BindTrigger::MouseButton || bind.mouseButton == button;
        const bool matchesDirection = trigger == Config::BindTrigger::MouseButton || bind.scrollDirection == direction;
        if (matchesTrigger && matchesButton && matchesDirection) {
            invoke(bind);
            return true;
        }
    }
    return false;
}

QString ShortcutManager::actionName(const Config::Bind &bind)
{
    return QStringLiteral("konveyor-%1").arg(Config::bindKeyLabel(bind));
}

QList<QKeySequence> ShortcutManager::keySequences(const Config::Bind &bind)
{
    const Qt::KeyboardModifiers modifiers = toQtModifiers(bind.resolvedModifiers);
    QList<QKeySequence> sequences {QKeySequence(static_cast<int>(modifiers.toInt()) | bind.key)};
    const int shifted = shiftedKey(bind);
    if (shifted != 0) {
        const Qt::KeyboardModifiers withoutShift = modifiers & ~Qt::ShiftModifier;
        sequences.append(QKeySequence(static_cast<int>(withoutShift.toInt()) | shifted));
    }
    return sequences;
}

void ShortcutManager::registerKeyBind(const Config::Bind &bind)
{
    if (bind.key == 0) {
        return;
    }
    const QString name = actionName(bind);
    if (m_actions.contains(name)) {
        return;
    }
    auto *action = new QAction(this);
    action->setObjectName(name);
    action->setText(describe(bind));
    action->setProperty("componentName", QStringLiteral("konveyor"));
    action->setProperty("componentDisplayName", QStringLiteral("Konveyor"));
    action->setAutoRepeat(bind.repeat);
    connect(action, &QAction::triggered, this, [this, bind]() { invoke(bind); });
    KGlobalAccel::self()->setShortcut(action, keySequences(bind), KGlobalAccel::NoAutoloading);
    m_actions.insert(name, action);
}

void ShortcutManager::unregisterAll()
{
    for (QAction *action : std::as_const(m_actions)) {
        KGlobalAccel::self()->removeAllShortcuts(action);
        delete action;
    }
    m_actions.clear();
    m_lastTriggered.clear();
}

bool ShortcutManager::isOnCooldown(const QString &name, int cooldownMs)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 last = m_lastTriggered.value(name, 0);
    if (last != 0 && now - last < cooldownMs) {
        return true;
    }
    m_lastTriggered.insert(name, now);
    return false;
}

void ShortcutManager::invoke(const Config::Bind &bind)
{
    if (bind.cooldownMs && isOnCooldown(actionName(bind), *bind.cooldownMs)) {
        return;
    }
    m_handler(bind);
}

}
