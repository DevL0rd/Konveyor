#include "config/keypositions.h"

namespace Konveyor::Config
{

namespace
{

xkb_keymap *usKeymap()
{
    static xkb_keymap *const keymap = [] {
        xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!context) {
            return static_cast<xkb_keymap *>(nullptr);
        }
        const xkb_rule_names names {"evdev", "pc105", "us", "", ""};
        xkb_keymap *result = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
        xkb_context_unref(context);
        return result;
    }();
    return keymap;
}

bool levelProduces(xkb_keymap *keymap, xkb_keycode_t keycode, xkb_layout_index_t layout, xkb_level_index_t level, xkb_keysym_t wanted)
{
    const xkb_keysym_t *symbols = nullptr;
    const int count = xkb_keymap_key_get_syms_by_level(keymap, keycode, layout, level, &symbols);
    for (int index = 0; index < count; ++index) {
        if (xkb_keysym_to_lower(symbols[index]) == wanted) {
            return true;
        }
    }
    return false;
}

}

std::optional<quint32> usKeycode(quint32 keysym)
{
    xkb_keymap *keymap = usKeymap();
    if (!keymap) {
        return std::nullopt;
    }
    const xkb_keysym_t wanted = xkb_keysym_to_lower(keysym);
    for (xkb_keycode_t keycode = xkb_keymap_min_keycode(keymap); keycode <= xkb_keymap_max_keycode(keymap); ++keycode) {
        if (levelProduces(keymap, keycode, 0, 0, wanted)) {
            return keycode;
        }
    }
    return std::nullopt;
}

bool layoutTypesKeysym(xkb_keymap *keymap, xkb_layout_index_t layout, quint32 keysym)
{
    if (!keymap) {
        return true;
    }
    const xkb_keysym_t wanted = xkb_keysym_to_lower(keysym);
    for (xkb_keycode_t keycode = xkb_keymap_min_keycode(keymap); keycode <= xkb_keymap_max_keycode(keymap); ++keycode) {
        const xkb_layout_index_t layouts = xkb_keymap_num_layouts_for_key(keymap, keycode);
        if (layouts == 0) {
            continue;
        }
        const xkb_layout_index_t keyLayout = layout < layouts ? layout : 0;
        const xkb_level_index_t levels = xkb_keymap_num_levels_for_key(keymap, keycode, keyLayout);
        for (xkb_level_index_t level = 0; level < levels; ++level) {
            if (levelProduces(keymap, keycode, keyLayout, level, wanted)) {
                return true;
            }
        }
    }
    return false;
}

}
