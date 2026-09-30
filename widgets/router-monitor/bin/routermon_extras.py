import json
import os
import time

LOGDIR = os.path.expanduser("~/.local/state/Linux-Router-Monitor")
LOG = os.path.join(LOGDIR, "monitor.log")


def log(msg):
    try:
        os.makedirs(LOGDIR, exist_ok=True)
        # keep the log small (cap ~256 KiB)
        if os.path.exists(LOG) and os.path.getsize(LOG) > 262144:
            with open(LOG) as f:
                tail = f.readlines()[-1000:]
            with open(LOG, "w") as f:
                f.writelines(tail)
        with open(LOG, "a") as f:
            f.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')}  {msg}\n")
    except OSError:
        pass


def _toplist(items, limit=6):
    """AGH top lists are [{"name": count}, ...] -> [{"name":..,"count":..}]."""
    out = []
    for it in (items or [])[:limit]:
        if isinstance(it, dict) and it:
            k = next(iter(it))
            out.append({"name": k, "count": it[k]})
    return out


def _num(v, cast=float, default=0):
    try:
        return cast(v)
    except (TypeError, ValueError):
        return default


def cpu_util(stat, prev):
    """Return {'total': pct, 'cores': [pct,...]} from successive /proc/stat."""
    res = {"cores": []}

    def pct(cur, old):
        if not old:
            return 0.0
        dt = sum(cur) - sum(old)
        di = (cur[3] + cur[4]) - (old[3] + old[4])  # idle + iowait
        return round(100.0 * (dt - di) / dt, 1) if dt > 0 else 0.0

    res["total"] = pct(stat.get("cpu", []), prev.get("cpu", []))
    i = 0
    while f"cpu{i}" in stat:
        res["cores"].append(pct(stat[f"cpu{i}"], prev.get(f"cpu{i}", [])))
        i += 1
    return res


def rate(cur, old, dt):
    if old is None or dt <= 0 or cur < old:
        return 0.0
    return (cur - old) / dt


def _default_interface():
    try:
        choices = []
        with open("/proc/net/route") as routes:
            next(routes, None)
            for line in routes:
                fields = line.split()
                if len(fields) >= 8 and fields[1] == "00000000" and int(fields[3], 16) & 2:
                    choices.append((int(fields[6]), fields[0]))
        return min(choices)[1] if choices else ""
    except (OSError, ValueError):
        return ""


def _interface_bytes(interface):
    try:
        with open("/proc/net/dev") as devices:
            for line in devices:
                if ":" not in line:
                    continue
                name, values = line.split(":", 1)
                if name.strip() == interface:
                    fields = values.split()
                    return int(fields[0]), int(fields[8])
    except (OSError, ValueError, IndexError):
        pass
    return 0, 0


def local_network_fallback(cfg, state, error):
    """Keep the compact widget useful while router SSH is not configured."""
    import re
    import subprocess

    now = time.time()
    previous = state.get("local_fallback") or {}
    interface = _default_interface()
    rx, tx = _interface_bytes(interface)
    dt = now - previous.get("ts", now)
    down = rate(rx, previous.get("rx") if previous.get("interface") == interface else None, dt)
    up = rate(tx, previous.get("tx") if previous.get("interface") == interface else None, dt)
    ping_rtt = previous.get("ping_rtt", 0)
    ping_loss = previous.get("ping_loss", 100)
    last_ping = previous.get("last_ping", 0)
    if not 0 <= now - last_ping < 5:
        try:
            result = subprocess.run(
                ["ping", "-n", "-c", "1", "-W", "1", str(cfg.get("ping_target", "8.8.8.8"))],
                capture_output=True, text=True, timeout=3,
            )
            match = re.search(r"time[=<]([\d.]+)\s*ms", result.stdout)
            ping_rtt = round(float(match.group(1)), 1) if match else 0
            ping_loss = 0 if result.returncode == 0 else 100
        except (OSError, subprocess.SubprocessError, ValueError):
            ping_rtt, ping_loss = 0, 100
        last_ping = now
    state["local_fallback"] = {
        "ts": now, "interface": interface, "rx": rx, "tx": tx,
        "ping_rtt": ping_rtt, "ping_loss": ping_loss, "last_ping": last_ping,
    }
    return {
        "ts": now, "online": False, "paused": False, "fallback": "local", "error": error,
        "network": {
            "down_mbps": round(down * 8 / 1e6, 2),
            "up_mbps": round(up * 8 / 1e6, 2),
            "ping_rtt": ping_rtt, "ping_loss": ping_loss,
        },
    }, state


def fetch_agh(cfg):
    import base64
    import urllib.error
    import urllib.request
    a = cfg.get("adguard", {})
    if not a.get("enabled") or not a.get("url"):
        return None
    base = a["url"].rstrip("/")
    hdr = {}
    if a.get("username"):
        tok = base64.b64encode(f"{a['username']}:{a.get('password','')}".encode()).decode()
        hdr["Authorization"] = f"Basic {tok}"
    out = {}
    for ep, key in (("/control/stats", "stats"), ("/control/status", "status")):
        try:
            req = urllib.request.Request(base + ep, headers=hdr)
            with urllib.request.urlopen(req, timeout=5) as r:
                out[key] = json.load(r)
        except (urllib.error.URLError, ValueError, OSError) as e:
            log(f"AGH {ep} failed: {e}")
            out[key] = None
    return out


def compute_dns(cfg, agh_prev, leases, now):
    """Fetch AdGuard stats and build the dns block. Separate from build() so its
    ~0.6s latency runs on the medium tick without delaying the fast snapshot."""
    agh = fetch_agh(cfg)
    if not (agh and agh.get("stats")):
        return None, agh_prev
    s = agh["stats"]
    q = _num(s.get("num_dns_queries"), int)
    b = _num(s.get("num_blocked_filtering"), int)
    adt = now - agh_prev.get("ts", now)
    dns = {
        "queries_total": q, "blocked_total": b,
        "blocked_pct": round(100.0 * b / q, 1) if q else 0.0,
        "qps": round(rate(q, agh_prev.get("q"), adt), 1),
        "avg_ms": round(_num(s.get("avg_processing_time")) * 1000, 1),
        "malware": _num(s.get("num_replaced_safebrowsing"), int),
        "parental": _num(s.get("num_replaced_parental"), int),
        "top_queried": _toplist(s.get("top_queried_domains")),
        "top_blocked": _toplist(s.get("top_blocked_domains")),
        "top_clients": _toplist(s.get("top_clients")),
        "protection": (agh.get("status") or {}).get("protection_enabled"),
        "url": cfg.get("adguard", {}).get("url", ""),
        # AdGuard's own time-series (per time_unit) — plot it directly
        "history": s.get("dns_queries") or [],
        "history_blocked": s.get("blocked_filtering") or [],
        "time_units": s.get("time_units"),
    }
    ip2name = {l.get("ip"): l.get("name") for l in leases}
    for c in dns["top_clients"]:
        c["name"] = ip2name.get(c["name"], c["name"])
    return dns, {"q": q, "ts": now}
