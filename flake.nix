{
  description = "Konveyor - KWin tiling window manager with Plasma widgets";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    plasma-manager = {
      url = "github:nix-community/plasma-manager";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.home-manager.follows = "home-manager";
    };
  };

  outputs = { self, nixpkgs, home-manager, plasma-manager, ... }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];

      forAllSystems = f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            inherit system;
            pkgs = import nixpkgs {
              inherit system;
              config.allowUnfree = true;
            };
          }
        );

      # A locked flake always has `rev`; path flakes (common in development)
      # can have `rev = null`, so fall back to `shortRev` and finally "dev".
      pluginSuffix =
        if (self.rev or null) != null then
          builtins.substring 0 7 self.rev
        else if (self.shortRev or null) != null then
          self.shortRev
        else
          "dev";

      makeKonveyor = { pkgs, ... }:
        let
          plasmaShared = pkgs.fetchgit {
            url = "https://github.com/DevL0rd/Plasma-Shared.git";
            rev = "cd5299c0720c64071da0be8bfc038783ed13a4af";
            hash = "sha256-m0GhZlyQ1Ttnvd7G+FD6+ywqtzqvvs99oOq7RDHJAd0=";
          };
        in
        pkgs.callPackage ./pkg {
          src = self;
          inherit plasmaShared;
          pluginId = "konveyor_effect_${pluginSuffix}";
          telemetryPluginId = "process_monitor_telemetry_${pluginSuffix}";
        };

      packagesFor = { pkgs, ... }:
        let
          konveyor = makeKonveyor { inherit pkgs; };
        in
        {
          default = konveyor;
          inherit konveyor;
        };

      nixosModule = { pkgs, ... }: {
        imports = [ ./modules/nixos/konveyor.nix ];
        _module.args.konveyorPackage =
          self.packages.${pkgs.stdenv.hostPlatform.system}.konveyor;
      };

      homeManagerModule = { pkgs, ... }: {
        imports = [
          plasma-manager.homeModules.plasma-manager
          ./modules/home-manager/konveyor.nix
        ];
        _module.args.konveyorPackage =
          self.packages.${pkgs.stdenv.hostPlatform.system}.konveyor;
      };
    in
    {
      packages = forAllSystems packagesFor;

      overlays.default = final: prev: {
        konveyor = self.packages.${prev.system}.konveyor;
      };

      nixosModules = {
        default = nixosModule;
        konveyor = nixosModule;
      };

      homeManagerModules = {
        default = homeManagerModule;
        konveyor = homeManagerModule;
      };

      # Alias used by some home-manager setups.
      homeModules = {
        default = homeManagerModule;
        konveyor = homeManagerModule;
      };

      checks = forAllSystems ({ system, pkgs }: {
        konveyor-passthru = pkgs.runCommand "konveyor-passthru-check" { } ''
          plugin="${self.packages.${system}.konveyor.passthru.pluginId}"
          telemetry="${self.packages.${system}.konveyor.passthru.telemetryPluginId}"
          test "$plugin" = "konveyor_effect_${pluginSuffix}"
          test "$telemetry" = "process_monitor_telemetry_${pluginSuffix}"
          printf '%s\n%s\n' "$plugin" "$telemetry" > "$out"
        '';
      });
    };
}
