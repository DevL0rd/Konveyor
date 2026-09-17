# Linux Plasma Screen Rotate

A small KDE Plasma 6 panel widget that rotates the screen between landscape and portrait with one click.

Intended for streaming a desktop to a phone or tablet, where the client cannot rotate the host display on its own. The Moonlight and Sunshine protocol fixes the stream resolution when the session starts and has no message to change it afterwards, so rotation has to happen on the host side.

Every enabled output is rotated. The icon reflects the current orientation.

## Requirements

- KDE Plasma 6 on Wayland
- `kscreen-doctor`, `jq`, and `kpackagetool6`

## Install

```bash
chmod +x install.sh uninstall.sh
./install.sh
```

Then add **Screen Rotate** to a panel from the widget list.

## Uninstall

```bash
./uninstall.sh
```

Remove the widget from your panel if it is still shown.

## Behaviour

The panel button draws a small screen that turns to match the current orientation and pulses while a rotation is being applied. Clicking it switches between landscape and portrait. On the desktop the widget shows a card with the current orientation and a rotate button.

Rotation is applied with `kscreen-doctor output.<name>.rotation.left` and `output.<name>.rotation.none`.

## Notes

Rotating changes how the desktop is drawn, not the resolution the stream negotiated. On a client streaming a landscape resolution, a portrait desktop will be letterboxed. Reconnecting after rotating lets the client request a matching resolution, provided that mode exists on the display.

## License

GPL-3.0-or-later
