#pragma once

#include "plasmoidharness.h"

#include <QTest>

namespace Procmon
{

inline const QByteArray snapshot = R"({"ts": 1, "ncpu": 8, "mem_total": 34359738368, "vram_total": 17179869184, "procs": [
 {"pid": 1, "ppid": 0, "name": "systemd", "cpu": 0.5, "ram": 10485760, "threads": 1, "acpu": 30, "aram": 3000000000},
 {"pid": 2, "ppid": 0, "name": "kthreadd", "kernel": true, "threads": 1},
 {"pid": 100, "ppid": 1, "name": "plasmashell", "icon": "plasma", "cpu": 5, "ram": 524288000, "threads": 40, "gpu": 3, "vram": 104857600,
  "acpu": 25.5, "aram": 2671771648, "athreads": 100, "agpu": 83, "avram": 4399824896},
 {"pid": 200, "ppid": 100, "name": "game", "icon": "steam", "cpu": 20.5, "ram": 2147483648, "threads": 60, "gpu": 80, "dec": 5,
  "vram": 4294967296, "disk": 1048576, "fps": 144, "frametime": 6.94, "fps_low": 120},
 {"pid": 300, "ppid": 1, "name": "it\u0027s", "cpu": 1, "ram": 1024}
]})";

inline const QByteArray panel
    = R"({"focus": {"pid": 200, "ppid": 100, "name": "game", "icon": "steam", "parentName": "plasmashell", "cpu": 20.5,
 "gpu": 80, "ram": 2147483648, "vram": 4294967296, "disk": 0, "threads": 60, "enc": 0, "dec": 5, "fps": 144, "frametime": 6.94, "fpsLow": 120,
 "framePid": 200}, "summary": {"count": 5, "cpu": 27, "gpu": 83, "vram": 4399824896, "memTotal": 34359738368, "vramTotal": 17179869184,
 "ncpu": 8, "gpuTop": {"pid": 200, "name": "game", "gpu": 80}}})";

inline const PlasmoidSpec spec {QStringLiteral("process-monitor/plasmoids/org.devl0rd.procmon.panel"),
    QStringLiteral("org.devl0rd.procmon.panel"), QStringLiteral("utilities-system-monitor"), {}};

inline QString runtime()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/Linux-Process-Mon");
}

inline std::unique_ptr<PlasmoidHarness> started(int form, const QVariantMap &config = {}, const PlasmoidSpec &which = spec)
{
    return PlasmoidHarness::started(which, form, config);
}

inline bool feed(PlasmoidHarness &harness, const QByteArray &json = snapshot)
{
    QObject *root = harness.root();
    root->setProperty("hasData", false);
    return harness.deliver(runtime() + QStringLiteral("/data.json"), json, [root] { return root->property("hasData").toBool(); });
}

inline QVariantList rowPids(PlasmoidHarness &harness)
{
    return harness
        .eval(
            QStringLiteral("(function() { const out = []; for (let i = 0; i < rows.count; ++i) out.push(rows.get(i).pid); return out })()"))
        .toList();
}

inline bool rowsBecome(PlasmoidHarness &harness, const QVariantList &pids)
{
    return QTest::qWaitFor([&] { return rowPids(harness) == pids; });
}

inline TasksModelDouble *tasks()
{
    return TasksModelDouble::instances().value(0);
}

}
