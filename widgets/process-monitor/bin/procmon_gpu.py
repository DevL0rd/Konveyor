import json
import time
from ctypes import byref, c_uint, c_ulonglong

_nvml = {"init": False, "h": None, "vram_total": 0}


def vram_total():
    if _nvml["vram_total"]:
        return _nvml["vram_total"]
    try:
        import pynvml as N
        if not _nvml["init"]:
            N.nvmlInit()
            _nvml["h"] = N.nvmlDeviceGetHandleByIndex(0)
            _nvml["init"] = True
        _nvml["vram_total"] = N.nvmlDeviceGetMemoryInfo(_nvml["h"]).total
    except Exception:
        pass
    return _nvml["vram_total"]


_NVML_SAMPLES = 64
_NVML_PROCESSES = 64
_nvml_buffers = {}


def gpu_usage(poll_interval):
    try:
        import pynvml as N
    except Exception:
        return {}
    try:
        if not _nvml["init"]:
            N.nvmlInit()
            _nvml["h"] = N.nvmlDeviceGetHandleByIndex(0)
            _nvml["init"] = True
        h = _nvml["h"]
        since = int((time.time() - max(1.0, poll_interval + 0.5)) * 1e6)
        latest = {}
        samples = _nvml_buffers.get("samples")
        if samples is None:
            samples = _nvml_buffers["samples"] = (N.c_nvmlProcessUtilizationSample_t * _NVML_SAMPLES)()
        fn = N._nvmlGetFunctionPointer("nvmlDeviceGetProcessUtilization")
        while True:
            count = c_uint(len(samples))
            ret = fn(h, samples, byref(count), c_ulonglong(since))
            if ret != N.NVML_ERROR_INSUFFICIENT_SIZE:
                break
            samples = _nvml_buffers["samples"] = (N.c_nvmlProcessUtilizationSample_t * (count.value * 2 + 8))()
        if ret == N.NVML_SUCCESS:
            for i in range(count.value):
                su = samples[i]
                cur = latest.get(su.pid)
                if cur is None or su.timeStamp > cur[0]:
                    latest[su.pid] = (su.timeStamp, su.smUtil, su.encUtil, su.decUtil)
        res = {}
        missing = N.NVML_VALUE_NOT_AVAILABLE_ulonglong.value
        for name in ("nvmlDeviceGetComputeRunningProcesses_v3", "nvmlDeviceGetGraphicsRunningProcesses_v3"):
            procs = _nvml_buffers.get(name)
            if procs is None:
                procs = _nvml_buffers[name] = (N.c_nvmlProcessInfo_v3_t * _NVML_PROCESSES)()
            fn = N._nvmlGetFunctionPointer(name)
            while True:
                count = c_uint(len(procs))
                ret = fn(h, byref(count), procs)
                if ret != N.NVML_ERROR_INSUFFICIENT_SIZE:
                    break
                procs = _nvml_buffers[name] = (N.c_nvmlProcessInfo_v3_t * (count.value * 2 + 8))()
            N._nvmlCheckReturn(ret)
            for i in range(count.value):
                p = procs[i]
                used = p.usedGpuMemory
                s = latest.get(p.pid)
                res[p.pid] = {"gpu": s[1] if s else 0, "enc": s[2] if s else 0,
                              "dec": s[3] if s else 0, "vram": 0 if used == missing else used}
        for pid, s in latest.items():
            if pid not in res:
                res[pid] = {"gpu": s[1], "enc": s[2], "dec": s[3], "vram": 0}
        return res
    except Exception:
        return {}


_frame_telemetry = {"object": None, "retry": 0.0}


def frame_rates():
    if _frame_telemetry["object"] is None and time.monotonic() < _frame_telemetry["retry"]:
        return {}
    try:
        if _frame_telemetry["object"] is None:
            import dbus
            _frame_telemetry["object"] = dbus.SessionBus().get_object(
                "org.devl0rd.ProcessMonitor.FrameTelemetry", "/FrameTelemetry")
        raw = _frame_telemetry["object"].Frames(
            dbus_interface="org.devl0rd.ProcessMonitor.FrameTelemetry")
        frames = {}
        for item in json.loads(str(raw)) if raw else []:
            pid = int(item.get("pid", 0) or 0)
            if pid <= 0 or "fps" not in item:
                continue
            frame = {"fps": int(item["fps"]), "frametime": round(float(item.get("frametime", 0)), 2),
                     "fps_low": int(item.get("fps_low", 0))}
            if pid not in frames or frame["fps"] > frames[pid]["fps"]:
                frames[pid] = frame
        return frames
    except Exception:
        _frame_telemetry["object"] = None
        _frame_telemetry["retry"] = time.monotonic() + 2.0
        return {}
