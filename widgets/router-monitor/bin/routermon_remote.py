import os
import time

from routermon_extras import _num

SOCK = os.path.join(os.environ.get("XDG_RUNTIME_DIR", "/tmp"), "Linux-Router-Monitor", "cm.sock")


def ssh_base(cfg):
    key = os.path.expanduser(cfg.get("ssh_key", "~/.ssh/id_ed25519"))
    return [
        "ssh",
        "-o", "BatchMode=yes",
        "-o", "ConnectTimeout=6",
        "-o", "ControlMaster=auto",
        "-o", f"ControlPath={SOCK}",
        "-o", f"ControlPersist={cfg.get('control_persist', '60')}",
        "-o", "StrictHostKeyChecking=accept-new",
        "-i", key,
        f"{cfg.get('user', 'admin')}@{cfg['host']}",
    ]


SESSION_DONE = b"__LRM_DONE__"
SESSION_TIMEOUT = 20
_session = {"proc": None, "cmd": None}


def _close_session():
    proc = _session["proc"]
    _session["proc"] = None
    if proc is not None:
        try:
            proc.kill()
            proc.wait(timeout=2)
        except Exception:
            pass


def _session_for(cfg):
    import subprocess
    script = cfg.get("remote_script", "/jffs/lrm-collect.sh")
    loop = (f"while read -r a b c d; do sh {script} \"$a\" \"$b\" \"$c\" \"$d\" </dev/null 2>/dev/null; "
            f"echo {SESSION_DONE.decode()}; done")
    cmd = ssh_base(cfg) + [loop]
    proc = _session["proc"]
    if proc is not None and (proc.poll() is not None or _session["cmd"] != cmd):
        _close_session()
        proc = None
    if proc is None:
        if not _session.get("cleanup"):
            import atexit
            atexit.register(_close_session)
            _session["cleanup"] = True
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        _session["proc"], _session["cmd"] = proc, cmd
    return proc


def _session_request(cfg, args):
    import select
    proc = _session_for(cfg)
    try:
        proc.stdin.write(args.encode() + b"\n")
        proc.stdin.flush()
        fd = proc.stdout.fileno()
        deadline = time.monotonic() + SESSION_TIMEOUT
        chunks = []
        tail = b""
        marker = SESSION_DONE + b"\n"
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([fd], [], [], remaining)[0]:
                raise RuntimeError("remote collect timed out")
            data = os.read(fd, 65536)
            if not data:
                raise RuntimeError(f"remote session ended rc={proc.poll()}")
            chunks.append(data)
            tail = (tail + data)[-(len(marker) + 1):]
            if tail.endswith(marker):
                break
    except (OSError, ValueError, RuntimeError):
        _close_session()
        raise
    out = b"".join(chunks)[:-len(marker)]
    return out.decode(errors="replace")


def fetch_remote(cfg, do_ping=True, do_slow=True, do_static=False):
    ping_mode = do_ping if do_ping in (2, 3) else (1 if do_ping else 0)
    args = (f"{cfg.get('ping_target', '8.8.8.8')} {ping_mode} "
            f"{1 if do_slow else 0} {1 if do_static else 0}")
    try:
        out = _session_request(cfg, args)
    except (OSError, ValueError, RuntimeError) as e:
        raise RuntimeError(f"remote collect failed: {e}") from e
    if "END=1" not in out:
        _close_session()
        raise RuntimeError("remote collect failed: incomplete output")
    return out


def parse_remote(raw):
    d = {"radios": [], "radios_static": [], "stations": [], "stations_slow": [],
         "leases": [], "ifaces": {}, "ports": [], "arp": [], "blocked": [],
         "rcount": [], "_stat": {}}
    for line in raw.splitlines():
        if not line:
            continue
        if line.startswith("STAT "):
            parts = line.split()
            d["_stat"][parts[1]] = [int(x) for x in parts[2:]]
        elif line.startswith("IF "):
            _, name, rx, tx = line.split()
            d["ifaces"][name] = {"rx": int(rx), "tx": int(tx)}
        elif line.startswith("PORT "):
            p = line.split(maxsplit=2)
            d["ports"].append({"port": p[1], "link": p[2]})
        elif line.startswith("RADIO "):
            d["radios"].append(_kv(line[6:]))
        elif line.startswith("RSTATIC "):
            d["radios_static"].append(_kv(line[8:]))
        elif line.startswith("RCOUNT "):
            d["rcount"].append(_kv(line[7:]))
        elif line.startswith("STA "):
            d["stations"].append(_kv(line[4:]))
        elif line.startswith("STASLOW "):
            d["stations_slow"].append(_kv(line[8:]))
        elif line.startswith("LEASE "):
            d["leases"].append(_kv(line[6:]))
        elif line.startswith("ARP "):
            parts = line.split()
            d["arp"].append({"mac": parts[1], "flags": parts[2] if len(parts) > 2 else "0x0"})
        elif line.startswith("BLOCKED "):
            d["blocked"].append(line.split()[1])
        elif "=" in line:
            k, v = line.split("=", 1)
            d[k] = v
    return d


def _kv(s):
    out = {}
    for tok in s.split():
        if "=" in tok:
            k, v = tok.split("=", 1)
            out[k] = v
        else:  # value contained spaces (e.g. ssid) -> append to last key
            if out:
                out[list(out)[-1]] += " " + tok
    return out


def compute_static(cfg):
    """Router identity + ssid/radio/txpower + wired ports. These change only on
    reconfiguration (which drops the connection), so we refetch them only when the
    router transitions offline -> online, not on a timer."""
    p = parse_remote(fetch_remote(cfg, do_ping=False, do_slow=False, do_static=True))
    return {
        "info": {"model": p.get("model"), "fw": p.get("fw"), "lan_ip": p.get("lan_ip")},
        "ports": [pt for pt in p["ports"] if "Up" in pt["link"]] or p["ports"],
        "radios": {r.get("band"): {"ssid": r.get("ssid"), "on": r.get("radio") == "1",
                                   "txpower": _num(r.get("txpower"), int)}
                   for r in p.get("radios_static", [])},
    }


def compute_ping(cfg):
    """WAN latency via the router (~2.6s). Separate from build() so it never
    blocks the fast snapshot."""
    import re
    import subprocess
    cmd = ssh_base(cfg) + [f"ping -c 3 -w 4 {cfg.get('ping_target', '8.8.8.8')}"]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=15).stdout
    except (OSError, subprocess.SubprocessError):
        return None
    m = re.search(r"=\s*[\d.]+/([\d.]+)/", out)
    if not m:
        return None
    loss = re.search(r"(\d+)% packet loss", out)
    return {"rtt": round(float(m.group(1)), 1), "loss": int(loss.group(1)) if loss else 0}


def merge_static(snap, sc, cfg):
    """Fold a freshly-fetched static block into an already-built snapshot."""
    info = dict(sc.get("info") or {})
    info["admin_url"] = ("http://" + info["lan_ip"]) if info.get("lan_ip") else ""
    info["agh_url"] = cfg.get("adguard", {}).get("url", "")
    info["uptime"] = (snap.get("info") or {}).get("uptime", 0)
    snap["info"] = info
    snap.setdefault("network", {})["ports"] = sc.get("ports", [])
    sr = sc.get("radios", {})
    for r in (snap.get("wifi") or {}).get("radios", []):
        r.update(sr.get(r.get("band"), {}))
