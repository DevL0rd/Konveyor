#!/usr/bin/env python3
import os
import unittest
from pathlib import Path
from unittest import mock

from collectorharness import WIDGETS, CollectorTest
from test_procmon_collect import stat_line

SCRIPT = WIDGETS / "process-monitor" / "bin" / "procmon_proc.py"
IO = "rchar: 1\nread_bytes: %d\nwrite_bytes: %d\ncancelled_write_bytes: 0\n"


class TestProcmonProc(CollectorTest):
    def setUp(self):
        super().setUp()
        self.module = self.load(SCRIPT, "procmon_proc_direct")
        self.module.PAGE = 4096

    def stat(self, pid, text=None, **fields):
        text = text if text is not None else stat_line(pid, "proc%d" % pid, 1, fields.get("ticks", 0), 1, fields.get("rss", 1))
        self.fake.write("/proc/%d/stat" % pid, text)

    def io(self, pid, read=0, write=0):
        return self.fake.write("/proc/%d/io" % pid, IO % (read, write))

    def failing_reads(self, *errors):
        queue = list(errors)
        real = os.pread

        def pread(fd, size, offset):
            if queue:
                raise queue.pop(0)
            return real(fd, size, offset)
        return mock.patch("os.pread", pread)

    def open_descriptors(self):
        count = 0
        for fd in range(256):
            try:
                os.fstat(fd)
                count += 1
            except OSError:
                pass
        return count

    def test_processes_are_parsed_and_unchanged_ones_reported_as_such(self):
        self.stat(5, ticks=10, rss=3)
        first, changed = self.module.read_procs()
        self.assertEqual(first[5][:5], ("proc5", 1, 10, 3 * 4096, 1))
        self.assertEqual(changed, {5})
        again, changed = self.module.read_procs()
        self.assertEqual((again, changed), (first, set()))

    def test_a_process_without_a_readable_stat_file_is_skipped(self):
        Path(self.fake.root + "/proc/6").mkdir(parents=True)
        self.stat(7)
        self.fake.write("/proc/meminfo", "MemTotal: 1 kB\n")
        self.assertEqual(set(self.module.read_procs()[0]), {7})

    def test_a_stat_file_that_cannot_be_read_is_skipped_and_closed(self):
        self.stat(8)
        before = self.open_descriptors()
        with self.failing_reads(OSError()):
            self.assertEqual(self.module.read_procs(), ({}, set()))
        self.assertEqual(self.open_descriptors(), before)

    def test_a_truncated_stat_line_is_skipped(self):
        self.stat(9, "9 (short) S 1 2 3\n")
        self.stat(10)
        self.assertEqual(set(self.module.read_procs()[0]), {10})

    def test_a_kept_descriptor_that_goes_stale_is_reopened(self):
        self.stat(11, ticks=1)
        self.module.read_procs()
        self.stat(11, ticks=2)
        with self.failing_reads(OSError()):
            rows, changed = self.module.read_procs()
        self.assertEqual((rows[11][2], changed), (2, {11}))
        self.assertIn(11, self.module._stat_fds)

    def test_descriptors_of_vanished_processes_are_closed(self):
        self.stat(12)
        self.module.read_procs()
        gone = self.module._stat_fds[12]
        os.unlink(self.fake.root + "/proc/12/stat")
        os.rmdir(self.fake.root + "/proc/12")
        self.module.read_procs()
        self.assertNotIn(12, self.module._stat_fds)
        with self.assertRaises(OSError):
            os.fstat(gone)

    def test_disk_totals_add_reads_and_writes_and_skip_unchanged_files(self):
        self.io(20, 100, 50)
        procs = {20: ("p", 1, 0, 0, 1, b"1")}
        totals, changed = self.module.read_io(procs)
        self.assertEqual((totals, changed), ({20: 150}, {20}))
        self.assertEqual(self.module.read_io(procs), ({20: 150}, set()))
        self.io(20, 300, 50)
        self.assertEqual(self.module.read_io(procs), ({20: 350}, {20}))

    def test_a_missing_io_file_gives_no_total(self):
        self.assertEqual(self.module.read_io({21: ("p", 1, 0, 0, 1, b"1")}), ({}, set()))

    def test_a_process_whose_io_is_denied_is_not_retried_until_it_restarts(self):
        path = self.io(22, 1, 1)
        path.chmod(0)
        procs = {22: ("p", 1, 0, 0, 1, b"100")}
        self.assertEqual(self.module.read_io(procs), ({}, set()))
        path.chmod(0o644)
        self.assertEqual(self.module.read_io(procs), ({}, set()))
        procs[22] = ("p", 1, 0, 0, 1, b"200")
        self.assertEqual(self.module.read_io(procs), ({22: 2}, {22}))

    def test_a_denied_read_on_an_open_io_file_is_remembered(self):
        self.io(23, 1, 1)
        before = self.open_descriptors()
        with self.failing_reads(PermissionError()):
            self.assertEqual(self.module.read_io({23: ("p", 1, 0, 0, 1, b"1")}), ({}, set()))
        self.assertEqual(self.module._io_denied, {23: b"1"})
        self.assertEqual(self.open_descriptors(), before)

    def test_a_failed_read_on_an_open_io_file_is_skipped_and_closed(self):
        self.io(24, 1, 1)
        before = self.open_descriptors()
        with self.failing_reads(OSError()):
            self.assertEqual(self.module.read_io({24: ("p", 1, 0, 0, 1, b"1")}), ({}, set()))
        self.assertEqual(self.module._io_denied, {})
        self.assertEqual(self.open_descriptors(), before)

    def test_a_kept_io_descriptor_that_goes_stale_is_reopened(self):
        self.io(25, 1, 1)
        procs = {25: ("p", 1, 0, 0, 1, b"1")}
        self.module.read_io(procs)
        self.io(25, 5, 5)
        with self.failing_reads(OSError()):
            totals, changed = self.module.read_io(procs)
        self.assertEqual((totals, changed), ({25: 10}, {25}))
        self.assertIn(25, self.module._io_fds)

    def test_an_io_file_without_the_byte_counters_gives_no_total(self):
        self.fake.write("/proc/26/io", "rchar: 1\n")
        self.assertEqual(self.module.read_io({26: ("p", 1, 0, 0, 1, b"1")}), ({}, set()))


if __name__ == "__main__":
    unittest.main()
