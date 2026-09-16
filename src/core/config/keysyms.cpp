#include "config/loader.h"
#include "config/sections.h"

#include <QtGui/private/qxkbcommon_p.h>

#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

namespace Konveyor::Config
{

namespace
{

struct ModifierMapping
{
    BindModifier flag;
    Qt::KeyboardModifier qt;
};

constexpr ModifierMapping kModifierMappings[] = {
    {BindModifier::Ctrl, Qt::ControlModifier},
    {BindModifier::Shift, Qt::ShiftModifier},
    {BindModifier::Alt, Qt::AltModifier},
    {BindModifier::Super, Qt::MetaModifier},
    {BindModifier::IsoLevel3Shift, Qt::GroupSwitchModifier},
};

}

quint32 keysymFromName(const QString &name)
{
    const QByteArray raw = name.toUtf8();
    xkb_keysym_t keysym = xkb_keysym_from_name(raw.constData(), XKB_KEYSYM_CASE_INSENSITIVE);
    if (keysym == XKB_KEY_XF86Screensaver) {
        keysym = xkb_keysym_from_name(raw.constData(), XKB_KEYSYM_NO_FLAGS);
        if (keysym == XKB_KEY_NoSymbol) {
            keysym = XKB_KEY_XF86ScreenSaver;
        }
    }
    return keysym;
}

int qtKeyFromKeysym(quint32 keysym)
{
    return QXkbCommon::keysymToQtKey(keysym, Qt::NoModifier);
}

Qt::KeyboardModifiers qtModifiers(BindModifiers modifiers)
{
    Qt::KeyboardModifiers result = Qt::NoModifier;
    for (const ModifierMapping &mapping : kModifierMappings) {
        if (modifiers.testFlag(mapping.flag)) {
            result |= mapping.qt;
        }
    }
    return result;
}

QString bindKeyLabel(const Bind &bind)
{
    static const QList<std::pair<BindModifier, QString>> names {
        {BindModifier::Super, QStringLiteral("Super")},
        {BindModifier::Ctrl, QStringLiteral("Ctrl")},
        {BindModifier::Alt, QStringLiteral("Alt")},
        {BindModifier::Shift, QStringLiteral("Shift")},
    };
    QStringList parts;
    for (const auto &[modifier, name] : names) {
        if (bind.resolvedModifiers.testFlag(modifier)) {
            parts.append(name);
        }
    }
    parts.append(bind.keyText);
    return parts.join(QLatin1Char('+'));
}

}
