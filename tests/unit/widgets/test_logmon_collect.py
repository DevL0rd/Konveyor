#!/usr/bin/env python3
import json
import subprocess
import sys
import unittest

from collectorharness import RECORDING_STUB, WIDGETS, CollectorTest, load_script, write_stub

SCRIPT = WIDGETS / "system-log" / "bin" / "logmon-collect"
JOURNALCTL = RECORDING_STUB + """sys.stdout.write(open(os.environ["JOURNAL"]).read())
sys.stdout.flush()
if os.environ.get("JOURNAL_ERROR"):
    sys.stderr.write(os.environ["JOURNAL_ERROR"] + "\\n")
sys.exit(int(os.environ.get("JOURNAL_EXIT", "0")))
"""


def record(**fields):
    return json.dumps(fields) + "\n"


class TestLogmonParse(unittest.TestCase):
    def setUp(self):
        self.parse = load_script(SCRIPT, "logmon_collect").parse

    def test_keeps_the_fields_the_widget_shows(self):
        entry = self.parse(record(__REALTIME_TIMESTAMP="1700000000000001", PRIORITY="3", SYSLOG_IDENTIFIER="sshd",
                                  _SYSTEMD_UNIT="sshd.service", _PID="42", MESSAGE="Accepted key"))
        self.assertEqual(entry, {"t": 1700000000000001, "p": 3, "id": "sshd", "u": "sshd.service", "pid": "42", "m": "Accepted key"})

    def test_falls_back_through_the_identifier_sources(self):
        self.assertEqual(self.parse(record(_COMM="bash", MESSAGE="x"))["id"], "bash")
        self.assertEqual(self.parse(record(_TRANSPORT="kernel", MESSAGE="x"))["id"], "kernel")
        self.assertEqual(self.parse(record(UNIT="foo.service", MESSAGE="x"))["id"], "foo")
        self.assertEqual(self.parse(record(UNIT="foo.timer", MESSAGE="x"))["id"], "foo.timer")
        self.assertEqual(self.parse(record(MESSAGE="x"))["id"], "?")
        self.assertEqual(self.parse(record(SYSLOG_IDENTIFIER="i" * 60, MESSAGE="x"))["id"], "i" * 40)
        self.assertEqual(self.parse(record(SYSLOG_PID="7", MESSAGE="x"))["pid"], "7")

    def test_decodes_binary_messages(self):
        self.assertEqual(self.parse(record(MESSAGE=list(b"ol\xc3\xa9 \xff")))["m"], "ol\u00e9 \ufffd")

    def test_defaults_bad_numbers(self):
        entry = self.parse(record(__REALTIME_TIMESTAMP="soon", PRIORITY="loud", MESSAGE="x"))
        self.assertEqual((entry["t"], entry["p"]), (0, 6))

    def test_skips_blank_and_broken_lines(self):
        self.assertIsNone(self.parse("\n"))
        self.assertIsNone(self.parse("{not json\n"))


class TestLogmonServe(CollectorTest):
    def setUp(self):
        super().setUp()
        write_stub(self.stubs, "journalctl", JOURNALCTL)
        self.journal = self.home / "journal"
        self.journal.write_text("")
        self.environment["JOURNAL"] = str(self.journal)

    def serve(self, **environment):
        return subprocess.run([sys.executable, str(SCRIPT), "--serve"], env={**self.environment, **environment},
                              capture_output=True, text=True, timeout=30)

    def cache(self):
        return json.loads((self.runtime / "Linux-Log-Monitor" / "log.json").read_text())

    def test_mirrors_the_journal_and_marks_it_dead_when_journalctl_ends(self):
        self.journal.write_text(record(MESSAGE="one", __REALTIME_TIMESTAMP="1") + "garbage\n" + record(MESSAGE="two", __REALTIME_TIMESTAMP="2"))
        result = self.serve()
        self.assertEqual(result.returncode, 1)
        cache = self.cache()
        self.assertFalse(cache["alive"])
        self.assertEqual([line["m"] for line in cache["lines"]], ["one", "two"])
        call = self.calls()[0]
        self.assertEqual(call[:5], ["journalctl", "-f", "-o", "json", "--all"])
        self.assertEqual(call[-2:], ["-n", "500"])

    def test_the_ring_buffer_keeps_the_newest_lines(self):
        self.journal.write_text("".join(record(MESSAGE=str(n)) for n in range(10)))
        self.serve(LOGMON_MAX_LINES="3")
        self.assertEqual([line["m"] for line in self.cache()["lines"]], ["7", "8", "9"])
        self.assertEqual(self.calls()[0][-2:], ["-n", "3"])

    def test_extra_journal_arguments_are_passed_on(self):
        self.serve(LOGMON_JOURNAL_ARGS="_UID=1000 --priority=4")
        self.assertEqual(self.calls()[0][-2:], ["_UID=1000", "--priority=4"])

    def test_no_journal_access_is_reported(self):
        result = self.serve(JOURNAL_ERROR="No journal files were opened due to insufficient permissions.", JOURNAL_EXIT="1")
        self.assertEqual(result.returncode, 1)
        self.assertIn("insufficient permissions", result.stderr)
        self.assertIn("journalctl exited with status 1", result.stderr)
        self.assertEqual(self.cache(), {**self.cache(), "alive": False, "lines": []})

    def test_a_second_server_leaves_the_first_alone(self):
        import fcntl
        (self.runtime / "Linux-Log-Monitor").mkdir()
        with open(self.runtime / "Linux-Log-Monitor" / "serve.lock", "w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            result = self.serve()
        self.assertIn("already running", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_usage_without_serve(self):
        result = subprocess.run([sys.executable, str(SCRIPT)], env=self.environment, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("logmon-collect --serve", result.stderr)


if __name__ == "__main__":
    unittest.main()
