<a id="top"></a>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/media/banner-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="docs/media/banner-light.svg">
    <img alt="Konveyor — scrolling tiling for KDE Plasma" src="docs/media/banner-dark.svg" width="100%">
  </picture>
</p>

<p align="center">
  <a href="https://github.com/DevL0rd/Konveyor/actions/workflows/ci.yml"><img alt="CI" src="https://img.shields.io/github/actions/workflow/status/DevL0rd/Konveyor/ci.yml?branch=main&style=for-the-badge&label=build&logo=githubactions&logoColor=white"></a>
  <img alt="KDE Plasma 6" src="https://img.shields.io/badge/KDE_Plasma-6-1d99f3?style=for-the-badge&logo=kde&logoColor=white">
  <img alt="Wayland" src="https://img.shields.io/badge/Wayland-native-ffbc00?style=for-the-badge&logo=wayland&logoColor=black">
  <a href="LICENSE"><img alt="GPL-3.0" src="https://img.shields.io/badge/license-GPL--3.0-8a5cd6?style=for-the-badge"></a>
  <a href="https://github.com/DevL0rd/Konveyor/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/DevL0rd/Konveyor?style=for-the-badge&logo=github&color=3daee9"></a>
</p>

<h3 align="center">Your windows, on a conveyor belt.</h3>

<p align="center">
  Konveyor turns Plasma into an endless, scrollable row of windows.<br>
  Nothing overlaps, nothing gets buried, and the rest of your desktop stays exactly the way you like it.
</p>

<p align="center">
  <a href="#get-started"><b>Get started</b></a> ·
  <a href="#see-it-move"><b>See it move</b></a> ·
  <a href="#widgets"><b>Konveyor widgets</b></a> ·
  <a href="#settings"><b>Settings</b></a> ·
  <a href="#shortcuts"><b>Shortcuts</b></a> ·
  <a href="#faq"><b>FAQ</b></a> ·
  <a href="#more"><b>More projects</b></a>
</p>

<p align="center">
  <img alt="Scrolling through a row of windows with Konveyor" src="docs/media/scroll.gif" width="92%">
</p>

---

<a id="get-started"></a>

## 🚀 Get started

```sh
git clone https://github.com/DevL0rd/Konveyor.git
cd Konveyor
./install.sh
```

That's it. The installer grabs what it needs, builds Konveyor for your exact KWin and switches it on, usually without even logging out. It also installs the [Konveyor widgets](#widgets), swaps your application launcher for the Kontrol Panel and opens it on **Shortcuts** so you can see everything Konveyor does.

> [!TIP]
> Press <kbd>Meta</kbd> + <kbd>K</kbd> any time to open the Kontrol Panel on **Shortcuts**: every shortcut, searchable, with a live diagram of what each Konveyor action does. Its gear, or <kbd>Ctrl</kbd> + <kbd>,</kbd>, jumps straight to Konveyor's settings.

<table>
  <tr>
    <td>🔄 <b>Update</b></td>
    <td>Konveyor updates itself with every system update and rebuilds when KWin, Qt or Plasma update. On Fedora Atomic desktops and SteamOS that happens at your next login after an update. You can also run <code>./install.sh</code> again any time. It's safe to repeat and keeps your settings.</td>
  </tr>
  <tr>
    <td>📦 <b>From a package</b></td>
    <td>Package builds run <code>./install.sh --aur</code>, so your package manager handles updates instead.</td>
  </tr>
  <tr>
    <td>🧹 <b>Remove</b></td>
    <td>Run <code>./uninstall.sh</code>. Your shortcuts and window rules go back to how KDE had them.</td>
  </tr>
  <tr>
    <td>🧩 <b>Just the tiling</b></td>
    <td>Run <code>./install.sh --no-widgets</code> to skip the widgets, or <code>./uninstall.sh --keep-widgets</code> to remove only the window manager.</td>
  </tr>
  <tr>
    <td>🧊 <b>Atomic desktops</b></td>
    <td>On Fedora Atomic desktops like Kinoite, Aurora and Bazzite, the installer builds Konveyor in a toolbox that matches your system's KWin exactly. On SteamOS it uses a distrobox that matches Valve's packages. Either way it installs to <code>~/.local</code>, so the read-only system stays untouched. Log out and back in once after the first install. After a system update, Konveyor rebuilds itself at your next login.</td>
  </tr>
  <tr>
    <td>🖥️ <b>Needs</b></td>
    <td>KDE Plasma 6.7 or newer on Wayland.</td>
  </tr>
  <tr>
    <td>🐧 <b>Distros</b></td>
    <td>The installer sets everything up on Arch and Arch-based systems like CachyOS, Fedora, openSUSE Tumbleweed, Debian testing and Fedora Atomic desktops like Kinoite, Aurora and Bazzite, plus SteamOS 3.9 and newer in Desktop Mode (on the Preview update channel for now). Kubuntu 26.04, Debian 13 and SteamOS 3.8 still ship an older Plasma.</td>
  </tr>
</table>

---

<a id="see-it-move"></a>

## 🎬 See it move

### ↔️ One endless row

Every new window gets its own column right next to the one you're using. When the row runs past the edge of your screen it simply scrolls, so a laptop shows two windows and an ultrawide shows five — from the same row. Tap a key to cycle a column through a third, half and two thirds of the screen, or let it fill whatever space is free.

<p align="center"><img alt="An ultrawide monitor showing four columns side by side" src="docs/screenshot.png" width="88%"></p>

### 🖥️ Built for multiple screens

Konveyor supports multi-monitor setups natively. Each display keeps its own scrolling row and workspace stack, windows move freely between screens, and per-monitor profiles let a laptop, ultrawide and portrait display each use the layout that fits it best.

### 🗂️ Stack them. Tab them.

Pull a window into the column next to it to stack them, then flip the column into tabs when you want one at a time. Kick it back out whenever you like.

<p align="center"><img alt="Stacking windows in a column and switching to tabs" src="docs/media/stack-and-tabs.gif" width="88%"></p>

### 🎮 Fullscreen that doesn't trap you

Games and videos stay fullscreen and untouched. Move left or right and your other columns slide in over the top as an overlay, with a soft shade at the edge — move back and they slide away until it's just your fullscreen app again.

<p align="center"><img alt="Columns sliding over a fullscreen app" src="docs/media/fullscreen-overlay.gif" width="88%"></p>

### 🧱 Workspaces that stack up

Workspaces live above and below each other and appear the moment you need one. Send a column down to a fresh workspace and carry on.

<p align="center"><img alt="Moving between stacked workspaces" src="docs/media/workspaces.gif" width="88%"></p>

### 🪟 Float anything

Some windows don't belong in a row. Lift one out with a single key, move it and resize it freely like any normal Plasma window, then drop it back in. Dialogs and pop-ups float on their own.

<p align="center"><img alt="Floating a window out of the row and back" src="docs/media/floating.gif" width="88%"></p>

### 📱 Portrait screens stack up

<table>
  <tr>
    <td width="36%" valign="top"><img alt="Windows stacking two to a column on a portrait monitor" src="docs/media/portrait.gif"></td>
    <td valign="top">
      <br>
      Turn a monitor on its side and Konveyor turns with it. Every column fills the full width and holds two windows, one above the other, so a tall screen shows two apps at a time without squeezing either of them.
      <br><br>
      New windows fill the focused column first, then start the next one. Rotate the screen back and the row returns to your normal widths.
      <br><br>
      It's just a monitor profile, so you can pick how many windows stack, or give any other screen the same treatment, under <b>Settings → Monitors</b>.
    </td>
  </tr>
</table>

<a id="touch"></a>

### 👆 Made for touch

Touchpads and touchscreens get the whole row under your fingers. Slide three fingers to scroll the row or change workspace, use four to carry windows around, and tap to resize.

<p align="center"><img alt="Touch gestures scrolling the row, switching workspaces, merging windows and cycling widths" src="docs/media/touch.gif" width="88%"></p>

| Gesture | What happens |
| :-- | :-- |
| 3 fingers ← → | Scroll the row |
| 3 fingers ↑ ↓ | Switch to the workspace above or below |
| 4 fingers ← → | Merge the window into the next column, or pop it back out |
| 4 fingers ↑ ↓ | Move the window up or down its column, then to the next workspace |
| 4-finger pinch | Open or close KDE's Overview |
| 3-finger tap | Cycle the column width |
| 4-finger tap | Open the Kontrol Panel |
| Drag a title bar on a touchscreen | Scroll the row |
| Hold a title bar on a touchscreen, then drag | Move the window |

The swipes, pinch and taps work on touchpads and touchscreens alike. Pick other finger counts, flip natural swiping or hand a gesture back to KDE on the **Touch & Gestures** settings page.

### ✨ And the little things

<table>
  <tr>
    <td width="33%" valign="top">
      <h4>🎨 Follows your accent</h4>
      The focus ring uses your Plasma accent color, or any color and gradient you pick.
    </td>
    <td width="33%" valign="top">
      <h4>🌊 Buttery motion</h4>
      Scrolling, opening, moving and resizing all glide on springs and curves drawn by the compositor.
    </td>
    <td width="33%" valign="top">
      <h4>🔲 Rounded, clipped corners</h4>
      Every window gets clean rounded corners, even apps that don't draw their own.
    </td>
  </tr>
  <tr>
    <td valign="top">
      <h4>📐 Per-monitor profiles</h4>
      Give your ultrawide narrower columns than your laptop, automatically. Portrait screens get full-width columns that stack two windows.
    </td>
    <td valign="top">
      <h4>🧩 Window rules</h4>
      Pin Discord to the start of the row, float picture-in-picture, open your browser at half width.
    </td>
    <td valign="top">
      <h4>🏠 Still your Plasma</h4>
      Panels, widgets, KRunner, Overview, notifications, screenshots and Alt+Tab all keep working.
    </td>
  </tr>
  <tr>
    <td valign="top">
      <h4>🧲 Apps stay together</h4>
      A second window from an app opens beside it or stacks under it, and new windows can fill the focused column before starting another.
    </td>
    <td valign="top">
      <h4>💬 Extra windows float</h4>
      An app's first window tiles and its friends lists, chats and settings float above the row. Steam does this out of the box.
    </td>
    <td valign="top">
      <h4>💾 Remembers your windows</h4>
      Reopen apps at the width they had last time and floating windows where you left them. Minimized windows come back to the same spot.
    </td>
  </tr>
</table>

<p align="right"><a href="#top">back to top ⬆</a></p>

---

<a id="widgets"></a>

## 🧩 Konveyor widgets

Konveyor comes with a set of Plasma widgets built to match: a full-screen launcher, system and process monitors, a router dashboard, a live system log and your Steam friends. Each one sits in your panel as a small, steady button and opens into a searchable dashboard. They also work as desktop widgets.

<p align="center"><img alt="The Kontrol Panel opening, searching, browsing games, shortcuts and settings" src="docs/media/widgets/launcher.gif" width="92%"></p>

### 🚀 Kontrol Panel

Press <kbd>Meta</kbd> and the desktop dims behind the Kontrol Panel, one place for your apps, games, friends, files, shortcuts and every Konveyor setting. Start typing and apps, games, friends, settings, shortcuts, files, maths, unit conversions, commands and even installable packages show up together, best match first.

<table>
  <tr>
    <td width="50%"><img alt="Kontrol Panel home page" src="docs/media/widgets/launcher-home.jpg"><p align="center"><b>Home</b> — pins and pinned folders, friends playing now and games to jump back into</p></td>
    <td width="50%"><img alt="Kontrol Panel search finding an app, settings, shortcuts and a sum" src="docs/media/widgets/launcher-search.gif"><p align="center"><b>Search</b> — best match first, then settings, shortcuts, answers and packages to install with Shelly</p></td>
  </tr>
  <tr>
    <td><img alt="Kontrol Panel games in cover flow" src="docs/media/widgets/launcher-games.jpg"><p align="center"><b>Games</b> — grid, banners, list, carousel and cover flow</p></td>
    <td><img alt="Kontrol Panel apps page" src="docs/media/widgets/launcher-apps.jpg"><p align="center"><b>Apps</b> — categories, an A–Z bar, sorting and zoom</p></td>
  </tr>
  <tr>
    <td><img alt="Kontrol Panel friends page" src="docs/media/widgets/launcher-friends.jpg"><p align="center"><b>Friends</b> — who's online and what they're playing</p></td>
    <td><img alt="Kontrol Panel system page" src="docs/media/widgets/launcher-system.jpg"><p align="center"><b>System</b> — lock, sleep, restart and your settings</p></td>
  </tr>
  <tr>
    <td><img alt="Kontrol Panel shortcuts page with animated previews" src="docs/media/widgets/launcher-shortcuts.gif"><p align="center"><b>Shortcuts</b> — every key and gesture by category, with a live diagram of what each Konveyor action does</p></td>
    <td><img alt="Kontrol Panel settings on the Touch and Gestures page" src="docs/media/widgets/launcher-settings.jpg"><p align="center"><b>Settings</b> — all of Konveyor's settings, applied instantly with Undo</p></td>
  </tr>
</table>

#### 📌 Pin anything to the sidebar

<table>
  <tr>
    <td width="60%"><img alt="Pinning a game and a folder to the Kontrol Panel sidebar" src="docs/media/widgets/launcher-pins.gif"></td>
    <td valign="top">
      <br>
      Keep your favourite apps, games, files and folders one click away under the pages on the left. Right-click anything and choose <b>Pin to sidebar</b>, press <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>P</kbd>, or drag it onto the sidebar. Drag pins to reorder them.
      <br><br>
      On a touchscreen, press and hold anything in the Kontrol Panel for the same menu.
    </td>
  </tr>
</table>

<table>
  <tr>
    <td><kbd>Meta</kbd></td>
    <td>Open the Kontrol Panel</td>
    <td><kbd>Meta</kbd> + <kbd>G</kbd></td>
    <td>Open it on your games</td>
  </tr>
  <tr>
    <td><kbd>Meta</kbd> + <kbd>K</kbd></td>
    <td>Open it on Shortcuts</td>
    <td><kbd>Ctrl</kbd> + <kbd>,</kbd></td>
    <td>Jump to Settings</td>
  </tr>
  <tr>
    <td><kbd>Alt</kbd> + <kbd>1</kbd> … <kbd>8</kbd></td>
    <td>Go to a page</td>
    <td><kbd>Ctrl</kbd> + <kbd>P</kbd></td>
    <td>Pin to Home</td>
  </tr>
</table>

### 📊 System Monitor and Process Monitor

<table>
  <tr>
    <td width="42%" valign="top"><img alt="System Monitor panel button and dashboard" src="docs/media/widgets/system-monitor.gif"><p align="center"><b>System Monitor</b> — CPU, every core, GPU, memory and temperatures with history</p></td>
    <td width="58%" valign="top"><img alt="Process Monitor panel button and dashboard" src="docs/media/widgets/process-monitor.gif"><p align="center"><b>Process Monitor</b> — the focused app with its FPS, plus every process as a tree</p></td>
  </tr>
</table>

### 📡 Router Monitor

Speeds, WiFi radios, every device, AdGuard Home and a speed test for ASUS routers running Asuswrt-Merlin, with rename and block right from the device list. Each tab is also its own desktop widget.

<table>
  <tr>
    <td width="40%" valign="top" rowspan="3"><img alt="Router Monitor going through its tabs" src="docs/media/widgets/router-monitor.gif"></td>
    <td width="30%"><img alt="Router overview" src="docs/media/widgets/router-overview.jpg"><p align="center"><b>Overview</b></p></td>
    <td width="30%"><img alt="Router network" src="docs/media/widgets/router-network.jpg"><p align="center"><b>Network</b></p></td>
  </tr>
  <tr>
    <td><img alt="Router WiFi" src="docs/media/widgets/router-wifi.jpg"><p align="center"><b>WiFi</b></p></td>
    <td><img alt="Router clients" src="docs/media/widgets/router-clients.jpg"><p align="center"><b>Clients</b></p></td>
  </tr>
  <tr>
    <td><img alt="Router DNS" src="docs/media/widgets/router-dns.jpg"><p align="center"><b>DNS</b></p></td>
    <td><img alt="Router system" src="docs/media/widgets/router-system.jpg"><p align="center"><b>System</b></p></td>
  </tr>
</table>

### 📜 System Log, 🎮 App Portal and 👥 Steam Friends

<table>
  <tr>
    <td width="50%" valign="top"><img alt="System Log with severity tabs and search" src="docs/media/widgets/system-log.gif"><p align="center"><b>System Log</b> — the live journal, by severity, searchable across everything</p></td>
    <td width="50%" valign="top"><img alt="App Portal popup" src="docs/media/widgets/app-portal.gif"><p align="center"><b>App Portal</b> — the Kontrol Panel in a compact panel popup</p></td>
  </tr>
  <tr>
    <td valign="top"><img alt="Steam Friends popup" src="docs/media/widgets/steam-friends.gif"><p align="center"><b>Steam Friends</b> — online, in game, join and chat</p></td>
    <td valign="top" align="center"><br><img alt="Screen Rotate button" src="docs/media/widgets/screen-rotate.png"><p align="center"><b>Screen Rotate</b> — one tap gives the screen a quarter turn</p></td>
  </tr>
</table>

### ⚙️ Setting them up

Everything the widgets need is installed for you, and one background service feeds all of them. A few need a detail only you have:

<table>
  <tr>
    <td>📡 <b>Router Monitor</b></td>
    <td>Enter your router's address in the Router Monitor to connect it, or add the address, user and your AdGuard Home URL and login to <code>~/.config/Linux-Router-Monitor/config.json</code>, then run <code>./install.sh</code> again.</td>
  </tr>
  <tr>
    <td>👥 <b>Steam friends</b></td>
    <td>Paste a free <a href="https://steamcommunity.com/dev/apikey">Steam Web API key</a> into the Steam Friends widget, or into <code>~/.config/Plasma-App-Portal/config.json</code>.</td>
  </tr>
  <tr>
    <td>🎮 <b>FPS</b></td>
    <td>Process Monitor reads window update rates through its own independent KWin telemetry plugin, with no overlay or log files.</td>
  </tr>
  <tr>
    <td>🖼️ <b>Desktop widgets</b></td>
    <td>Konveyor hides desktop widgets while windows cover them and shows them again when you look at the desktop.</td>
  </tr>
</table>

<p align="right"><a href="#top">back to top ⬆</a></p>

---

<a id="settings"></a>

## 🎛️ Settings that show you

Every Konveyor setting lives in the Kontrol Panel. Press <kbd>Meta</kbd> + <kbd>K</kbd> and open **Settings** at the bottom of the sidebar, hit the gear, or press <kbd>Ctrl</kbd> + <kbd>,</kbd>. The same pages are in **System Settings → Window Management → Konveyor** and behind **Configure** next to Konveyor under Desktop Effects, so they're there without the widgets too. Every option has a live preview, changes apply instantly, and Undo takes them back.

<p align="center"><img alt="A tour through the Konveyor settings pages" src="docs/media/settings-tour.gif" width="88%"></p>

<table>
  <tr>
    <td width="50%"><img alt="Layout settings" src="docs/media/settings-layout.png"><p align="center"><b>Layout</b> — gaps, column widths, where new windows land and which apps stay together</p></td>
    <td width="50%"><img alt="Look settings" src="docs/media/settings-look.png"><p align="center"><b>Look</b> — focus ring, borders, tabs and corners</p></td>
  </tr>
  <tr>
    <td><img alt="Motion settings" src="docs/media/settings-motion.png"><p align="center"><b>Motion</b> — shape every animation and watch it play</p></td>
    <td><img alt="Shortcut settings" src="docs/media/settings-shortcuts.png"><p align="center"><b>Shortcuts</b> — record keys, pick actions, spot clashes</p></td>
  </tr>
  <tr>
    <td><img alt="Window rule settings" src="docs/media/settings-rules.png"><p align="center"><b>Window rules</b> — pick an app, see what matches, decide how it behaves</p></td>
    <td><img alt="Searching Konveyor settings and shortcuts from the Kontrol Panel" src="docs/media/settings-hub.png"><p align="center"><b>Search everything</b> — type in the Kontrol Panel and jump straight to any setting or shortcut</p></td>
  </tr>
</table>

<table>
  <tr>
    <td width="50%"><img alt="Touch and gesture settings" src="docs/media/settings-touch.png"><p align="center"><b>Touch &amp; Gestures</b> — finger counts, swipes, pinches, taps and long presses</p></td>
    <td width="50%"><img alt="Portrait monitor profile settings" src="docs/media/settings-portrait.png"><p align="center"><b>Monitor profiles</b> — portrait screens get full-width columns that stack two windows</p></td>
  </tr>
  <tr>
    <td><img alt="New window settings: grouping, stacked windows per column, floating extra windows and starting width" src="docs/media/settings-new-windows.png"><p align="center"><b>New windows</b> — keep apps together, cap stacked windows, float an app's extra windows, pick a starting width</p></td>
    <td><img alt="Scrolling, centering and remembered window size settings" src="docs/media/settings-remember.png"><p align="center"><b>Remember windows</b> — reopen apps at their last width and floating windows where you left them</p></td>
  </tr>
</table>

<details>
<summary><b>📸 More settings pages</b></summary>
<br>
<table>
  <tr>
    <td width="50%"><img alt="Mouse settings" src="docs/media/settings-mouse.png"><p align="center"><b>Mouse</b> — focus follows mouse, edge scrolling, title bar drags</p></td>
    <td width="50%"><img alt="Monitor settings" src="docs/media/settings-monitors.png"><p align="center"><b>Monitors</b> — per-screen profiles drawn to scale</p></td>
  </tr>
  <tr>
    <td><img alt="Workspace settings" src="docs/media/settings-workspaces.png"><p align="center"><b>Workspaces</b> — named workspaces and switching</p></td>
    <td><img alt="Plasma integration settings" src="docs/media/settings-plasma.png"><p align="center"><b>Plasma integration</b> — widgets, panels and minimizing</p></td>
  </tr>
</table>
</details>

---

<a id="shortcuts"></a>

## ⌨️ The keys you need

| Keys | What happens |
| :-- | :-- |
| <kbd>Meta</kbd> + <kbd>←</kbd> <kbd>→</kbd> | Move to the column on the left or right |
| <kbd>Meta</kbd> + <kbd>↑</kbd> <kbd>↓</kbd> | Move within a column, then to the workspace above or below |
| <kbd>Meta</kbd> + <kbd>Ctrl</kbd> + <kbd>←</kbd> <kbd>→</kbd> | Carry the column left or right |
| <kbd>Meta</kbd> + <kbd>Ctrl</kbd> + <kbd>↑</kbd> <kbd>↓</kbd> | Carry the window up or down, into the next workspace |
| <kbd>Meta</kbd> + <kbd>R</kbd> | Cycle the column width |
| <kbd>Meta</kbd> + <kbd>F</kbd> | Cycle full width, fill the screen, back to normal |
| <kbd>Meta</kbd> + <kbd>Shift</kbd> + <kbd>F</kbd> | Fullscreen |
| <kbd>Meta</kbd> + <kbd>[</kbd> <kbd>]</kbd> | Stack into the neighbouring column, or pop back out |
| <kbd>Meta</kbd> + <kbd>Shift</kbd> + <kbd>W</kbd> | Show a column as tabs |
| <kbd>Meta</kbd> + <kbd>Space</kbd> | Float or unfloat |
| <kbd>Meta</kbd> + <kbd>1</kbd> … <kbd>9</kbd> | Jump to a workspace |
| <kbd>Meta</kbd> + <kbd>Return</kbd> | Open a terminal |
| <kbd>Meta</kbd> + <kbd>Q</kbd> | Close the window |
| <kbd>Meta</kbd> + <kbd>K</kbd> | Open the Kontrol Panel on Shortcuts |
| <kbd>Meta</kbd> | Open the Kontrol Panel |
| <kbd>Meta</kbd> + <kbd>G</kbd> | Open the Kontrol Panel on your games |

On a touchpad or touchscreen, see [Made for touch](#touch) for every gesture.

> [!NOTE]
> Konveyor leaves KDE's own shortcuts alone — <kbd>Alt</kbd> + <kbd>F4</kbd>, <kbd>Meta</kbd> + <kbd>L</kbd>, screenshots and the rest work as always. Change anything from the **Shortcuts** settings page. The 3- and 4-finger swipes replace KDE's desktop-switching and Overview swipes; pick other finger counts, or turn gestures off, on the **Touch & Gestures** settings page.

---

<a id="faq"></a>

## 💬 Questions

<details>
<summary><b>Does it replace Plasma or KWin?</b></summary>
<br>
No. Konveyor is a KWin effect that decides where windows go. Everything else — panels, widgets, themes, the lock screen, KRunner — is still plain Plasma.
</details>

<details>
<summary><b>Do games and fullscreen video work?</b></summary>
<br>
Yes. Fullscreen apps are never moved or resized, whether they run natively or through XWayland, and you can still reach your other windows on top of them.
</details>

<details>
<summary><b>Does it work on X11?</b></summary>
<br>
Konveyor runs in the Plasma Wayland session. X11 apps running through XWayland are tiled like everything else.
</details>

<details>
<summary><b>Can I tweak things beyond the settings page?</b></summary>
<br>
Everything the settings page changes lives in one readable file, and there's a command line tool for scripting. See <a href="docs/configuration.md">docs/configuration.md</a>.
</details>

<details>
<summary><b>Do the widgets slow my system down?</b></summary>
<br>
No. A single background service collects everything they show, and the dashboards only update while you can see them. Close a popup or cover a desktop widget and it goes quiet.
</details>

<details>
<summary><b>Can I keep my old application launcher?</b></summary>
<br>
Yes. Install with <code>./install.sh --no-widgets</code>, or put the launcher back from your panel's Add Widgets menu. Uninstalling the widgets restores the launcher you had.
</details>

<details>
<summary><b>How do I get my old desktop back?</b></summary>
<br>
Turn Konveyor off under <b>Desktop Effects</b>, or run <code>./uninstall.sh</code>. Your shortcuts and window rules are restored.
</details>

---

<a id="more"></a>

## 🧰 More from DevL0rd

Other Plasma projects made to sit on the same desktop. Click a banner to open it on GitHub.

<p align="center">
  <a href="https://github.com/DevL0rd/RVC-Voice-Changer">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="docs/media/more/rvc-voice-changer-dark.svg">
      <source media="(prefers-color-scheme: light)" srcset="docs/media/more/rvc-voice-changer-light.svg">
      <img alt="RVC Voice Changer — Real-time AI voice changing for Plasma" src="docs/media/more/rvc-voice-changer-dark.svg" width="600">
    </picture>
  </a>
  <br>
  <a href="https://github.com/DevL0rd/RVC-Voice-Changer"><b>RVC Voice Changer</b></a> · Sound like anyone, in every app.
</p>

<p align="center">
  <a href="https://github.com/DevL0rd/KBoard">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="docs/media/more/kboard-dark.svg">
      <source media="(prefers-color-scheme: light)" srcset="docs/media/more/kboard-light.svg">
      <img alt="KBoard — The on-screen keyboard for Plasma" src="docs/media/more/kboard-dark.svg" width="600">
    </picture>
  </a>
  <br>
  <a href="https://github.com/DevL0rd/KBoard"><b>KBoard</b></a> · Type, glide and talk, right on your desktop.
</p>

<p align="center">
  <a href="https://github.com/DevL0rd/Android-Daemon">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="docs/media/more/android-daemon-dark.svg">
      <source media="(prefers-color-scheme: light)" srcset="docs/media/more/android-daemon-light.svg">
      <img alt="Android-Daemon — Your Android phone, part of your Plasma desktop" src="docs/media/more/android-daemon-dark.svg" width="600">
    </picture>
  </a>
  <br>
  <a href="https://github.com/DevL0rd/Android-Daemon"><b>Android-Daemon</b></a> · Your phone, right on your desktop.
</p>

<p align="center">
  <a href="https://github.com/DevL0rd/Syncthing-Monitor">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="docs/media/more/syncthing-monitor-dark.svg">
      <source media="(prefers-color-scheme: light)" srcset="docs/media/more/syncthing-monitor-light.svg">
      <img alt="Syncthing Monitor — Syncthing, live in your Plasma panel" src="docs/media/more/syncthing-monitor-dark.svg" width="600">
    </picture>
  </a>
  <br>
  <a href="https://github.com/DevL0rd/Syncthing-Monitor"><b>Syncthing Monitor</b></a> · Your sync, at a glance.
</p>

---

<p align="center">
  Inspired by <a href="https://github.com/niri-wm/niri">niri</a>. Konveyor is an independent project and is not affiliated with it or with KDE.<br>
  Released under the <a href="LICENSE">GPL-3.0-or-later</a>.
</p>

<p align="center"><a href="#top">back to top ⬆</a></p>
