{
  description = "Konveyor - KWin tiling window manager with Plasma widgets";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, home-manager, ... }@inputs:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
      lib = pkgs.lib;

      # Get the git short hash from the flake's self
      gitShortHash = builtins.substring 0 7 self.rev;

      # Get the Plasma-Shared submodule
      plasmaShared = pkgs.fetchgit {
        url = "https://github.com/DevL0rd/Plasma-Shared.git";
        rev = "cd5299c0720c64071da0be8bfc038783ed13a4af";
        fetchSubmodules = false;
        hash = "sha256-m0GhZlyQ1Ttnvd7G+FD6+ywqtzqvvs99oOq7RDHJAd0=";
      };

      # Build Konveyor from local source (flake's source tree)
      konveyor = pkgs.callPackage ./pkg { 
        src = self;
        plasmaShared = plasmaShared;
        pluginId = "konveyor_effect_${gitShortHash}";
        telemetryPluginId = "process_monitor_telemetry_${gitShortHash}";
      };

      # NixOS module
      nixosModule = { config, lib, pkgs, ... }:
        (import ./modules/nixos/konveyor.nix {
          inherit lib pkgs config;
        });

      # Home Manager module
      homeManagerModule = { config, lib, pkgs, osConfig, ... }:
        (import ./modules/home-manager/konveyor.nix {
          inherit lib pkgs config osConfig;
        });
    in
    {
      inherit konveyor;

      nixosModules.konveyor = nixosModule;
      homeManagerModules.konveyor = homeManagerModule;

      # Package for direct use
      packages.${system}.konveyor = konveyor;

      # NixOS configurations (empty - user provides their own)
      nixosConfigurations = { };
    };
}