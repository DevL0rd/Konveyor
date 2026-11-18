# Konveyor NixOS Implementation Summary

## Overview
Successfully implemented a NixOS module + Home Manager module pair for Konveyor that:
- Builds from local source
- Uses deterministic plugin IDs based on git commit hash
- Includes the Plasma-Shared submodule from GitHub
- Installs KWin effect, widgets, services, and config declaratively

## Files Created/Modified

### Package (`pkg/default.nix`)
- Builds Konveyor from flake's source (`self`)
- Accepts `plasmaShared` submodule as parameter
- Copies Plasma-Shared to `$out/share/konveyor/widgets/shared/common` in `postInstall`
- Generates deterministic plugin IDs from git commit hash passed from flake
- Stages widgets exactly like `widgets/install.sh` but immutably in the Nix store

### Flake (`flake.nix`)
- Inputs: nixpkgs, home-manager
- Fetches Plasma-Shared submodule from GitHub with fixed rev and hash
- Computes git short hash from `self.rev` for deterministic plugin IDs
- Exposes:
  - `packages.x86_64-linux.konveyor` - the package
  - `nixosModules.konveyor` - NixOS module
  - `homeManagerModules.konveyor` - Home Manager module

### NixOS Module (`modules/nixos/konveyor.nix`)
- Option: `programs.konveyor.enable`
- Option: `programs.konveyor.enableWidgets` (default: true)
- Adds konveyor to `environment.systemPackages`
- Sets `QT_PLUGIN_PATH`, `QML_IMPORT_PATH`, `QML2_IMPORT_PATH`, `QML_XHR_ALLOW_FILE_READ` via `environment.sessionVariables`
- Enables `services.desktopManager.plasma6`

### Home Manager Module (`modules/home-manager/konveyor.nix`)
- Option: `programs.konveyor.enable`
- Asserts NixOS module has `enableWidgets = true`
- Installs widgets via `home.file` symlinks from store
- Enables KWin effect via `programs.plasma.configFile.kwinrc.Plugins` with versioned plugin IDs
- Disables unversioned upstream plugin names
- Sets immutable global shortcuts via `programs.plasma.configFile.kglobalshortcutsrc`
- Installs systemd user services: `konveyor-widgets`, `konveyor-kontrol-panel`
- Registers D-BUS service and .desktop files via `xdg.dataFile`
- Seeds widget configs via `home.activation`
- Runs `kbuildsycoca6` via `home.activation`

## Loopholes Fixed

| # | Issue | Fix |
|---|-------|-----|
| 1 | Non-deterministic plugin IDs (timestamp) | Generated from `git rev-parse --short HEAD` |
| 2 | Fixed GitHub rev vs local source | Uses flake's `self` as source |
| 3 | Imperative `kwriteconfig6` | Declarative `programs.plasma.configFile.kwinrc` |
| 4 | Imperative `kpackagetool6` | `home.file` symlinks from store |
| 5 | Imperative systemd services | Declarative `systemd.user.services` |
| 6 | Imperative `systemctl --user set-environment` | `environment.sessionVariables` + `xdg.configFile.environment.d` |
| 7 | Imperative `kglobalaccel` shortcuts | Declarative `kglobalshortcutsrc` with `immutable = true` |
| 8 | Runtime desktop containment patch | Patched at build time in derivation |
| 9 | Package-manager update hooks | Not registered; Nix handles rebuilds |
| 10 | Toolbox/distrobox build logic | Standard `stdenv.mkDerivation` |
| 11 | Custom fingerprint/ABI detection | Nix's dependency tracking |
| 12 | Imperative widget config seeding | `home.activation` one-time seed |
| 13 | Imperative panel/taskbar config | Declarative or manual setup |

## Usage

### In your flake.nix:
```nix
{
  inputs.konveyor.url = "file:///home/sunny/konveyor";
  inputs.konveyor.flake = false;  # if not a flake yet
  
  outputs = { self, nixpkgs, konveyor, ... }: {
    nixosConfigurations.myhost = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        konveyor.nixosModules.konveyor
        konveyor.homeManagerModules.konveyor
        ({ config, pkgs, ... }: {
          programs.konveyor = {
            enable = true;
            enableWidgets = true;
          };
        })
      ];
    };
  };
}
```

### Or directly in NixOS configuration:
```nix
{ config, pkgs, ... }:
{
  imports = [ <konveyor/nixosModules/konveyor> ];
  
  programs.konveyor = {
    enable = true;
    enableWidgets = true;
  };
}
```

## Verification

After `nixos-rebuild switch` and login to Plasma:
- `qdbus org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded konveyor_effect_<hash>` → `true`
- Widgets appear in "Add Widgets"
- Meta+Shift+P shows cheatsheet (config.kdl)
- Meta+G opens Kontrol Panel
- `systemctl --user status konveyor-widgets` → active
- `systemctl --user status konveyor-kontrol-panel` → active

## Key Features

1. **Deterministic builds**: Same git commit = same plugin IDs = reproducible
2. **Submodule handling**: Plasma-Shared fetched from GitHub with fixed rev/hash
3. **Declarative config**: All KWin, shortcuts, services, widgets managed by Nix
4. **No imperative state**: No `kwriteconfig6`, `kpackagetool6`, `systemctl --user set-environment`
5. **Clean uninstall**: `programs.konveyor.enable = false` removes all traces