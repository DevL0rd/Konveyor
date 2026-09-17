# Linux-Plasma-Portals

A pair of native KDE Plasma 6 widgets: a category-filtered **app launcher** with a
rich, animated **games view** (Steam art, multiple layouts, friend presence), and a
**Steam friends list** with live presence, avatars and quick actions.

This is mostly for myself, but in case my friends use it, here are the instructions.

## Widgets

| Widget | Shows |
|--------|-------|
| **App Portal**   | The Kontrol Panel in a panel popup or on the desktop: the same Home, Apps, Games, Files, Friends and System pages and the same search, in a compact layout. Games keep grid, banners, list, carousel and cover flow with Steam art, friends glow, custom art and zoom. Pins, folders, hidden apps and launch history are shared with the launcher. Settings from the older App Portal (category, games view, card size, sort) carry over. Panel icon shows how many friends are in game. |
| **Kontrol Panel** | A start button that opens a full-screen launcher: the desktop dims and a centered card animates in. **Home** (pinned apps, recently used, continue playing with Steam art and friend glow, recent files), **Apps** (every category), **Games** (your library with art, played/friends filters), **Files** (places, recent documents and folders), **Friends** (in game / online / offline) and **System** (lock, sleep, restart, shut down with a second press to confirm, settings shortcuts). One search finds apps, games, friends, settings pages, files, calculations and commands; prefixes narrow it (`g ` games, `f ` files, `a ` apps, `@` friends, `=` math, `>` command). Full keyboard control, right-click actions (pin, app jump lists, add to panel/desktop, edit, uninstall). Pinned apps can be grouped into folders by dragging one onto another (open in place, rename, drag out, ungroup). Rail hints show each page's Alt+number shortcut. Meta opens it when it replaces the application launcher, and **Meta+G** opens it straight on Games (the installer frees Meta+G from KWin's Grid View). Uses no CPU while closed. |
| **Steam Friends** | Live friends list with avatars and presence (in-game / online / away / offline), grouped into **Favourites / In Game / Online / Offline** sections. Per-friend right-click: Open Chat, Join Game, Watch Game, View Profile, and add/remove **Favourite** (pinned to the top). Right side shows in-game art, a country flag, or "last online". Panel icon carries an online-count badge. |

Each widget keeps its own per-instance settings (view mode, sort, filters, zoom,
favourites) — so multiple copies on your panel/desktop don't interfere.

## How it works (and why it's light)

The widgets don't each do their own heavy lifting. Small helpers in `bin/` do the work
and the widgets read cached snapshots:

* **`portal-games`** resolves your Steam library + artwork, tracks "last opened",
  and stores favourites/custom art.
* **`portal-friends`** is a resident systemd **user service** that polls the Steam
  Web API and writes a JSON snapshot to `$XDG_RUNTIME_DIR`. Both widgets read that
  one snapshot, so whether you run one widget or both, Steam sees about **one poll
  per interval**. The service is niced and pinned to the CPU's efficiency cores
  (via `portal-ecores`), same as the router/log collectors.
* The Friends widget reads the snapshot **in-process** (QML XHR over `file://`, no
  subprocess per poll) and reconciles its list **in place** keyed by SteamID, so
  rows update without being destroyed and recreated.

## Requirements

* KDE Plasma 6 (Qt 6)
* `python3`, `kpackagetool6`
* A free **Steam Web API key** for the friends features (badges, glow, the Friends
  widget). The launcher and games art work without it.

## Installation

Clone **with submodules** — the shared QML components live in the
[Plasma-Shared](https://github.com/DevL0rd/Plasma-Shared) submodule:

```bash
git clone --recurse-submodules https://github.com/DevL0rd/Linux-Plasma-Portals.git
# already cloned without it?  git submodule update --init --recursive
./install.sh
```

Then add your Steam Web API key (get one free at
<https://steamcommunity.com/dev/apikey> — any domain works in the form):

```bash
~/.config/Plasma-App-Portal/config.json
```

```jsonc
{
  "steam_api_key": "PASTE_YOUR_STEAM_WEB_API_KEY_HERE",
  "steamid": "",          // auto-detected from the Steam client; set only to override
  "poll_interval": 10     // seconds between Steam Web API polls
}
```

You can also paste the key straight into the **Steam Friends** widget — it shows a
setup panel with an input box when it can't authenticate, and reloads the collector
for you.

Add the widgets via **right-click → Add Widgets → search "App Portal" / "Steam Friends"**.

> The config file holds your API key and is **git-ignored**.

## Uninstallation

```bash
./uninstall.sh
```

Removes the widgets, the `~/.local/bin` links and the friends service. Your config
(with the API key) and any custom game art in `~/.local/share/Plasma-App-Portal`
are kept.

## Layout

```
bin/portal-games        Steam library + art resolver, favourites, usage tracking
bin/portal-friends      Steam Web API presence collector (--serve resident service)
bin/portal-ecores       detects efficiency cores to pin the collector to
plasmoids/              the two Plasma 6 applets (App Portal + Steam Friends)
config.example.json     template copied to ~/.config/Plasma-App-Portal/config.json
```

## Notes

* **Steam friend "favourites" are our own.** Steam doesn't expose its favourites or
  friend categories through the Web API (or any readable config), so the Friends
  widget keeps its own per-instance favourites instead.
* **"Last online" can be inaccurate** — Steam fuzzes/hides `lastlogoff`, so some
  offline friends show stale values. That's Steam's data, not a bug here.
* The friends presence service only reports friends whose game details are public.
  "Join Game" is a best-effort `steam://` call; the real lobby token isn't on the
  Web API, so it may not connect for every game.

## License

MIT
