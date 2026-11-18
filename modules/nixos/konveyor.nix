{ lib, pkgs, config, konveyorPackage ? null, ... }:

let
  cfg = config.programs.konveyor;
in
{
  options.programs.konveyor = {
    enable = lib.mkEnableOption "Konveyor KWin effect, KCM and CLI";

    package = lib.mkOption {
      type = lib.types.nullOr lib.types.package;
      default = konveyorPackage;
      defaultText = lib.literalExpression "inputs.konveyor.packages.<system>.konveyor";
      description = ''
        The Konveyor package to install. The flake module supplies its own
        package automatically. Set this only when building a custom package.
      '';
    };

    enableWidgets = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = ''
        Advertise to the Home Manager module that widgets, services and panel
        integration should be enabled for users on this host.
      '';
    };

    managePlasma = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        Enable Plasma 6 from this module. Leave this off if your desktop is
        already managed elsewhere (recommended for most NixOS setups).
      '';
    };

    manageEnvironmentFile = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = ''
        Write /etc/environment.d/konveyor.conf in addition to
        `environment.sessionVariables`. Useful for sessions started outside
        PAM. Set to false if you want to own that file yourself.
      '';
    };
  };

  config = lib.mkMerge [
    (lib.mkIf (cfg.enable && cfg.package != null) {
      environment.systemPackages = [ cfg.package ];

      # KWin loads the effect from QT_PLUGIN_PATH, the KCM's QML from
      # QML_IMPORT_PATH, and widgets read collector snapshots with QML XHR.
      environment.sessionVariables = {
        QT_PLUGIN_PATH = [ "${cfg.package}/lib/qt-6/plugins" ];
        QML_IMPORT_PATH = [ "${cfg.package}/lib/qt-6/qml" ];
        QML2_IMPORT_PATH = [ "${cfg.package}/lib/qt-6/qml" ];
        QML_XHR_ALLOW_FILE_READ = "1";
      };
    })

    (lib.mkIf (cfg.enable && cfg.managePlasma) {
      services.desktopManager.plasma6.enable = true;
    })

    (lib.mkIf (cfg.enable && cfg.manageEnvironmentFile && cfg.package != null) {
      # Static assignments keep this file valid for systemd's environment.d
      # syntax. The same values are exported as session variables above.
      environment.etc."environment.d/konveyor.conf".source =
        pkgs.writeText "konveyor-env.conf" ''
          QT_PLUGIN_PATH=${cfg.package}/lib/qt-6/plugins
          QML_IMPORT_PATH=${cfg.package}/lib/qt-6/qml
          QML2_IMPORT_PATH=${cfg.package}/lib/qt-6/qml
          QML_XHR_ALLOW_FILE_READ=1
        '';
    })
  ];
}
