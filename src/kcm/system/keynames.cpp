#include "system/keynames.h"

#include <QKeyEvent>
#include <QStringList>
#include <QtGui/private/qxkbcommon_p.h>

namespace Konveyor::Settings
{

QString bindKeyName(const QKeySequence &sequence, const QString &modKey)
{
    if (sequence.isEmpty()) {
        return QString();
    }
    const QKeyCombination combination = sequence[0];
    QKeyEvent event(QEvent::KeyPress, combination.key(), combination.keyboardModifiers());
    const QList<xkb_keysym_t> keysyms = QXkbCommon::toKeysym(&event);
    if (keysyms.isEmpty()) {
        return QString();
    }
    char name[64] = {};
    if (xkb_keysym_get_name(keysyms.first(), name, sizeof(name)) <= 0) {
        return QString();
    }
    const QList<std::pair<Qt::KeyboardModifier, QString>> modifiers {
        {Qt::MetaModifier, QStringLiteral("Super")},
        {Qt::ControlModifier, QStringLiteral("Ctrl")},
        {Qt::AltModifier, QStringLiteral("Alt")},
        {Qt::ShiftModifier, QStringLiteral("Shift")},
    };
    QStringList parts;
    for (const auto &[flag, label] : modifiers) {
        if (!combination.keyboardModifiers().testFlag(flag)) {
            continue;
        }
        if (label.compare(modKey, Qt::CaseInsensitive) == 0) {
            parts.prepend(QStringLiteral("Mod"));
        } else {
            parts.append(label);
        }
    }
    parts.append(QString::fromLatin1(name));
    return parts.join(QLatin1Char('+'));
}

}
