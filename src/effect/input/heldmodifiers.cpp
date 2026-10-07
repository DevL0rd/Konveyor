#include "input/heldmodifiers.h"

#include <utility>

namespace Konveyor
{

HeldModifiers::HeldModifiers(std::function<void(bool, bool)> changed)
    : m_changed(std::move(changed))
{ }

void HeldModifiers::key(int key, bool pressed, Qt::KeyboardModifiers modifiers)
{
    bool super = modifiers.testFlag(Qt::MetaModifier);
    bool alt = modifiers.testFlag(Qt::AltModifier);
    if (key == Qt::Key_Meta || key == Qt::Key_Super_L || key == Qt::Key_Super_R) {
        super = pressed;
    } else if (key == Qt::Key_Alt) {
        alt = pressed;
    }
    set(super, alt);
}

void HeldModifiers::sync(Qt::KeyboardModifiers modifiers)
{
    set(modifiers.testFlag(Qt::MetaModifier), modifiers.testFlag(Qt::AltModifier));
}

void HeldModifiers::clear()
{
    set(false, false);
}

void HeldModifiers::set(bool super, bool alt)
{
    if (super == m_super && alt == m_alt) {
        return;
    }
    m_super = super;
    m_alt = alt;
    m_changed(super, alt);
}

}
