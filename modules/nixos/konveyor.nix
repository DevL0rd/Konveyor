{ lib, config, ... }:

let
  cfg = config.programs.konveyor;
in
{
  options.programs.konveyor = {
    enable = lib.mkEnableOption "Konveyor KWin effect, KCM and CLI";

    package = lib.mkOption {
      type = lib.types.package;
      defaultText = lib.literalExpression "inputs.konveyor.packages.<system>.konveyor";
      description = ''
        The Konveyor package to install. The flake module sets it to the
        flake's own package; set it only to use a custom build.
      '';
    };

    enableWidgets = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = ''
        Tell the Home Manager module to enable widgets, services and panel
        integration for users on this host.
      '';
    };

    managePlasma = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        Enable Plasma 6 from this module. Leave this off if your desktop is
        already managed elsewhere.
      '';
    };
  };

  config = lib.mkMerge [
    (lib.mkIf cfg.enable {
      environment.systemPackages = [ cfg.package ];

      environment.sessionVariables = {
        QT_PLUGIN_PATH = [ "${cfg.package}/lib/qt-6/plugins" ];
        QML_IMPORT_PATH = [ "${cfg.package}/lib/qt-6/qml" ];
        QML2_IMPORT_PATH = [ "${cfg.package}/lib/qt-6/qml" ];
      };
    })

    (lib.mkIf (cfg.enable && cfg.managePlasma) {
      services.desktopManager.plasma6.enable = true;
    })
  ];
}
