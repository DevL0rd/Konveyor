#!/usr/bin/env python3
import contextlib
import io
import json
import os
import stat
import unittest
import urllib.error
import urllib.parse
from unittest import mock

from portal_harness import PORTAL_BIN, Sandbox, load_script


SCRIPT = PORTAL_BIN / "portal-friends"
ME = "76561198000000001"
KEY = "SECRETKEY123"
LOGINUSERS = """"users"
{
    "76561198000000002"
    {
        "AccountName" "old"
        "MostRecent" "0"
    }
    "%s"
    {
        "AccountName" "me"
        "MostRecent" "1"
    }
}
""" % ME


def player(steamid, name, gameid=None, game="", **fields):
    entry = {"steamid": steamid, "personaname": name, "avatarmedium": "m-" + name, "avatarfull": "f-" + name, "personastate": 1, **fields}
    if gameid:
        entry.update(gameid=gameid, gameextrainfo=game)
    return entry


class FakeSteam:
    def __init__(self):
        self.friends = []
        self.players = {}
        self.store = {}
        self.failure = None
        self.requests = []

    def urlopen(self, request, timeout=None):
        url = request.full_url if hasattr(request, "full_url") else request
        parsed = urllib.parse.urlparse(url)
        query = dict(urllib.parse.parse_qsl(parsed.query))
        self.requests.append((parsed.netloc + parsed.path, query))
        if parsed.netloc == "api.steampowered.com":
            if self.failure:
                raise self.failure
            if parsed.path == "/ISteamUser/GetFriendList/v1/":
                body = {"friendslist": {"friends": [{"steamid": steamid} for steamid in self.friends]}}
            else:
                body = {"response": {"players": [self.players[steamid] for steamid in query["steamids"].split(",") if steamid in self.players]}}
        elif parsed.netloc == "store.steampowered.com":
            body = self.store[query["appids"]]
            if isinstance(body, Exception):
                raise body
        else:
            raise AssertionError("unexpected request to " + url)
        return io.BytesIO(json.dumps(body).encode())

    def calls_to(self, path):
        return [query for where, query in self.requests if where.endswith(path)]


class TestPortalFriends(unittest.TestCase):
    def setUp(self):
        self.box = Sandbox()
        self.module = load_script(SCRIPT, self.box.environment)
        self.steam = FakeSteam()
        patcher = mock.patch("urllib.request.urlopen", self.steam.urlopen)
        patcher.start()
        self.addCleanup(patcher.stop)
        self.config = self.box.config / "Plasma-App-Portal" / "config.json"
        self.snapshot = self.box.runtime / "Plasma-App-Portal" / "friends.json"

    def tearDown(self):
        self.box.cleanup()

    def configure(self, **fields):
        self.box.write(self.config, json.dumps(fields))

    def run_mode(self, *arguments):
        output = io.StringIO()
        with mock.patch("sys.argv", ["portal-friends", *arguments]), contextlib.redirect_stdout(output):
            code = self.module.main()
        return code, output.getvalue()

    def add_friends(self, *players):
        for entry in players:
            self.steam.friends.append(entry["steamid"])
            self.steam.players[entry["steamid"]] = entry

    def test_the_example_config_asks_for_a_key(self):
        self.box.write(self.config, (PORTAL_BIN.parent / "config.example.json").read_text())
        snapshot = self.module.build_snapshot(self.module.load_config())
        self.assertEqual((snapshot["ok"], snapshot["needs_api_key"]), (False, True))
        self.assertEqual(self.steam.requests, [])

    def test_a_missing_or_broken_config_asks_for_a_key(self):
        self.assertTrue(self.module.build_snapshot(self.module.load_config())["needs_api_key"])
        self.box.write(self.config, "{")
        self.assertTrue(self.module.build_snapshot(self.module.load_config())["needs_api_key"])

    def test_detects_the_most_recent_steam_account(self):
        self.box.write(self.box.home / ".steam" / "steam" / "config" / "loginusers.vdf", LOGINUSERS)
        self.assertEqual(self.module.detect_steamid(), ME)
        self.box.write(self.box.home / ".local" / "share" / "Steam" / "config" / "loginusers.vdf", LOGINUSERS.replace('"MostRecent" "1"', '"MostRecent" "0"'))
        self.assertEqual(self.module.detect_steamid(), "76561198000000002")

    def test_the_most_recent_account_wins_when_another_remembers_its_password(self):
        loginusers = LOGINUSERS.replace('"AccountName" "old"', '"AccountName" "old"\n        "RememberPassword" "1"\n        "AllowAutoLogin" "1"')
        self.box.write(self.box.home / ".steam" / "steam" / "config" / "loginusers.vdf", loginusers)
        self.assertEqual(self.module.detect_steamid(), ME)

    def test_finds_the_account_of_the_flatpak_steam_client(self):
        self.box.write(self.box.home / ".var" / "app" / "com.valvesoftware.Steam" / "data" / "Steam" / "config" / "loginusers.vdf", LOGINUSERS)
        self.assertEqual(self.module.detect_steamid(), ME)

    def test_a_steamid_written_as_a_number_is_used(self):
        self.box.write(self.config, '{"steam_api_key": "%s", "steamid": %s}' % (KEY, ME))
        snapshot = self.module.build_snapshot(self.module.load_config())
        self.assertEqual((snapshot["ok"], snapshot["self"]), (True, ME))
        self.assertEqual(self.steam.calls_to("GetFriendList/v1/")[0]["steamid"], ME)

    def test_a_config_that_is_not_an_object_asks_for_a_key_instead_of_crashing(self):
        for text in ("[]", "null", '"key"', "7"):
            self.box.write(self.config, text)
            self.assertEqual(self.run_mode("--check")[0], 1)
            interval, _, snapshot = self.serve_once()
            self.assertEqual((interval, snapshot["needs_api_key"]), (60.0, True), text)

    def test_a_configured_steamid_wins(self):
        self.box.write(self.box.home / ".steam" / "steam" / "config" / "loginusers.vdf", LOGINUSERS)
        self.configure(steam_api_key=KEY, steamid=" 76561198000000009 ")
        self.module.build_snapshot(self.module.load_config())
        self.assertEqual(self.steam.calls_to("GetFriendList/v1/")[0]["steamid"], "76561198000000009")

    def test_without_a_steam_account_it_says_so(self):
        self.configure(steam_api_key=KEY)
        snapshot = self.module.build_snapshot(self.module.load_config())
        self.assertEqual((snapshot["ok"], snapshot["error"]), (False, "could not determine SteamID"))

    def test_groups_friends_by_the_game_they_play(self):
        self.configure(steam_api_key=KEY, steamid=ME)
        self.add_friends(player("3", "zed", "570", "Deadlock"), player("1", "Amy", "570", "Deadlock"), player("2", "bob", profileurl="https://p/bob"),
                         player("4", "Cat", "400", "Portal", personastate=3, loccountrycode="DE"))
        self.steam.store = {"570": {"570": {"success": True, "data": {"header_image": "h570"}}}, "400": {"400": {"success": False}}}
        snapshot = self.module.build_snapshot(self.module.load_config())
        self.assertEqual((snapshot["ok"], snapshot["self"], snapshot["count"], snapshot["error"]), (True, ME, 4, ""))
        self.assertEqual([friend["name"] for friend in snapshot["friends"]], ["Amy", "bob", "Cat", "zed"])
        self.assertEqual({appid: [friend["name"] for friend in friends] for appid, friends in snapshot["by_appid"].items()},
                         {"570": ["Amy", "zed"], "400": ["Cat"]})
        cat = snapshot["friends"][2]
        self.assertEqual((cat["state"], cat["ingame"], cat["appid"], cat["country"], cat["header"], cat["avatar"]), (3, True, "400", "DE", "", "f-Cat"))
        self.assertEqual(cat["capsule"], "https://cdn.cloudflare.steamstatic.com/steam/apps/400/capsule_184x69.jpg")
        self.assertEqual(cat["join"], "steam://joinlobby/400/4")
        bob = snapshot["friends"][1]
        self.assertEqual((bob["ingame"], bob["join"], bob["capsule"], bob["header"], bob["profile_web"]), (False, "", "", "", "https://p/bob"))
        self.assertEqual(snapshot["friends"][0]["header"], "h570")
        self.assertEqual(snapshot["by_appid"]["570"][0], {"steamid": "1", "name": "Amy", "avatar": "m-Amy", "game": "Deadlock",
                                                          "profile": "steam://url/SteamIDPage/1", "watch": "steam://broadcast/watch/1",
                                                          "message": "steam://friends/message/1", "join": "steam://joinlobby/570/1"})

    def test_sends_the_key_and_asks_for_summaries_a_hundred_at_a_time(self):
        self.configure(steam_api_key=KEY, steamid=ME)
        self.add_friends(*[player(str(number), "f%03d" % number) for number in range(250)])
        snapshot = self.module.build_snapshot(self.module.load_config())
        self.assertEqual(len(snapshot["friends"]), 250)
        batches = self.steam.calls_to("GetPlayerSummaries/v2/")
        self.assertEqual([len(query["steamids"].split(",")) for query in batches], [100, 100, 50])
        self.assertTrue(all(query["key"] == KEY for _, query in self.steam.requests))
        self.assertEqual(self.steam.calls_to("GetFriendList/v1/"), [{"steamid": ME, "relationship": "friend", "key": KEY}])

    def test_without_friends_it_skips_the_summaries(self):
        self.configure(steam_api_key=KEY, steamid=ME)
        snapshot = self.module.build_snapshot(self.module.load_config())
        self.assertEqual((snapshot["ok"], snapshot["count"], snapshot["friends"]), (True, 0, []))
        self.assertEqual(self.steam.calls_to("GetPlayerSummaries/v2/"), [])

    def test_caches_store_headers_and_retries_failures(self):
        self.steam.store = {"1": {"1": {"success": True, "data": {"header_image": "one"}}}, "2": urllib.error.URLError("offline")}
        self.assertEqual(self.module.resolve_headers(["1", "2"]), {"1": "one"})
        self.steam.store["2"] = {"2": {"success": True, "data": {}}}
        self.assertEqual(self.module.resolve_headers(["1", "2"]), {"1": "one", "2": ""})
        self.assertEqual([query["appids"] for query in self.steam.calls_to("/api/appdetails")], ["1", "2", "2"])

    def test_fetches_at_most_eight_headers_per_poll(self):
        self.steam.store = {str(appid): {str(appid): {"success": True, "data": {"header_image": "h"}}} for appid in range(10)}
        self.assertEqual(len(self.module.resolve_headers([str(appid) for appid in range(10)])), 8)
        self.assertEqual(len(self.module.resolve_headers([str(appid) for appid in range(10)])), 10)

    def test_check_prints_counts_but_never_the_key(self):
        self.configure(steam_api_key=KEY, steamid=ME)
        self.add_friends(player("1", "Amy", "570", "Deadlock"), player("2", "Bob"))
        self.steam.store = {"570": {"570": {"success": True, "data": {"header_image": "h"}}}}
        code, output = self.run_mode("--check")
        self.assertEqual(code, 0)
        self.assertIn(f"OK  steamid={ME}  friends=2  appids-with-friends=1  friends-in-game=1", output)
        self.assertIn("appid 570: 1 -> ['Amy'] (Deadlock)", output)
        self.assertNotIn(KEY, output)

    def test_check_explains_what_is_missing(self):
        code, output = self.run_mode()
        self.assertEqual(code, 1)
        self.assertIn("NOT OK: no steam_api_key in config", output)
        self.assertIn(f"config:  {self.config} (exists: False)", output)
        self.assertIn("steamid: unknown", output)

    def test_check_reports_a_rejected_key_without_leaking_it(self):
        self.configure(steam_api_key=KEY, steamid=ME)
        self.steam.failure = urllib.error.HTTPError("https://api.steampowered.com/?key=" + KEY, 403, "Forbidden", {}, None)
        code, output = self.run_mode("--check")
        self.assertEqual(code, 1)
        self.assertIn("NOT OK: HTTP Error 403: Forbidden", output)
        self.assertNotIn(KEY, output)

    def test_snapshot_prints_the_last_poll(self):
        self.assertEqual(self.run_mode("--snapshot"), (None, '{"ts":0,"ok":false,"by_appid":{},"error":"no snapshot"}'))
        self.box.write(self.snapshot, '{"ok": true}')
        self.assertEqual(self.run_mode("--snapshot")[1], '{"ok": true}')

    def test_set_key_keeps_the_other_settings_and_polls(self):
        self.configure(steamid=ME, poll_interval=30, extra="kept")
        code, output = self.run_mode("--set-key", f"  {KEY} ")
        self.assertEqual((code, output), (0, "ok\n"))
        self.assertEqual(json.loads(self.config.read_text()), {"steamid": ME, "poll_interval": 30, "extra": "kept", "steam_api_key": KEY})
        self.assertEqual(stat.S_IMODE(os.stat(self.config).st_mode), 0o600)
        self.assertTrue(json.loads(self.snapshot.read_text())["ok"])

    def test_set_key_on_a_fresh_config_fills_the_defaults(self):
        self.box.write(self.box.home / ".steam" / "steam" / "config" / "loginusers.vdf", LOGINUSERS)
        self.assertEqual(self.run_mode("--set-key", KEY)[0], 0)
        self.assertEqual(json.loads(self.config.read_text()), {"steam_api_key": KEY, "steamid": "", "poll_interval": 10})

    def test_set_key_reports_a_rejected_key(self):
        self.configure(steamid=ME)
        self.steam.failure = urllib.error.HTTPError("https://api.steampowered.com/", 403, "Forbidden", {}, None)
        code, output = self.run_mode("--set-key", KEY)
        self.assertEqual((code, output), (1, "HTTP Error 403: Forbidden\n"))
        self.assertEqual(json.loads(self.snapshot.read_text())["error"], "HTTP Error 403: Forbidden")

    def test_set_key_needs_a_key(self):
        for arguments in (["--set-key"], ["--set-key", "   "]):
            self.assertEqual(self.run_mode(*arguments), (1, "no key given\n"))
        self.assertFalse(self.config.exists())

    def serve_once(self):
        with mock.patch("time.sleep", side_effect=KeyboardInterrupt) as sleep, contextlib.redirect_stderr(io.StringIO()) as errors:
            with self.assertRaises(KeyboardInterrupt):
                self.run_mode("--serve")
        return sleep.call_args.args[0], errors.getvalue(), json.loads(self.snapshot.read_text())

    def test_serve_writes_a_snapshot_every_interval(self):
        self.configure(steam_api_key=KEY, steamid=ME, poll_interval=15)
        self.add_friends(player("1", "Amy"))
        interval, _, snapshot = self.serve_once()
        self.assertEqual((interval, snapshot["ok"], snapshot["count"]), (15.0, True, 1))

    def test_serve_polls_every_minute_by_default_and_records_errors(self):
        self.configure(steam_api_key=KEY, steamid=ME)
        self.steam.failure = urllib.error.URLError("offline")
        interval, errors, snapshot = self.serve_once()
        self.assertEqual(interval, 60.0)
        self.assertIn("portal-friends: <urlopen error offline>", errors)
        self.assertEqual((snapshot["ok"], snapshot["error"]), (False, "<urlopen error offline>"))


if __name__ == "__main__":
    unittest.main()
