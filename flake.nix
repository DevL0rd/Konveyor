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
          system: f { inherit system; pkgs = nixpkgs.legacyPackages.${system}; }
        );

      sourceRevision =
        self.shortRev or self.dirtyShortRev
          or (throw "Konveyor needs a git revision to name its KWin plugins uniquely; use a git checkout or a locked flake input");

      pluginSuffix = nixpkgs.lib.replaceStrings [ "-" ] [ "_" ] sourceRevision;

      cmakeVersion =
        let
          line = nixpkgs.lib.findFirst
            (l: builtins.match "project\\(konveyor VERSION [0-9.]+ .*" l != null)
            (throw "CMakeLists.txt has no project(konveyor VERSION ...) line")
            (nixpkgs.lib.splitString "\n" (builtins.readFile ./CMakeLists.txt));
        in
        builtins.head (builtins.match "project\\(konveyor VERSION ([0-9.]+) .*" line);

      version = "${cmakeVersion}-unstable-${builtins.substring 0 8 self.lastModifiedDate}-${sourceRevision}";

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
          inherit plasmaShared version;
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

      nixosModule = { lib, pkgs, ... }: {
        imports = [ ./modules/nixos/konveyor.nix ];
        programs.konveyor.package = lib.mkDefault self.packages.${pkgs.stdenv.hostPlatform.system}.konveyor;
      };

      homeManagerModule = { lib, pkgs, ... }: {
        imports = [
          plasma-manager.homeModules.plasma-manager
          ./modules/home-manager/konveyor.nix
        ];
        programs.konveyor.package = lib.mkDefault self.packages.${pkgs.stdenv.hostPlatform.system}.konveyor;
      };
    in
    {
      packages = forAllSystems packagesFor;

      overlays.default = final: prev: {
        konveyor = self.packages.${prev.stdenv.hostPlatform.system}.konveyor;
      };

      nixosModules = {
        default = nixosModule;
        konveyor = nixosModule;
      };

      homeModules = {
        default = homeManagerModule;
        konveyor = homeManagerModule;
      };

      checks = forAllSystems ({ system, ... }: {
        konveyor = self.packages.${system}.konveyor;
      });
    };
}
