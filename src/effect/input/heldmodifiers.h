#pragma once

#include <Qt>

#include <functional>

namespace Konveyor
{

class HeldModifiers
{
public:
    explicit HeldModifiers(std::function<void(bool, bool)> changed);

    void key(int key, bool pressed, Qt::KeyboardModifiers modifiers);
    void sync(Qt::KeyboardModifiers modifiers);
    void clear();

private:
    void set(bool super, bool alt);

    std::function<void(bool, bool)> m_changed;
    bool m_super = false;
    bool m_alt = false;
};

}
