#!/usr/bin/env python3
import json
import sqlite3
import unittest

from portal_harness import PORTAL_BIN, Sandbox


SCRIPT = PORTAL_BIN / "portal-games"
FAVORITES = "org.kde.plasma.favorites.applications"


class TestPortalGames(unittest.TestCase):
    def setUp(self):
        self.box = Sandbox(stubs=("gdbus", "xdg-settings"))
        self.state = self.box.data / "Plasma-App-Portal"

    def tearDown(self):
        self.box.cleanup()

    def run_games(self, *arguments):
        return self.box.run(SCRIPT, *arguments)

    def games(self):
        listed = self.box.run_json(SCRIPT)["games"]
        return {game["id"]: game for game in listed if game["id"].startswith("test-")}

    def steam_cache(self, appid, *files):
        cache = self.box.data / "Steam" / "appcache" / "librarycache" / appid
        for name in files:
            self.box.write(cache / name, "art")
        return cache

    def test_lists_only_visible_games_from_the_menu(self):
        self.box.desktop(self.box.data, "test-chess.desktop", Name="Chess", Categories="Game;BoardGame;", Exec="chess %U")
        self.box.desktop(self.box.data, "test-editor.desktop", Name="Editor", Categories="Utility;")
        self.box.desktop(self.box.data, "test-hidden.desktop", Name="Hidden", Categories="Game;", Hidden="true")
        self.box.desktop(self.box.data, "test-nodisplay.desktop", Name="Quiet", Categories="Game;", NoDisplay="True")
        self.box.desktop(self.box.data, "test-link.desktop", Name="Link", Categories="Game;", Type="Link")
        games = self.games()
        self.assertEqual(sorted(games), ["test-chess"])
        self.assertEqual(games["test-chess"]["launch"], "chess")
        self.assertEqual(games["test-chess"]["appid"], "")
        self.assertFalse(games["test-chess"]["has_art"])

    def test_the_user_entry_shadows_the_system_one(self):
        self.box.desktop(self.box.system_data, "test-go.desktop", Name="System Go", Categories="Game;")
        self.box.desktop(self.box.data, "test-go.desktop", Name="User Go", Categories="Game;")
        self.box.desktop(self.box.system_data, "test-alpha.desktop", Name="alpha", Categories="Game;")
        listed = [game["name"] for game in self.box.run_json(SCRIPT)["games"] if game["id"].startswith("test-")]
        self.assertEqual(listed, ["alpha", "User Go"])

    def test_finds_steam_art_in_both_cache_layouts(self):
        self.box.desktop(self.box.data, "test-portal.desktop", Name="Portal", Categories="Game;", Exec="steam steam://rungameid/400")
        flat = self.steam_cache("400", "library_600x900.jpg", "library_hero_blur.jpg", "library_hero.jpg", "logo.png", "header.jpg")
        self.box.desktop(self.box.data, "test-nested.desktop", Name="Nested", Categories="Game;", Icon="steam_icon_620")
        nested = self.steam_cache("620", "abc123/library_600x900_2x.jpg")
        games = self.games()
        self.assertEqual(games["test-portal"]["appid"], "400")
        self.assertEqual(games["test-portal"]["portrait"], str(flat / "library_600x900.jpg"))
        self.assertEqual(games["test-portal"]["hero"], str(flat / "library_hero.jpg"))
        self.assertEqual(games["test-portal"]["logo"], str(flat / "logo.png"))
        self.assertEqual(games["test-portal"]["header"], str(flat / "header.jpg"))
        self.assertEqual(games["test-nested"]["portrait"], str(nested / "abc123" / "library_600x900_2x.jpg"))
        self.assertTrue(games["test-nested"]["has_art"])

    def test_custom_art_overrides_and_resets(self):
        self.box.desktop(self.box.data, "test-portal.desktop", Name="Portal", Categories="Game;", Exec="steam steam://rungameid/400")
        self.steam_cache("400", "library_600x900.jpg")
        picture = self.box.write(self.box.root / "cover.png", "png")
        result = self.run_games("--set-art", "test-portal", str(picture))
        self.assertEqual(result.returncode, 0, result.stderr)
        copied = self.state / "art" / "test-portal.png"
        self.assertEqual(result.stdout.strip(), str(copied))
        game = self.games()["test-portal"]
        self.assertEqual(game["portrait"], str(copied))
        self.assertTrue(game["custom_art"])
        self.assertEqual(self.run_games("--reset-art", "test-portal").returncode, 0)
        self.assertFalse(copied.exists())
        self.assertFalse(self.games()["test-portal"]["custom_art"])

    def test_art_from_a_missing_file_is_refused(self):
        result = self.run_games("--set-art", "test-portal", str(self.box.root / "missing.png"))
        self.assertEqual(result.returncode, 1)
        self.assertIn("no such file", result.stderr)

    def test_resetting_art_before_any_was_set_succeeds(self):
        result = self.run_games("--reset-art", "test-portal")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_tracks_launches(self):
        self.box.desktop(self.box.data, "test-chess.desktop", Name="Chess", Categories="Game;")
        self.assertEqual(self.run_games("--track", "test-chess").returncode, 0)
        self.assertEqual(self.run_games("--track-app", "test-chess").returncode, 0)
        usage = self.box.run_json(SCRIPT, "--usage")
        self.assertIn("file://" + str(self.box.data / "applications" / "test-chess.desktop"), usage)
        self.assertGreater(self.games()["test-chess"]["last"], 0)
        result = self.run_games("--track-app", "test-missing")
        self.assertEqual(result.returncode, 1)
        self.assertIn("no such application: test-missing.desktop", result.stderr)

    def test_hides_and_unhides_by_desktop_id(self):
        self.assertEqual(self.box.run_json(SCRIPT, "--hidden"), [])
        self.run_games("--hide", "applications:org.kde.konsole.desktop")
        self.run_games("--hide", "steam.desktop")
        self.run_games("--hide", "steam")
        self.assertEqual(self.box.run_json(SCRIPT, "--hidden"), ["org.kde.konsole", "steam"])
        self.run_games("--unhide", "org.kde.konsole.desktop")
        self.run_games("--unhide", "never-hidden")
        self.assertEqual(self.box.run_json(SCRIPT, "--hidden"), ["steam"])

    def test_a_corrupt_hidden_list_reads_as_empty(self):
        self.box.write(self.state / "hidden.json", '{"steam": true}')
        self.assertEqual(self.box.run_json(SCRIPT, "--hidden"), [])

    def test_folders_keep_each_app_in_one_place(self):
        self.run_games("--folder-create", "f1", "Tools", "a", "b", "a")
        self.run_games("--folder-create", "f2", "Games", "b", "c")
        self.assertEqual(self.box.run_json(SCRIPT, "--folders"),
                         [{"id": "f1", "name": "Tools", "apps": ["a"]}, {"id": "f2", "name": "Games", "apps": ["b", "c"]}])
        self.run_games("--folder-add", "f1", "c")
        self.run_games("--folder-rename", "f2", "Play")
        self.assertEqual(self.box.run_json(SCRIPT, "--folders"),
                         [{"id": "f1", "name": "Tools", "apps": ["a", "c"]}, {"id": "f2", "name": "Play", "apps": ["b"]}])
        self.run_games("--folder-remove", "b")
        self.assertEqual([folder["id"] for folder in self.box.run_json(SCRIPT, "--folders")], ["f1"])
        self.run_games("--folder-delete", "f1")
        self.assertEqual(self.box.run_json(SCRIPT, "--folders"), [])

    def test_adding_to_a_missing_folder_fails(self):
        result = self.run_games("--folder-add", "nope", "a")
        self.assertEqual(result.returncode, 1)
        self.assertIn("no such folder: nope", result.stderr)

    def test_renaming_a_missing_folder_fails(self):
        result = self.run_games("--folder-rename", "nope", "Name")
        self.assertEqual(result.returncode, 1)
        self.assertIn("no such folder: nope", result.stderr)

    def test_sidebar_pins_are_cleaned_and_resolved(self):
        self.box.desktop(self.box.data, "org.kde.konsole.desktop", Name="Konsole", Icon="utilities-terminal")
        music = self.box.home / "Music"
        music.mkdir()
        self.box.write(self.box.config / "user-dirs.dirs", 'XDG_MUSIC_DIR="$HOME/Music"\n')
        notes = self.box.write(self.box.home / "notes.txt", "")
        pins = [{"kind": "app", "id": "org.kde.konsole"}, {"kind": "app", "id": "org.kde.konsole"}, {"kind": "app", "id": "gone", "name": "Gone"},
                {"kind": "path", "id": "file://" + str(music)}, {"kind": "path", "id": str(notes)}, {"kind": "url", "id": "x"}, "junk"]
        self.assertEqual(self.run_games("--sidebar-set", json.dumps(pins)).returncode, 0)
        resolved = self.box.run_json(SCRIPT, "--sidebar")
        self.assertEqual([pin["name"] for pin in resolved], ["Konsole", "Gone", "Music", "notes.txt"])
        self.assertEqual(resolved[0]["icon"], "utilities-terminal")
        self.assertTrue(resolved[1]["missing"])
        self.assertEqual(resolved[1]["icon"], "application-x-executable")
        self.assertEqual((resolved[2]["icon"], resolved[2]["folder"]), ("folder-music", True))
        self.assertEqual((resolved[3]["icon"], resolved[3]["missing"]), ("text-plain", False))

    def test_sidebar_rejects_bad_input(self):
        for text, message in (("{", "invalid JSON"), ('{"kind": "app"}', "expected a list")):
            result = self.run_games("--sidebar-set", text)
            self.assertEqual(result.returncode, 1)
            self.assertIn(message, result.stderr)

    def favorite(self, *resources):
        database = self.box.data / "kactivitymanagerd" / "resources" / "database"
        database.parent.mkdir(parents=True, exist_ok=True)
        with sqlite3.connect(database) as connection:
            connection.execute("CREATE TABLE IF NOT EXISTS ResourceLink (initiatingAgent TEXT, targettedResource TEXT)")
            connection.executemany("INSERT INTO ResourceLink VALUES (?, ?)", [(FAVORITES, resource) for resource in resources])
            connection.execute("INSERT INTO ResourceLink VALUES ('other.agent', 'applications:org.kde.dolphin.desktop')")

    def test_favorites_follow_the_kickoff_database(self):
        self.box.desktop(self.box.data, "org.kde.konsole.desktop", Name="Konsole", Exec="konsole %u")
        self.box.desktop(self.box.data, "firefox.desktop", Name="Firefox", NoDisplay="true")
        self.box.desktop(self.box.data, "org.kde.dolphin.desktop", Name="Dolphin")
        self.box.stub("xdg-settings", stdout="firefox.desktop\n")
        self.favorite("applications:org.kde.konsole.desktop", "applications:uninstalled.desktop", "preferred://browser",
                      "applications:org.kde.konsole.desktop")
        favorites = self.box.run_json(SCRIPT, "--favorites")["favorites"]
        self.assertEqual([(entry["id"], entry["launch"]) for entry in favorites],
                         [("firefox.desktop", ""), ("org.kde.konsole.desktop", "konsole")])
        self.assertIn(["xdg-settings", "get", "default-web-browser"], self.box.calls())

    def test_no_favorites_database_means_no_favorites(self):
        self.assertEqual(self.box.run_json(SCRIPT, "--favorites"), {"favorites": []})

    def test_favoriting_links_the_resource_like_kickoff(self):
        self.assertEqual(self.run_games("--fav-add", "org.kde.konsole.desktop").returncode, 0)
        self.assertEqual(self.run_games("--fav-remove", "preferred://browser").returncode, 0)
        linking = ["gdbus", "call", "--session", "--dest", "org.kde.ActivityManager", "--object-path",
                   "/ActivityManager/Resources/Linking", "--method"]
        self.assertEqual(self.box.calls(), [
            [*linking, "org.kde.ActivityManager.ResourcesLinking.LinkResourceToActivity", FAVORITES,
             "applications:org.kde.konsole.desktop", ":global"],
            [*linking, "org.kde.ActivityManager.ResourcesLinking.UnlinkResourceFromActivity", FAVORITES, "preferred://browser", ":global"],
        ])

    def test_a_failed_favorite_link_is_reported(self):
        self.box.stub("gdbus", stderr="Error: The name org.kde.ActivityManager was not provided\n", code=1)
        result = self.run_games("--fav-add", "org.kde.konsole.desktop")
        self.assertEqual(result.returncode, 1)
        self.assertIn("org.kde.ActivityManager was not provided", result.stderr)

    def test_unknown_requests_print_usage(self):
        for arguments in (["--nope"], ["--hide"], ["--folder-create", "f", "name"]):
            result = self.run_games(*arguments)
            self.assertEqual(result.returncode, 2)
            self.assertIn("Usage:", result.stderr)


if __name__ == "__main__":
    unittest.main()
