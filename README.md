<a id="top"></a>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/media/banner-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="docs/media/banner-light.svg">
    <img alt="Konveyor — scrolling tiling for KDE Plasma" src="docs/media/banner-dark.svg" width="100%">
  </picture>
</p>

<p align="center">
  <a href="https://github.com/DevL0rd/Linux-Konveyor/actions/workflows/ci.yml"><img alt="CI" src="https://img.shields.io/github/actions/workflow/status/DevL0rd/Linux-Konveyor/ci.yml?branch=main&style=for-the-badge&label=build&logo=githubactions&logoColor=white"></a>
  <img alt="KDE Plasma 6" src="https://img.shields.io/badge/KDE_Plasma-6-1d99f3?style=for-the-badge&logo=kde&logoColor=white">
  <img alt="Wayland" src="https://img.shields.io/badge/Wayland-native-ffbc00?style=for-the-badge&logo=wayland&logoColor=black">
  <a href="LICENSE"><img alt="GPL-3.0" src="https://img.shields.io/badge/license-GPL--3.0-8a5cd6?style=for-the-badge"></a>
  <a href="https://github.com/DevL0rd/Linux-Konveyor/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/DevL0rd/Linux-Konveyor?style=for-the-badge&logo=github&color=3daee9"></a>
</p>

<h3 align="center">Your windows, on a conveyor belt.</h3>

<p align="center">
  Konveyor turns Plasma into an endless, scrollable row of windows.<br>
  Nothing overlaps, nothing gets buried, and the rest of your desktop stays exactly the way you like it.
</p>

<p align="center">
  <a href="#get-started"><b>Get started</b></a> ·
  <a href="#see-it-move"><b>See it move</b></a> ·
  <a href="#settings"><b>Settings</b></a> ·
  <a href="#shortcuts"><b>Shortcuts</b></a> ·
  <a href="#faq"><b>FAQ</b></a>
</p>

<p align="center">
  <img alt="Scrolling through a row of windows with Konveyor" src="docs/media/scroll.gif" width="92%">
</p>

---

<a id="get-started"></a>

## 🚀 Get started

```sh
git clone https://github.com/DevL0rd/Linux-Konveyor.git
cd Linux-Konveyor
./install.sh
```

That's it. The installer grabs what it needs, builds Konveyor for your exact KWin and switches it on, usually without even logging out.

> [!TIP]
> Press <kbd>Meta</kbd> + <kbd>K</kbd> any time for a searchable cheat sheet of every shortcut.

<table>
  <tr>
    <td>🔄 <b>Update</b></td>
    <td>Run <code>./install.sh</code> again. It's safe to repeat and keeps your settings.</td>
  </tr>
  <tr>
    <td>🧹 <b>Remove</b></td>
    <td>Run <code>./uninstall.sh</code>. Your shortcuts and window rules go back to how KDE had them.</td>
  </tr>
  <tr>
    <td>🖥️ <b>Needs</b></td>
    <td>KDE Plasma 6.4 or newer on Wayland. The installer sets up dependencies on Arch, Fedora, openSUSE and Debian-based systems.</td>
  </tr>
</table>

---

<a id="see-it-move"></a>

## 🎬 See it move

### ↔️ One endless row

Every new window gets its own column right next to the one you're using. When the row runs past the edge of your screen it simply scrolls, so a laptop shows two windows and an ultrawide shows five — from the same row. Tap a key to cycle a column through a third, half and two thirds of the screen, or let it fill whatever space is free.

<p align="center"><img alt="An ultrawide monitor showing four columns side by side" src="docs/screenshot.png" width="88%"></p>

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
      Give your ultrawide narrower columns than your laptop, automatically.
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
</table>

<p align="right"><a href="#top">back to top ⬆</a></p>

---

<a id="settings"></a>

## 🎛️ Settings that show you

Open **System Settings → Window Management → Konveyor**. Every option has a live preview, so you can see exactly what a change does before you hit Apply.

<p align="center"><img alt="A tour through the Konveyor settings pages" src="docs/media/settings-tour.gif" width="88%"></p>

<table>
  <tr>
    <td width="50%"><img alt="Layout settings" src="docs/media/settings-layout.png"><p align="center"><b>Layout</b> — gaps, column widths and where new windows land</p></td>
    <td width="50%"><img alt="Look settings" src="docs/media/settings-look.png"><p align="center"><b>Look</b> — focus ring, borders, tabs and corners</p></td>
  </tr>
  <tr>
    <td><img alt="Motion settings" src="docs/media/settings-motion.png"><p align="center"><b>Motion</b> — shape every animation and watch it play</p></td>
    <td><img alt="Shortcut settings" src="docs/media/settings-shortcuts.png"><p align="center"><b>Shortcuts</b> — record keys, pick actions, spot clashes</p></td>
  </tr>
  <tr>
    <td><img alt="Window rule settings" src="docs/media/settings-rules.png"><p align="center"><b>Window rules</b> — pick an app, see what matches, decide how it behaves</p></td>
    <td><img alt="Settings overview" src="docs/media/settings-hub.png"><p align="center"><b>Everything in one place</b> — search any setting in seconds</p></td>
  </tr>
</table>

<details>
<summary><b>📸 More settings pages</b></summary>
<br>
<table>
  <tr>
    <td width="50%"><img alt="Mouse and gesture settings" src="docs/media/settings-mouse.png"><p align="center"><b>Mouse &amp; gestures</b> — hot corners, edge scrolling, title bar drags</p></td>
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
| <kbd>Meta</kbd> + <kbd>K</kbd> | Show every shortcut |

> [!NOTE]
> Konveyor leaves KDE's own shortcuts alone — <kbd>Alt</kbd> + <kbd>F4</kbd>, <kbd>Meta</kbd> + <kbd>L</kbd>, screenshots and the rest work as always. Change anything from the **Shortcuts** settings page.

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
<summary><b>How do I get my old desktop back?</b></summary>
<br>
Turn Konveyor off under <b>Desktop Effects</b>, or run <code>./uninstall.sh</code>. Your shortcuts and window rules are restored.
</details>

---

<p align="center">
  Inspired by <a href="https://github.com/niri-wm/niri">niri</a>. Konveyor is an independent project and is not affiliated with it or with KDE.<br>
  Released under the <a href="LICENSE">GPL-3.0-or-later</a>.
</p>

<p align="center"><a href="#top">back to top ⬆</a></p>
