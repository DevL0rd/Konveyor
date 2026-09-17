# Linux Plasma Keyboard Toggle

A small KDE Plasma 6 panel widget that opens and closes the on-screen keyboard.

Useful when streaming a desktop to a tablet or phone, where the keyboard normally only appears on a touched text field and there is no other way to summon it.

## Requirements

- KDE Plasma 6 on Wayland
- An on-screen keyboard configured in **System Settings - Keyboard - Virtual Keyboard**, such as `plasma-keyboard`
- `busctl`, `gdbus`, and `kpackagetool6`

## Install

```bash
chmod +x install.sh uninstall.sh
./install.sh
```

Then add **Keyboard Toggle** to a panel from the widget list.

## Uninstall

```bash
./uninstall.sh
```

Remove the widget from your panel if it is still shown.

## Behaviour

Clicking the panel button toggles KWin's virtual keyboard, with a short ripple to confirm the click. If the keyboard is showing it closes; otherwise it opens, even on a laptop that isn't in tablet mode, and your virtual keyboard mode is put back once it closes. If a rotation or screen change leaves it stuck open, closing restarts `plasma-keyboard`. On the desktop the widget shows a card with an Open or close button.

There is no hide action. KWin's virtual keyboard D-Bus interface exposes no way to dismiss the keyboard; setting `active` to false or changing `mode` leaves it visible. The keyboard hides itself when the focused text field loses focus, or through its own close button. For the same reason the widget does not indicate whether the keyboard is currently open.

## Notes

KWin only raises the keyboard automatically when a text field is touched, not when it is clicked with a mouse. That is deliberate upstream behaviour. This widget exists so the keyboard can still be summoned in those cases, including inside XWayland applications that never trigger it.

## License

GPL-3.0-or-later
