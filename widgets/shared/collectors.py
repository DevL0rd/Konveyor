import json
import math
import os
import signal
import subprocess
import time

SERVICE = "konveyor-widgets.service"


def read_json_object(path):
    try:
        with open(path) as f:
            value = json.load(f)
    except (OSError, ValueError):
        return {}
    return value if isinstance(value, dict) else {}


def number_setting(config, path, key, default, low, report):
    value = config.get(key, default)
    try:
        number = float(value)
    except (TypeError, ValueError):
        number = math.nan
    if not math.isfinite(number):
        report(f"ignoring {key} {value!r} in {path}, using {default}")
        return default
    return max(low, number)


def set_interval(path, argument):
    try:
        seconds = float(argument)
    except (TypeError, ValueError):
        return 1
    if not math.isfinite(seconds):
        return 1
    config = read_json_object(path)
    config["poll_interval"] = max(0.25, seconds)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(config, f, indent=2)
    return 0


def write_pidfile(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        f.write(str(os.getpid()))
    os.replace(tmp, path)


def serving_pid(path, name):
    try:
        with open(path) as f:
            pid = int(f.read())
        with open(f"/proc/{pid}/cmdline", "rb") as f:
            arguments = f.read().split(b"\0")
    except (OSError, ValueError):
        return None
    if b"--serve" in arguments and any(os.path.basename(argument) == name.encode() for argument in arguments):
        return pid
    return None


def restart(path, name):
    pid = serving_pid(path, name)
    if pid is not None:
        os.kill(pid, signal.SIGTERM)
        return 0
    return subprocess.run(["systemctl", "--user", "start", SERVICE], check=False).returncode


def serve(config_path, default, tick, report):
    last = time.monotonic()
    stamp = None
    interval = default
    while True:
        try:
            current = os.stat(config_path).st_mtime_ns
        except OSError:
            current = 0
        if current != stamp:
            stamp = current
            interval = number_setting(read_json_object(config_path), config_path, "poll_interval", default, 0.25, report)
        elapsed = time.monotonic() - last
        if elapsed >= interval:
            last = time.monotonic()
            try:
                tick(interval)
            except Exception as e:
                report(e)
        else:
            time.sleep(interval - elapsed)
