# Using Konveyor with Nix

The flake at `github:DevL0rd/Konveyor` provides a package, a NixOS module, a Home Manager module and an overlay for `x86_64-linux` and `aarch64-linux`. It needs a git revision (a git checkout or a locked flake input) because the KWin plugin ids are named after it.

## Install

```nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    konveyor = {
      url = "github:DevL0rd/Konveyor";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.home-manager.follows = "home-manager";
    };
  };

  outputs = { nixpkgs, home-manager, konveyor, ... }: {
    nixosConfigurations.myhost = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        konveyor.nixosModules.default
        home-manager.nixosModules.home-manager
        {
          programs.konveyor.enable = true;
          home-manager.users.me.imports = [ konveyor.homeModules.default ];
        }
      ];
    };
  };
}
```

The Home Manager module follows `programs.konveyor.enableWidgets` of the NixOS module. For the package alone, use `konveyor.packages.<system>.konveyor` or `konveyor.overlays.default`.

## NixOS options (`programs.konveyor`)

| Option | Default | Purpose |
| --- | --- | --- |
| `enable` | `false` | Install the package and set the plugin and QML paths. |
| `package` | flake package | Use a custom build. |
| `enableWidgets` | `true` | Enable the Home Manager widgets for users on this host. |
| `managePlasma` | `false` | Enable Plasma 6 from this module. |

## Home Manager options (`programs.konveyor`)

| Option | Default | Purpose |
| --- | --- | --- |
| `enable` | `osConfig.programs.konveyor.enableWidgets` or `false` | Enable widgets, services and desktop entries. |
| `package` | flake package | Use a custom build. |
| `kontrolPanelOnly` | `false` | Install only the Kontrol Panel, like `widgets/install.sh --kontrol-panel-only`. |
| `hideDesktopWidgets` | `true` | Replace Plasma's desktop containment with the patched one that hides desktop widgets under windows. |
| `configKdl` | `null` | Custom `config.kdl` source. |
| `manageConfig` | `false` | Nix owns `config.kdl`; otherwise it is seeded once and left to the Konveyor UI. |
| `manageKwinPlugin` | `true` | Enable the versioned KWin plugins in `kwinrc`. |
| `manageShortcuts` | `false` | Own the Kontrol Panel and Games shortcuts. |
| `enableServices` | `true` | Install the `konveyor-widgets` and `konveyor-kontrol-panel` user services. |
| `enableSycoca` | `true` | Refresh the KDE desktop-file cache on activation. |
| `extraPackages` | `[ ]` | Extra packages for the user environment. |
