# Nix implementation summary

The `nix` branch provides a reusable flake for Konveyor. See [NIX.md](NIX.md)
for usage and the full option list.

## What the flake provides

- `packages.<system>.konveyor` (and `default`) for `x86_64-linux` and
  `aarch64-linux`.
- `overlays.default`, adding `pkgs.konveyor`.
- `nixosModules.default` / `nixosModules.konveyor`.
- `homeManagerModules.default` / `homeManagerModules.konveyor`.

The modules are passed the flake's own package through `_module.args`, so they
work without requiring users to apply the overlay first.

## Design decisions

- **Package**: built from the flake source; `Plasma-Shared` is fetched at a
  fixed revision and staged into `share/konveyor/widgets/shared/common`. The
  widget tree is made writable before staging, and the shared files are copied
  with `cp -RL "${plasmaShared}/." .../` so they land in `common` directly.
- **Plugin ids**: derived from the flake revision and exported through
  `passthru`; no eval-time reads of `$out`.
- **NixOS module**: installs the package, exports plugin/QML paths, and only
  enables Plasma or writes `/etc/environment.d` when asked.
- **Home Manager module**: installs widgets, user services and desktop entries;
  enables versioned KWin plugins by default; seeds user configuration once.
- **UI-friendly defaults**: `config.kdl` and widget config JSON files are seeded
  once and then user-owned. Shortcuts and panel layout are not managed unless
  explicitly enabled.

## Verification

Run:

```sh
nix flake check
nix build .#konveyor
```

Then check `passthru.pluginId` / `passthru.telemetryPluginId` and the staged
widget tree under `share/konveyor/widgets`.
