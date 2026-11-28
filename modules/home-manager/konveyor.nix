{ lib, pkgs, config, ... }@args:

let
  cfg = config.programs.konveyor;

  # `osConfig` only exists when home-manager runs as a NixOS module. Using
  # `@args` keeps the module usable in standalone home-manager configurations.
  osConfig = args.osConfig or { };
  nixosWidgetsEnabled = osConfig.programs.konveyor.enableWidgets or false;

  konveyor = cfg.package;
  widgets = "${konveyor}/share/konveyor/widgets";
  sourceWidgets = "${konveyor.src}/widgets";
  pluginId = konveyor.passthru.pluginId;
  telemetryPluginId = konveyor.passthru.telemetryPluginId;
  konveyorPanel = "${widgets}/portals/kontrol-panel";

  # The shipped config is the documented baseline. Users can supply their own
  # with `configKdl`; by default it is seeded once and then owned by the user
  # (and by Konveyor's UI), not replaced on every switch.
  defaultConfigSource =
    if cfg.configKdl != null then
      cfg.configKdl
    else
      "${konveyor}/share/konveyor/default-config.kdl";

  routerExample = builtins.fromJSON (builtins.readFile "${sourceWidgets}/router-monitor/config.example.json");
  portalExample = builtins.fromJSON (builtins.readFile "${sourceWidgets}/portals/config.example.json");

  # Mutable seeds: these files hold credentials/router details, so they are
  # created once and then owned by the widget settings UI rather than
  # symlinked into the read-only store.
  routerSeed = pkgs.writeText "konveyor-router-config.json" (
    builtins.toJSON (
      routerExample
      // {
        adguard = routerExample.adguard // { enabled = false; };
      }
    )
  );
  portalSeed = pkgs.writeText "konveyor-portal-config.json" (
    builtins.toJSON (portalExample // { steam_api_key = ""; })
  );
  kontrolPanelSeed = pkgs.writeText "konveyor-kontrolpanelrc" ''
    [General]
    openPageOnStart=shortcuts
  '';
  taskbarSeed = pkgs.writeText "konveyor-taskbarrc" ''
    [General]
  '';

  kontrolPanelOnly = cfg.kontrolPanelOnly;

  plasmoidDirs = lib.optionals (!kontrolPanelOnly) [
    "system-monitor/plasmoids/org.devl0rd.sysmon"
    "system-monitor/plasmoids/org.devl0rd.sysmon.overlay"
    "system-monitor/plasmoids/org.devl0rd.sysmon.panel"
    "process-monitor/plasmoids/org.devl0rd.procmon"
    "process-monitor/plasmoids/org.devl0rd.procmon.overlay"
    "process-monitor/plasmoids/org.devl0rd.procmon.panel"
    "router-monitor/plasmoids/org.devl0rd.routermon.clients"
    "router-monitor/plasmoids/org.devl0rd.routermon.dns"
    "router-monitor/plasmoids/org.devl0rd.routermon.network"
    "router-monitor/plasmoids/org.devl0rd.routermon.overlay"
    "router-monitor/plasmoids/org.devl0rd.routermon.panel"
    "router-monitor/plasmoids/org.devl0rd.routermon.speedtest"
    "router-monitor/plasmoids/org.devl0rd.routermon.system"
    "router-monitor/plasmoids/org.devl0rd.routermon.wifi"
    "system-log/plasmoids/org.devl0rd.logmon.journal"
    "portals/plasmoids/org.devl0rd.portal"
    "portals/plasmoids/org.devl0rd.portal.friends"
    "portals/plasmoids/org.devl0rd.portal.launcher"
    "screen-rotate/plasmoid"
    "taskbar/plasmoids/org.devl0rd.taskbar"
  ];

  # Read each metadata.json so the user-visible directory is the plugin id
  # even for the one source directory that isn't named that way.
  plasmoids = builtins.listToAttrs (
    map (
      dir:
      let
        metadata = builtins.fromJSON (builtins.readFile "${sourceWidgets}/${dir}/metadata.json");
      in
      {
        name = metadata.KPlugin.Id;
        value = "${widgets}/${dir}";
      }
    ) plasmoidDirs
  );

  kontrolPanelCommandPaths = [
    "portals/bin/portal-games"
    "portals/bin/portal-packages"
    "portals/bin/portal-launcher"
    "portals/bin/portal-friends"
  ];

  widgetCommandPaths = [
    "system-monitor/bin/sysmon-collect"
    "process-monitor/bin/procmon-collect"
    "router-monitor/bin/routermon-collect"
    "router-monitor/bin/routermon-ctl"
    "router-monitor/bin/routermon-config"
    "router-monitor/bin/routermon-speedtest"
    "system-log/bin/logmon-collect"
    "service/monitor-overlay"
    "screen-rotate/bin/linux-plasma-screen-rotate"
  ];

  plasmoidFiles = lib.concatMapAttrs (
    id: path: {
      ".local/share/plasma/plasmoids/${id}".source = path;
    }
  ) plasmoids;

  commandPaths = kontrolPanelCommandPaths ++ lib.optionals (!kontrolPanelOnly) widgetCommandPaths;

  commandFiles = lib.listToAttrs (
    map (
      path: {
        name = ".local/bin/${builtins.baseNameOf path}";
        value.source = "${widgets}/${path}";
      }
    ) commandPaths
  );

  # Seed once, never overwrite: this is what lets users edit settings in
  # Konveyor's own UI and have those edits survive later switches.
  seedFile = path: mode: source: ''
    if [ ! -e "$HOME/${path}" ]; then
      run ${pkgs.coreutils}/bin/install -m ${toString mode} ${source} "$HOME/${path}"
    fi
  '';
in
{
  options.programs.konveyor = {
    enable = lib.mkOption {
      type = lib.types.bool;
      default = nixosWidgetsEnabled;
      defaultText = lib.literalExpression "osConfig.programs.konveyor.enableWidgets or false";
      description = ''
        Enable Konveyor widgets, user services and desktop entries. When used
        as a NixOS module together with `programs.konveyor.enable`, this
        follows `programs.konveyor.enableWidgets` automatically.
      '';
    };

    package = lib.mkOption {
      type = lib.types.package;
      defaultText = lib.literalExpression "inputs.konveyor.packages.<system>.konveyor";
      description = ''
        The Konveyor package whose widgets and binaries are wired up. The flake
        module sets it to the flake's own package.
      '';
    };

    kontrolPanelOnly = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        Install only the Kontrol Panel, like `widgets/install.sh
        --kontrol-panel-only`: its service, helper commands and friends
        collector, without the widgets, the other collectors, the Games
        shortcut or the desktop-widget hider.
      '';
    };

    hideDesktopWidgets = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = ''
        Replace Plasma's `org.kde.desktopcontainment` with a copy patched, at
        build time, against the pinned Plasma so desktop widgets can hide while
        windows cover the desktop. The widget installer does the same by
        default. Ignored with `kontrolPanelOnly`.
      '';
    };

    configKdl = lib.mkOption {
      type = lib.types.nullOr lib.types.path;
      default = null;
      description = ''
        Optional `config.kdl` source. When null, the package's shipped default
        is used. It is seeded once unless `manageConfig` is true.
      '';
    };

    manageConfig = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        If true, Nix owns `~/.config/konveyor/config.kdl` and replaces UI edits
        on every switch. If false (default), the file is seeded once and then
        left to the user/Konveyor UI.
      '';
    };

    manageKwinPlugin = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = ''
        Declaratively enable the versioned KWin effect plugins in `kwinrc`.
        Disable this if you prefer toggling the effect from System Settings.
      '';
    };

    manageShortcuts = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        Declaratively own `kglobalshortcutsrc` entries for the Kontrol Panel
        and the Games page. Disabled by default so shortcuts remain editable
        from KDE's UI.
      '';
    };

    enableServices = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = "Install the konveyor-widgets and konveyor-kontrol-panel user services.";
    };

    enableSycoca = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = "Refresh the KDE desktop-file cache on activation.";
    };

    extraPackages = lib.mkOption {
      type = lib.types.listOf lib.types.package;
      default = [ ];
      description = "Extra packages to add alongside Konveyor and its helper tools.";
    };
  };

  config = lib.mkIf cfg.enable {
    home.packages =
      with pkgs;
      [
        konveyor
        jq # screen-rotate
        glib # gdbus, used by the portal helper scripts
        kdePackages.libkscreen # kscreen-doctor
        openssh # router monitor
        curl # router speed test
        iputils # ping
      ]
      ++ cfg.extraPackages;

    # KWin loads the versioned effect names; also disable the unversioned
    # upstream names so an old install cannot load a stale ABI-mismatched .so.
    programs.plasma.configFile."kwinrc" = lib.mkIf cfg.manageKwinPlugin {
      Plugins = {
        "konveyor_effectEnabled" = false;
        "${pluginId}Enabled" = true;
        "process_monitor_telemetryEnabled" = false;
        "${telemetryPluginId}Enabled" = true;
      };
    };

    # Opt-in because these keys are user-owned in KDE's Shortcuts UI. When
    # enabled, immutable entries are used because KGlobalAccel rewrites
    # kglobalshortcutsrc from its in-memory registry on logout/relogin.
    programs.plasma.configFile."kglobalshortcutsrc" = lib.mkIf cfg.manageShortcuts {
      plasmashell."activate application launcher" = {
        value = "none,,";
        immutable = true;
      };
      kwin."Grid View" = {
        value = "none,,";
        immutable = true;
      };
      "konveyor-kontrol-panel".toggle = {
        value = "Meta\tAlt+F1,,";
        immutable = true;
      };
      "services/org.devl0rd.portal.launcher.games.desktop"._launch = "Meta+G";
    };

    xdg.configFile =
      lib.optionalAttrs cfg.manageConfig {
        "konveyor/config.kdl".source = defaultConfigSource;
      }
      // lib.optionalAttrs (!kontrolPanelOnly) {
        "environment.d/konveyor-widgets.conf".text = ''
          QML_XHR_ALLOW_FILE_READ=1
        '';
        "systemd/user/plasma-plasmashell.service.d/konveyor-widgets.conf".text = ''
          [Service]
          Environment=QML_XHR_ALLOW_FILE_READ=1
        '';
      };

    # Widgets run inside plasmashell and read their collectors' snapshots with
    # QML XHR, so plasmashell needs QML_XHR_ALLOW_FILE_READ, as the widget
    # installer's configure_file_access sets it. The Kontrol Panel service
    # sets it on its own unit.
    systemd.user.services = lib.mkIf cfg.enableServices {
      konveyor-widgets = {
        Unit = {
          Description = "Konveyor widget collectors";
          After = [ "graphical-session.target" ];
        };
        Service = {
          Type = "simple";
          ExecStart = "${widgets}/service/konveyor-widgets${lib.optionalString kontrolPanelOnly " portals"}";
          Restart = "always";
          RestartSec = 3;
          Nice = 19;
        };
        Install = {
          WantedBy = [ "default.target" ];
        };
      };

      konveyor-kontrol-panel = {
        Unit = {
          Description = "Konveyor Kontrol Panel";
          PartOf = [ "graphical-session.target" ];
          After = [ "graphical-session.target" ];
        };
        Service = {
          Type = "dbus";
          BusName = "org.devl0rd.KontrolPanel";
          ExecStart = "${konveyor}/bin/konveyor-kontrol-panel ${konveyorPanel}";
          Environment = [ "QML_XHR_ALLOW_FILE_READ=1" ];
          Restart = "on-failure";
          RestartSec = 2;
        };
        Install = {
          WantedBy = [ "graphical-session.target" ];
        };
      };
    };

    xdg.dataFile = {
      "dbus-1/services/org.devl0rd.KontrolPanel.service".text = ''
        [D-BUS Service]
        Name=org.devl0rd.KontrolPanel
        Exec=${konveyor}/bin/konveyor-kontrol-panel ${konveyorPanel}
        SystemdService=konveyor-kontrol-panel.service
      '';
      "applications/org.devl0rd.KontrolPanel.desktop".text = ''
        [Desktop Entry]
        Type=Application
        Name=Kontrol Panel
        Comment=Apps, games, files, friends and system actions
        Exec=${widgets}/portals/bin/portal-launcher toggle
        Icon=start-here-kde-plasma-symbolic
        NoDisplay=true
        StartupNotify=false
      '';
      "applications/konveyor-kontrol-panel.desktop".text = ''
        [Desktop Entry]
        Type=Application
        Name=Kontrol Panel service
        Comment=Lets the Kontrol Panel list open windows in its search
        Exec=${konveyor}/bin/konveyor-kontrol-panel ${konveyorPanel}
        Icon=start-here-kde-plasma-symbolic
        NoDisplay=true
        X-KDE-Wayland-Interfaces=org_kde_plasma_window_management
      '';
    } // lib.optionalAttrs (!kontrolPanelOnly) {
      "applications/org.devl0rd.portal.launcher.games.desktop".text = ''
        [Desktop Entry]
        Type=Application
        Name=Kontrol Panel: Games
        Comment=Open the Kontrol Panel on its Games page
        Exec=${widgets}/portals/bin/portal-launcher games
        Icon=input-gamepad-symbolic
        NoDisplay=true
        StartupNotify=false
        X-KDE-Shortcuts=Meta+G
      '';
    };

    home.file = plasmoidFiles // commandFiles // lib.optionalAttrs (cfg.hideDesktopWidgets && !kontrolPanelOnly) {
      ".local/share/plasma/plasmoids/org.kde.desktopcontainment".source =
        "${konveyor}/share/konveyor/desktop-containment/org.kde.desktopcontainment";
    };

    home.activation.konveyorSeedConfigs = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
      run ${pkgs.coreutils}/bin/mkdir -p \
        ${lib.optionalString (!kontrolPanelOnly) ''"$HOME/.config/Linux-System-Monitor" "$HOME/.config/Linux-Process-Mon" "$HOME/.config/Linux-Router-Monitor"''} \
        "$HOME/.config/Plasma-App-Portal" \
        "$HOME/.config/konveyor"

      ${lib.optionalString (!cfg.manageConfig) (seedFile ".config/konveyor/config.kdl" 644 defaultConfigSource)}
      ${lib.optionalString (!kontrolPanelOnly) (seedFile ".config/Linux-System-Monitor/config.json" 644 "${widgets}/system-monitor/config.example.json")}
      ${lib.optionalString (!kontrolPanelOnly) (seedFile ".config/Linux-Process-Mon/config.json" 644 "${widgets}/process-monitor/config.example.json")}
      ${lib.optionalString (!kontrolPanelOnly) (seedFile ".config/Linux-Router-Monitor/config.json" 600 routerSeed)}
      ${seedFile ".config/Plasma-App-Portal/config.json" 600 portalSeed}
      ${seedFile ".config/konveyor/kontrolpanelrc" 644 kontrolPanelSeed}
      ${lib.optionalString (!kontrolPanelOnly) (seedFile ".config/konveyor/taskbarrc" 644 taskbarSeed)}
    '';

    # Make the new .desktop entries (especially Meta+G) known to KDE without
    # needing the upstream installer.
    home.activation.konveyorKSycoca =
      lib.mkIf cfg.enableSycoca
        (lib.hm.dag.entryAfter [ "writeBoundary" ] ''
          run ${pkgs.kdePackages.kservice}/bin/kbuildsycoca6
        '');
  };
}
