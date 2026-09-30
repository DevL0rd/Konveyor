import os

PAGE = os.sysconf("SC_PAGE_SIZE")
_O_RDONLY = os.O_RDONLY
_io_denied = {}
_stat_cache = {}
_io_cache = {}
_stat_fds = {}
_io_fds = {}


def read_procs():
    out = {}
    parsed = {}
    previous = _stat_cache
    changed = set()
    fds = _stat_fds
    fd_get = fds.get
    cache_get = previous.get
    pread, open_file, close_file = os.pread, os.open, os.close
    for name in os.listdir("/proc"):
        if not name.isdigit():
            continue
        pid = int(name)
        cached = cache_get(pid)
        fd = fd_get(pid)
        line = None
        if fd is not None:
            try:
                line = pread(fd, 1024, 0)
            except OSError:
                close_file(fd)
                del fds[pid]
        if line is None:
            try:
                fd = open_file("/proc/" + name + "/stat", _O_RDONLY)
            except OSError:
                continue
            try:
                line = pread(fd, 1024, 0)
            except OSError:
                close_file(fd)
                continue
            fds[pid] = fd
        if cached is not None and cached[0] == line:
            parsed[pid] = cached
            out[pid] = cached[1]
            continue
        r = line.rfind(b")")
        rest = line[r + 2:].split(None, 22)
        if len(rest) < 22:
            continue
        values = (line[line.find(b"(") + 1:r].decode(errors="replace"), int(rest[1]),
                  int(rest[11]) + int(rest[12]), int(rest[21]) * PAGE, int(rest[17]), rest[19])
        parsed[pid] = (line, values)
        out[pid] = values
        changed.add(pid)
    _close_gone(fds, out)
    _stat_cache.clear()
    _stat_cache.update(parsed)
    return out, changed


def read_io(procs):
    out = {}
    denied = _io_denied
    denied_get = denied.get
    still = {}
    previous = _io_cache
    cache_get = previous.get
    parsed = {}
    changed = set()
    fds = _io_fds
    fd_get = fds.get
    pread, open_file, close_file = os.pread, os.open, os.close
    for pid, p in procs.items():
        start = p[5]
        if denied_get(pid) == start:
            still[pid] = start
            continue
        fd = fd_get(pid)
        data = None
        if fd is not None:
            try:
                data = pread(fd, 512, 0)
            except OSError:
                close_file(fd)
                del fds[pid]
        if data is None:
            try:
                fd = open_file("/proc/" + str(pid) + "/io", _O_RDONLY)
            except PermissionError:
                still[pid] = start
                continue
            except OSError:
                continue
            try:
                data = pread(fd, 512, 0)
            except PermissionError:
                close_file(fd)
                still[pid] = start
                continue
            except OSError:
                close_file(fd)
                continue
            fds[pid] = fd
        cached = cache_get(pid)
        if cached is not None and cached[0] == data:
            parsed[pid] = cached
            out[pid] = cached[1]
            continue
        rb = data.find(b"read_bytes:")
        wb = data.find(b"write_bytes:", rb)
        if rb < 0 or wb < 0:
            continue
        total = int(data[rb + 11:data.find(b"\n", rb)]) + int(data[wb + 12:data.find(b"\n", wb)])
        parsed[pid] = (data, total)
        out[pid] = total
        changed.add(pid)
    _close_gone(fds, out)
    denied.clear()
    denied.update(still)
    _io_cache.clear()
    _io_cache.update(parsed)
    return out, changed


def _close_gone(fds, alive):
    for pid in [pid for pid in fds if pid not in alive]:
        os.close(fds.pop(pid))
