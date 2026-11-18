{ lib, stdenv, python3, cmake, ninja, patch, pkg-config, qt6, pipewire, libxkbcommon, xcbutilwm, xcbutil, libX11, libxcb, libXext, libdrm, libepoxy, wayland, vulkan-headers, kdePackages, git, src, plasmaShared, pluginId, telemetryPluginId }:

let
  pname = "konveyor";
  version = "0.1.0-unstable-20261118-1";

  # Use the source from the flake
  srcPath = src;

  # The widget collectors are plain Python scripts with `env python3` /
  # `/usr/bin/python3 -S` shebangs. Pin them to a store python that carries the
  # optional NVIDIA-ML and D-Bus bindings (used for GPU stats and frame
  # telemetry). patchShebangs maps the bare `python3` names to this interpreter.
  widgetPython = python3.withPackages (ps: [
    ps.nvidia-ml-py
    ps.dbus-python
  ]);

  inherit (kdePackages)
    extra-cmake-modules
    kwin
    kdecoration
    kglobalaccel
    kconfig
    kcolorscheme
    knotifications
    kcoreaddons
    ki18n
    kwindowsystem
    kcmutils
    kirigami
    kirigami-addons
    libplasma
    kservice
    kpackage
    kdeclarative
    kiconthemes
    qqc2-desktop-style
    layer-shell-qt
    plasma-desktop
    ;
in
stdenv.mkDerivation rec {
  inherit pname version;

  # Native build inputs
  nativeBuildInputs = [
    cmake
    ninja
    patch
    widgetPython
    extra-cmake-modules
    pkg-config
    qt6.wrapQtAppsHook
    qt6.qtshadertools
    qt6.qttools
    git
  ];

  # Build inputs
  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
    qt6.qt5compat
    pipewire
    libxkbcommon
    xcbutilwm
    xcbutil
    libX11
    libxcb
    libXext
    libdrm
    libepoxy
    wayland
    vulkan-headers
    kwin
    kdecoration
    kglobalaccel
    kconfig
    kcolorscheme
    knotifications
    kcoreaddons
    ki18n
    kwindowsystem
    kcmutils
    kirigami
    kirigami-addons
    libplasma
    kservice
    kpackage
    kdeclarative
    kiconthemes
    qqc2-desktop-style
    layer-shell-qt
  ];

  cmakeFlags = [
    "-DKONVEYOR_BUILD_TESTS=OFF"
    "-DKDE_INSTALL_USE_QT_SYS_PATHS=ON"
  ];

  # Skip default unpack phase - source is already a directory in the store
  unpackPhase = "true";

  # Pre-configure: copy source to build directory
  preConfigure = ''
    # Copy source to build directory
    cp -r ${srcPath}/. .
    # Use pre-computed plugin IDs
    echo "pluginId=${pluginId}" > $TMPDIR/plugin-ids.sh
    echo "telemetryPluginId=${telemetryPluginId}" >> $TMPDIR/plugin-ids.sh
  '';

  # Stage the widget tree exactly like widgets/install.sh's staging functions,
  # but inside the immutable derivation so nothing lives in a mutable home dir.
  postInstall = ''
    source $TMPDIR/plugin-ids.sh
    WIDGETS_DIR="$out/share/konveyor/widgets"
    mkdir -p "$WIDGETS_DIR"
    # NAR-optimised store files are hardlinked; `cp -a` would preserve those
    # hardlinks and break the lib-staging `cp`s below (same inode). `-RL` gives
    # each staged file its own inode and dereferences any symlinks.
    cp -RL "${srcPath}/widgets/." "$WIDGETS_DIR/"
    # Sources come from the read-only Nix store; make the staged copy writable
    # before copying Plasma-Shared into it or rearranging files below.
    chmod -R u+w "$WIDGETS_DIR"

    # Copy the Plasma-Shared contents, not the directory itself. The `/.` and
    # trailing slash are important: without them cp would create
    # `shared/common/<store-name>/` and the QML staging helpers would not find
    # any shared QML/JS files.
    mkdir -p "$WIDGETS_DIR/shared/common"
    cp -RL "${plasmaShared}/." "$WIDGETS_DIR/shared/common/"
    find "$WIDGETS_DIR" -name .git -prune -exec rm -rf {} +

    stage_lib_into() {
      local lib="$1"; shift
      rm -rf "$lib"
      mkdir -p "$lib"
      cp "$WIDGETS_DIR/shared/common/"*.qml "$WIDGETS_DIR/shared/common/"*.js "$lib/"
      cp "$WIDGETS_DIR/shared/"*.qml "$lib/"
      local extra
      for extra in "$@"; do cp -r "$extra/." "$lib/"; done
    }
    stage_lib() {
      local plasmoid="$1"; shift
      mkdir -p "$plasmoid/contents/ui/lib"
      stage_lib_into "$plasmoid/contents/ui/lib" "$@"
    }
    copy_panel_ui() {
      local source="$1" target="$2"; shift 2
      mkdir -p "$target/contents/ui" "$target/contents/config"
      cp "$source/contents/ui/"*.qml "$target/contents/ui/"
      local name
      for name in "$@"; do cp -r "$source/contents/ui/$name" "$target/contents/ui/"; done
      cp "$source/contents/config/main.xml" "$source/contents/config/config.qml" "$target/contents/config/"
    }

    for plasmoid in "$WIDGETS_DIR"/system-monitor/plasmoids/org.devl0rd.sysmon*; do
      [[ $plasmoid == */org.devl0rd.sysmon.panel ]] || copy_panel_ui "$WIDGETS_DIR/system-monitor/plasmoids/org.devl0rd.sysmon.panel" "$plasmoid"
      stage_lib "$plasmoid"
    done
    for plasmoid in "$WIDGETS_DIR"/process-monitor/plasmoids/org.devl0rd.procmon*; do
      [[ $plasmoid == */org.devl0rd.procmon.panel ]] || copy_panel_ui "$WIDGETS_DIR/process-monitor/plasmoids/org.devl0rd.procmon.panel" "$plasmoid" proc.worker.mjs
      stage_lib "$plasmoid"
    done
    for plasmoid in "$WIDGETS_DIR"/router-monitor/plasmoids/org.devl0rd.routermon.*; do
      if [[ $plasmoid != */org.devl0rd.routermon.panel ]]; then
        rm -rf "$plasmoid/contents/ui/tabs"
        copy_panel_ui "$WIDGETS_DIR/router-monitor/plasmoids/org.devl0rd.routermon.panel" "$plasmoid" tabs
      fi
      stage_lib "$plasmoid" "$WIDGETS_DIR/router-monitor/shared/lib"
    done
    for plasmoid in "$WIDGETS_DIR"/system-log/plasmoids/org.devl0rd.logmon.*; do
      stage_lib "$plasmoid" "$WIDGETS_DIR/system-log/shared/lib"
    done
    for plasmoid in "$WIDGETS_DIR"/portals/plasmoids/org.devl0rd.portal*; do
      case "$(basename "$plasmoid")" in
        org.devl0rd.portal)
          stage_lib "$plasmoid" "$WIDGETS_DIR/portals/shared/lib"
          rm -rf "$plasmoid/contents/ui/pages" "$plasmoid/contents/ui/search" "$plasmoid/contents/ui/shortcuts" "$plasmoid/contents/ui/settings"
          cp -r "$WIDGETS_DIR/portals/shared/launcher/." "$plasmoid/contents/ui/"
          ;;
        org.devl0rd.portal.launcher)
          find "$plasmoid/contents/ui" -mindepth 1 -maxdepth 1 ! -name main.qml ! -name configButton.qml -exec rm -rf {} +
          ;;
        *)
          stage_lib "$plasmoid" "$WIDGETS_DIR/portals/shared/lib"
          ;;
      esac
    done

    KP="$WIDGETS_DIR/portals/kontrol-panel"
    cp -r "$WIDGETS_DIR/portals/shared/launcher/." "$KP/"
    stage_lib_into "$KP/lib" "$WIDGETS_DIR/portals/shared/lib"

    # The desktop-widget hider replaces Plasma's desktop containment with a
    # patched copy. Build it against the exact plasma-desktop pinned here so it
    # cannot silently stop applying after a Plasma update.
    DESKTOP_HIDER="$out/share/konveyor/desktop-containment/org.kde.desktopcontainment"
    mkdir -p "$(dirname "$DESKTOP_HIDER")"
    cp -RL "${plasma-desktop}/share/plasma/plasmoids/org.kde.desktopcontainment" "$DESKTOP_HIDER"
    chmod -R u+w "$DESKTOP_HIDER"
    ${patch}/bin/patch -d "$DESKTOP_HIDER" -p1 < "${srcPath}/widgets/desktop-hider/patches/desktopcontainment.patch"
    touch "$DESKTOP_HIDER/.konveyor-managed"

    PLUGINDIR="$out/lib/qt-6/plugins/kwin/effects/plugins"
    mv "$PLUGINDIR/konveyor_effect.so" "$PLUGINDIR/$pluginId.so"
    mv "$PLUGINDIR/process_monitor_telemetry.so" "$PLUGINDIR/$telemetryPluginId.so"

    rm -f "$WIDGETS_DIR/install.sh" "$WIDGETS_DIR/uninstall.sh"

    mkdir -p "$out/share/konveyor"
    printf '%s\n' $pluginId > "$out/share/konveyor/plugin-id"
    printf '%s\n' $telemetryPluginId > "$out/share/konveyor/telemetry-plugin-id"
  '';

  postFixup = ''
    # patchShebangsAuto resolves store files against HOST_PATH under strictDeps,
    # so the Python in nativeBuildInputs is missed. Force a build-PATH lookup
    # so every collector resolves to the store python with NVIDIA/D-Bus bindings.
    patchShebangs --build "$out/share/konveyor/widgets"
  '';

  # These are passed in by the flake and are available at evaluation time.
  # Reading `$out/share/konveyor/plugin-id` here would be wrong: `$out` is a
  # shell variable, not a Nix path.
  passthru = {
    inherit pluginId telemetryPluginId;
  };

  meta = with lib; {
    description = "KWin tiling window manager with Plasma widgets";
    homepage = "https://github.com/sunnysamantara/Konveyor";
    license = licenses.gpl3Plus;
    platforms = platforms.linux;
    mainProgram = "konveyor";
  };
}