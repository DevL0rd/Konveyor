{ lib, pkgs, config, ... }:

let
  cfg = config.programs.konveyor;
in
{
  options.programs.konveyor = {
    enable = lib.mkEnableOption "Konveyor KWin effect, KCM, CLI and widgets";

    source = lib.mkOption {
      type = lib.types.nullOr lib.types.path;
      default = null;
      description = "Path to Konveyor source directory. If null, uses the flake's source (when used via flake).";
      example = "/path/to/konveyor";
    };

    enableWidgets = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = "Also enable widgets, services and panel integration (requires Home Manager module)";
    };
  };

  config = lib.mkIf cfg.enable {
    # Build Konveyor from local source
    # The package is defined in the flake and uses the flake's source by default.
    # If source is overridden, it passes the custom source to the package.
    environment.systemPackages = [
      (pkgs.callPackage ../../pkg { src = cfg.source; })
    ];

    # Critical: KWin finds effect via QT_PLUGIN_PATH
    # QML settings UI finds via QML_IMPORT_PATH
    # Widgets read their collectors' JSON snapshots via QML XHR
    environment.sessionVariables = {
      QT_PLUGIN_PATH = [ "${pkgs.konveyor}/lib/qt-6/plugins" ];
      QML_IMPORT_PATH = [ "${pkgs.konveyor}/lib/qt-6/qml" ];
      QML2_IMPORT_PATH = [ "${pkgs.konveyor}/lib/qt-6/qml" ];
      QML_XHR_ALLOW_FILE_READ = "1";
    };

    # Ensure plasma6 is enabled (required for KWin effect)
    services.desktopManager.plasma6.enable = true;

    # Make the session variables available in the user session as well
    environment.etc."environment.d/konveyor.conf".source = pkgs.writeText "konveyor-env.conf" ''
      QT_PLUGIN_PATH=${pkgs.konveyor}/lib/qt-6/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
      QML_IMPORT_PATH=${pkgs.konveyor}/lib/qt-6/qml${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}
      QML2_IMPORT_PATH=${pkgs.konveyor}/lib/qt-6/qml${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}
      QML_XHR_ALLOW_FILE_READ=1
    '';
  };
}