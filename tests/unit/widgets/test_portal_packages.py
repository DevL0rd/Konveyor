#!/usr/bin/env python3
import json
import unittest

from portal_harness import PORTAL_BIN, Sandbox


SCRIPT = PORTAL_BIN / "portal-packages"


def repo(name, **fields):
    return {"Name": name, "Version": "1.0-1", "Description": name + " package", "Repository": "extra", **fields}


def aur(name, **fields):
    return {"Name": name, "Version": "2.0-1", "Description": name + " from the AUR", "NumVotes": 5, "Popularity": 0.5, **fields}


class TestPortalPackages(unittest.TestCase):
    def setUp(self):
        self.box = Sandbox(stubs=("shelly", "pacman"))
        self.box.stub("pacman", code=1)

    def tearDown(self):
        self.box.cleanup()

    def search(self, *arguments, standard=None, found_in_aur=None):
        if standard is not None:
            self.box.stub("shelly", stdout=standard if isinstance(standard, str) else json.dumps(standard), when="standard")
        if found_in_aur is not None:
            self.box.stub("shelly", stdout=found_in_aur if isinstance(found_in_aur, str) else json.dumps(found_in_aur), when="aur")
        return self.box.run_json(SCRIPT, *arguments)

    def names(self, result):
        return [package["name"] for package in result["packages"]]

    def test_an_empty_query_asks_nothing(self):
        for arguments in ((), ("  ",)):
            self.assertEqual(self.box.run_json(SCRIPT, *arguments), {"query": "", "packages": []})
        self.assertEqual(self.box.calls(), [])

    def test_reports_a_missing_shelly(self):
        (self.box.stubs / "shelly").unlink()
        self.assertEqual(self.box.run_json(SCRIPT, "gimp"), {"query": "gimp", "packages": [], "error": "shelly is not installed"})

    def test_asks_shelly_for_repository_and_aur_packages(self):
        self.search(" gimp ", standard=[], found_in_aur=[])
        calls = [call for call in self.box.calls() if call[0] == "shelly"]
        self.assertCountEqual(calls, [["shelly", "search", "standard", "gimp", "-v", "-t", "40", "-j"],
                                      ["shelly", "search", "aur", "gimp", "-j"]])

    def test_describes_repository_and_aur_packages(self):
        result = self.search("gimp", standard=[repo("gimp", Url="https://gimp.org")],
                             found_in_aur=[aur("gimp-git", OutOfDate=1700000000, Url=None)])
        self.assertEqual(result["query"], "gimp")
        self.assertEqual(result["packages"], [
            {"name": "gimp", "version": "1.0-1", "description": "gimp package", "source": "standard", "repo": "extra",
             "url": "https://gimp.org", "page": "https://archlinux.org/packages/?q=gimp", "votes": 0, "popularity": 0,
             "outOfDate": False, "rank": 0},
            {"name": "gimp-git", "version": "2.0-1", "description": "gimp-git from the AUR", "source": "aur", "repo": "AUR",
             "url": "", "page": "https://aur.archlinux.org/packages/gimp-git", "votes": 5, "popularity": 0.5,
             "outOfDate": True, "rank": 1},
        ])

    def test_leaves_out_installed_packages(self):
        self.box.stub("pacman", stdout="gimp\nkrita\n", code=1)
        result = self.search("art", standard=[repo("gimp"), repo("krita"), repo("mypaint"), repo("inkscape", InstallReason="Explicitly installed")],
                             found_in_aur=[aur("krita"), aur("artha")])
        self.assertEqual(self.names(result), ["artha", "mypaint"])
        pacman = [call for call in self.box.calls() if call[0] == "pacman"]
        self.assertEqual(pacman, [["pacman", "-Qq", "artha", "gimp", "inkscape", "krita", "mypaint"]])

    def test_the_repository_wins_over_the_aur_and_translations_are_left_out(self):
        result = self.search("firefox", standard=[repo("firefox"), repo("firefox"), repo("firefox-i18n-de")],
                             found_in_aur=[aur("firefox"), aur("firefox-i18n-fr")])
        self.assertEqual([(package["name"], package["source"]) for package in result["packages"]], [("firefox", "standard")])

    def test_ranks_exact_then_prefix_then_substring_matches(self):
        result = self.search("Vim", standard=[repo("gvim"), repo("vim-plugins"), repo("neovim-qt"), repo("vim"), repo("editor", Description="vim")],
                             found_in_aur=[aur("vimb", OutOfDate=1), aur("vimx", Popularity=9), aur("vimy", Popularity=1)])
        self.assertEqual(self.names(result), ["vim", "vimx", "vimy", "vim-plugins", "vimb", "gvim", "neovim-qt", "editor"])
        self.assertEqual([package["rank"] for package in result["packages"]], [0, 1, 1, 1, 1, 2, 2, 3])

    def test_the_limit_defaults_to_eight_and_can_be_set(self):
        many = [repo("pkg%02d" % number) for number in range(12)]
        self.assertEqual(len(self.search("pkg", standard=many, found_in_aur=[])["packages"]), 8)
        self.assertEqual(self.names(self.search("pkg", "3", standard=many, found_in_aur=[])), ["pkg00", "pkg01", "pkg02"])

    def test_broken_shelly_output_counts_as_no_results(self):
        for standard, found_in_aur in (("error: database locked", "not json"), ("[{", "{\"Name\": 1"), ('{"Name": "x"}', "[1, null, {}]")):
            with self.subTest(standard=standard):
                self.assertEqual(self.search("x", standard=standard, found_in_aur=found_in_aur)["packages"], [])

    def test_a_failing_aur_search_keeps_the_repository_results(self):
        self.box.stub("shelly", stderr="network unreachable", code=1, when="aur")
        self.assertEqual(self.names(self.search("gimp", standard=[repo("gimp")])), ["gimp"])


if __name__ == "__main__":
    unittest.main()
