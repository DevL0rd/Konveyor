{ lib, stdenv, python3, cmake, ninja, patch, pkg-config, qt6, pipewire, libxkbcommon, xcbutilwm, libxcb, libdrm, libepoxy, kdePackages, src, version, plasmaShared, pluginId, telemetryPluginId }:

let
  pname = "konveyor";
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
    kdeclarative
    ksvg
    plasma-workspace
    kiconthemes
    qqc2-desktop-style
    layer-shell-qt
    plasma-desktop
    ;
in
stdenv.mkDerivation {
  inherit pname version src;

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
  ];

  # Build inputs
  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
    qt6.qt5compat
    pipewire
    libxkbcommon
    xcbutilwm
    libxcb
    libdrm
    libepoxy
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
    kdeclarative
    ksvg
    plasma-workspace
    plasma-desktop
    kiconthemes
    qqc2-desktop-style
    layer-shell-qt
  ];

  cmakeFlags = [
    "-DKONVEYOR_BUILD_TESTS=OFF"
    "-DKDE_INSTALL_USE_QT_SYS_PATHS=ON"
  ];

  postInstall = ''
    WIDGETS_DIR="$out/share/konveyor/widgets"
    mkdir -p "$WIDGETS_DIR"
    cp -RL "${src}/widgets/." "$WIDGETS_DIR/"
    chmod -R u+w "$WIDGETS_DIR"

    mkdir -p "$WIDGETS_DIR/shared/common"
    cp -RL "${plasmaShared}/." "$WIDGETS_DIR/shared/common/"
    find "$WIDGETS_DIR" -name .git -prune -exec rm -rf {} +

    # Stage with the installer's own functions so the layout cannot drift from
    # widgets/lib.sh; only the kpackagetool6 registration is replaced, because
    # Home Manager links the staged plasmoids.
    say() { :; }
    source "$WIDGETS_DIR/lib.sh"
    install_plasmoid() { :; }
    install_plasmoids
    stage_kontrol_panel

    DESKTOP_HIDER="$out/share/konveyor/desktop-containment/org.kde.desktopcontainment"
    mkdir -p "$(dirname "$DESKTOP_HIDER")"
    cp -RL "${plasma-desktop}/share/plasma/plasmoids/org.kde.desktopcontainment" "$DESKTOP_HIDER"
    chmod -R u+w "$DESKTOP_HIDER"
    ${patch}/bin/patch -d "$DESKTOP_HIDER" -p1 < "${src}/widgets/desktop-hider/patches/desktopcontainment.patch"
    touch "$DESKTOP_HIDER/.konveyor-managed"

    PLUGINDIR="$out/lib/qt-6/plugins/kwin/effects/plugins"
    mv "$PLUGINDIR/konveyor_effect.so" "$PLUGINDIR/${pluginId}.so"
    mv "$PLUGINDIR/process_monitor_telemetry.so" "$PLUGINDIR/${telemetryPluginId}.so"

    rm -f "$WIDGETS_DIR/install.sh" "$WIDGETS_DIR/uninstall.sh" "$WIDGETS_DIR/lib.sh"

    mkdir -p "$out/share/konveyor"
    printf '%s\n' ${pluginId} > "$out/share/konveyor/plugin-id"
    printf '%s\n' ${telemetryPluginId} > "$out/share/konveyor/telemetry-plugin-id"
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
    homepage = "https://github.com/DevL0rd/Konveyor";
    license = licenses.gpl3Plus;
    platforms = platforms.linux;
    mainProgram = "konveyor";
  };
}