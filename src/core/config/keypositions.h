#pragma once

#include <QtGlobal>

#include <xkbcommon/xkbcommon.h>

#include <optional>

namespace Konveyor::Config
{

std::optional<quint32> usKeycode(quint32 keysym);
bool layoutTypesKeysym(xkb_keymap *keymap, xkb_layout_index_t layout, quint32 keysym);

}
