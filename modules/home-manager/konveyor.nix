{ lib, pkgs, config, osConfig, ... }:

let
  cfg = config.programs.konveyor;
  enable = cfg.enable;

  konveyor = pkgs.konveyor;
  widgets = "${konveyor}/share/konveyor/widgets";
  pluginId = konveyor.passthru.pluginId;
  telemetryPluginId = konveyor.passthru.telemetryPluginId;
  konveyorPanel = "${widgets}/portals/kontrol-panel";

  # The shipped config is the documented baseline. The only change is the
  # cheatsheet bind: Meta+K is already Launch Konsole, so move Konveyor's
  # overlay to Meta+Shift+P (free in both Konveyor and KDE; Meta+Shift+K is
  # taken by Konveyor's focus-monitor-up). Nix owns this file; GUI edits are
  # replaced on switch.
  defaultConfig = builtins.readFile "${konveyor}/share/konveyor/default-config.kdl";
  configKdl = pkgs.writeText "konveyor-config.kdl" (
    lib.replaceStrings
      [ "Mod+K hotkey-overlay-title=\"Show Shortcut Cheatsheet\" { show-hotkey-overlay; }" ]
      [ "Mod+Shift+P hotkey-overlay-title=\"Show Shortcut Cheatsheet\" { show-hotkey-overlay; }" ]
      defaultConfig
  );

  routerExample = builtins.fromJSON (builtins.readFile "${widgets}/router-monitor/config.example.json");
  portalExample = builtins.fromJSON (builtins.readFile "${widgets}/portals/config.example.json");

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

  plasmoidDirs = [
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
        metadata = builtins.fromJSON (builtins.readFile "${widgets}/${dir}/metadata.json");
      in
      {
        name = metadata.KPlugin.Id;
        value = "${widgets}/${dir}";
      }
    ) plasmoidDirs
  );

  commandPaths = [
    "system-monitor/bin/sysmon-collect"
    "process-monitor/bin/procmon-collect"
    "router-monitor/bin/routermon-collect"
    "router-monitor/bin/routermon-ctl"
    "router-monitor/bin/routermon-config"
    "router-monitor/bin/routermon-speedtest"
    "system-log/bin/logmon-collect"
    "portals/bin/portal-games"
    "portals/bin/portal-packages"
    "portals/bin/portal-launcher"
    "portals/bin/portal-friends"
    "service/monitor-overlay"
    "screen-rotate/bin/linux-plasma-screen-rotate"
  ];

  plasmoidFiles = lib.concatMapAttrs (
    id: path: {
      ".local/share/plasma/plasmoids/${id}".source = path;
    }
  ) plasmoids;

  commandFiles = lib.listToAttrs (
    map (
      path: {
        name = ".local/bin/${builtins.baseNameOf path}";
        value.source = "${widgets}/${path}";
      }
    ) commandPaths
  );
in
{
  options.programs.konveyor = {
    enable = lib.mkEnableOption "Konveyor widgets, services and panel integration";
  };

  config = lib.mkIf enable {
    # Ensure we're only enabled if the NixOS feature is on
    # (This is enforced by the NixOS module setting enableWidgets = true)
    assertions = [
      { assertion = osConfig.programs.konveyor.enableWidgets or false; message = "NixOS module programs.konveyor.enableWidgets must be true"; }
    ];

    home.packages = with pkgs; [
      konveyor
      jq # screen-rotate
      glib # gdbus, used by the portal helper scripts
      kdePackages.libkscreen # kscreen-doctor
      openssh # router monitor
      curl # router speed test
      iputils # ping
    ];

    # KWin loads the versioned effect names; also disable the unversioned
    # upstream names so an old install cannot load a stale ABI-mismatched .so.
    programs.plasma.configFile."kwinrc".Plugins = {
      "konveyor_effectEnabled" = false;
      "${pluginId}Enabled" = true;
      "process_monitor_telemetryEnabled" = false;
      "${telemetryPluginId}Enabled" = true;
    };

    # Upstream frees Meta/Alt+F1 from Plasma's launcher and Meta+G from KWin's
    # Grid View before taking them for the Kontrol Panel. Do the same
    # declaratively. Launch Konsole keeps Meta+K; Konveyor's cheatsheet moved
    # to Meta+Shift+P in config.kdl.
    #
    # Written through configFile rather than `programs.plasma.shortcuts` so
    # these keys can be marked immutable ([$i]): KGlobalAccel rewrites
    # kglobalshortcutsrc from its in-memory action registry on logout/relogin
    # and otherwise restores the old bindings, leaving colliding shortcuts
    # and a dead Kontrol Panel toggle after every restart.
    programs.plasma.configFile."kglobalshortcutsrc" = {
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

    xdg.configFile."konveyor/config.kdl".source = configKdl;

    # Home Manager's user units use the systemd section layout (Unit/Service/
    # Install), not NixOS' description/serviceConfig shorthand. Widgets read
    # their collectors' snapshots via QML XHR; QML_XHR_ALLOW_FILE_READ is set
    # globally by the NixOS feature and repeated here so the collectors also
    # work in a Plasma session started outside PAM.
    systemd.user.services.konveyor-widgets = {
      Unit = {
        Description = "Konveyor widget collectors";
        After = [ "graphical-session.target" ];
      };
      Service = {
        Type = "simple";
        ExecStart = "${widgets}/service/konveyor-widgets";
        Restart = "always";
        RestartSec = 3;
        Nice = 19;
        Environment = [ "QML_XHR_ALLOW_FILE_READ=1" ];
      };
      Install = {
        WantedBy = [ "default.target" ];
      };
    };

    systemd.user.services.konveyor-kontrol-panel = {
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

    home.file = plasmoidFiles // commandFiles // {
      # Patched Plasma desktop containment (desktop-widget hider).
      ".local/share/plasma/plasmoids/org.kde.desktopcontainment".source =
        "${konveyor}/share/konveyor/desktop-containment/org.kde.desktopcontainment";
    };

    home.activation.konveyorWidgetConfigs = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
      run ${pkgs.coreutils}/bin/mkdir -p \
        "$HOME/.config/Linux-System-Monitor" \
        "$HOME/.config/Linux-Process-Mon" \
        "$HOME/.config/Linux-Router-Monitor" \
        "$HOME/.config/Plasma-App-Portal" \
        "$HOME/.config/konveyor"

      if [ ! -e "$HOME/.config/Linux-System-Monitor/config.json" ]; then
        run ${pkgs.coreutils}/bin/install -m 644 \
          ${pkgs.writeText "konveyor-sysmon-config.json" (builtins.readFile "${widgets}/system-monitor/config.example.json")} \
          "$HOME/.config/Linux-System-Monitor/config.json"
      fi
      if [ ! -e "$HOME/.config/Linux-Process-Mon/config.json" ]; then
        run ${pkgs.coreutils}/bin/install -m 644 \
          ${pkgs.writeText "konveyor-procmon-config.json" (builtins.readFile "${widgets}/process-monitor/config.example.json")} \
          "$HOME/.config/Linux-Process-Mon/config.json"
      fi
      if [ ! -e "$HOME/.config/Linux-Router-Monitor/config.json" ]; then
        run ${pkgs.coreutils}/bin/install -m 600 ${routerSeed} \
          "$HOME/.config/Linux-Router-Monitor/config.json"
      fi
      if [ ! -e "$HOME/.config/Plasma-App-Portal/config.json" ]; then
        run ${pkgs.coreutils}/bin/install -m 600 ${portalSeed} \
          "$HOME/.config/Plasma-App-Portal/config.json"
      fi
      if [ ! -e "$HOME/.config/konveyor/kontrolpanelrc" ]; then
        run ${pkgs.coreutils}/bin/install -m 644 ${kontrolPanelSeed} \
          "$HOME/.config/konveyor/kontrolpanelrc"
      fi
      if [ ! -e "$HOME/.config/konveyor/taskbarrc" ]; then
        run ${pkgs.coreutils}/bin/install -m 644 ${taskbarSeed} \
          "$HOME/.config/konveyor/taskbarrc"
      fi
    '';

    # Make the new .desktop entries (especially Meta+G) known to KDE without
    # needing the upstream installer.
    home.activation.konveyorKSycoca = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
      run ${pkgs.kdePackages.kservice}/bin/kbuildsycoca6
    '';
  };
}