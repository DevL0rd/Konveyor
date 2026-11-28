# Using Konveyor with Nix

This fork exposes a flake with a reusable NixOS module, a Home Manager module,
and an overlay. It works on `x86_64-linux` and `aarch64-linux`.

## Quick start

```nix
# flake.nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    konveyor = {
      url = "github:sunnysamantara/Konveyor/nix";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.home-manager.follows = "home-manager";
    };
  };

  outputs = { self, nixpkgs, home-manager, konveyor, ... }:
    let
      system = "x86_64-linux";
    in
    {
      nixosConfigurations.myhost = nixpkgs.lib.nixosSystem {
        inherit system;
        modules = [
          konveyor.nixosModules.default
          home-manager.nixosModules.home-manager
          {
            programs.konveyor = {
              enable = true;
              # Set this if Plasma isn't already enabled elsewhere:
              # managePlasma = true;
            };

            home-manager.users.me = {
              imports = [ konveyor.homeManagerModules.default ];
              # Usually unnecessary: follows programs.konveyor.enableWidgets.
              programs.konveyor.enable = true;
            };
          }
        ];
      };
    };
}
```

The flake also provides `packages.<system>.konveyor` and `overlays.default`:

```nix
pkgs = import nixpkgs {
  inherit system;
  overlays = [ konveyor.overlays.default ];
};
# now pkgs.konveyor is available
```

## Module options

### NixOS (`programs.konveyor`)

| Option | Default | Purpose |
| --- | --- | --- |
| `enable` | `false` | Install the package and set plugin/QML environment variables. |
| `package` | flake package | Override with a custom build. |
| `enableWidgets` | `true` | Tell the Home Manager module to enable widgets/services. |
| `managePlasma` | `false` | Enable Plasma 6 from this module. Leave off if Plasma is already managed. |
| `manageEnvironmentFile` | `true` | Write `/etc/environment.d/konveyor.conf`. |

### Home Manager (`programs.konveyor`)

| Option | Default | Purpose |
| --- | --- | --- |
| `enable` | follows `osConfig.programs.konveyor.enableWidgets` | Enable widgets, services and desktop entries. |
| `package` | flake package | Override with a custom build. |
| `configKdl` | `null` | Custom `config.kdl` source. |
| `manageConfig` | `false` | If true, Nix owns `config.kdl` and replaces UI edits; if false, seed once only. |
| `manageKwinPlugin` | `true` | Enable versioned KWin plugins in `kwinrc`. |
| `manageShortcuts` | `false` | Own the Kontrol Panel/Games shortcut entries. Off by default so KDE's Shortcuts UI stays authoritative. |
| `enableServices` | `true` | Install `konveyor-widgets` and `konveyor-kontrol-panel` user services. |
| `enableSycoca` | `true` | Refresh the KDE desktop-file cache on activation. |
| `extraPackages` | `[ ]` | Extra helper packages for the user environment. |

## UI-friendly by default

Konveyor's own settings (`~/.config/konveyor/config.kdl` and the widget
`config.json` files) are **seeded once if absent** and then left alone. Editing
them in Konveyor/KDE survives later `nixos-rebuild switch` runs.

Plasma shortcuts and panel layout are deliberately not managed by default:

- set `programs.konveyor.manageShortcuts = true` only if you want Nix to own
  those shortcut entries;
- panel widgets remain yours to arrange in Plasma;
- set `manageConfig = true` only if you prefer fully declarative `config.kdl`.

The KWin effect is enabled by default. If you want System Settings to own that
toggle instead, set `manageKwinPlugin = false`.

## Plugin ids

Plugin ids are derived from the locked flake revision, for example
`konveyor_effect_5c6738e`. The package exports them as
`passthru.pluginId` and `passthru.telemetryPluginId`, and the Home Manager
module uses those values to enable the correct KWin plugins and disable stale
unversioned ones.

For path flakes without a git revision, the suffix falls back to `shortRev`
and finally `"dev"`, so local development still evaluates.
