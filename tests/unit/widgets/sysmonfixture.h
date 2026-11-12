#pragma once

#include "plasmoidharness.h"

#include <QTest>

namespace Sysmon
{

inline const QByteArray snapshot = PlasmoidHarness::fixture(QStringLiteral("sysmon.json"));

inline const QByteArray hybridSnapshot = PlasmoidHarness::fixture(QStringLiteral("sysmon-hybrid.json"));

inline const QByteArray intelSnapshot = PlasmoidHarness::fixture(QStringLiteral("sysmon-intel.json"));

inline const QByteArray emptySnapshot = R"({"ts":0,"cpu":{},"mem":{},"gpu":null})";

inline const PlasmoidSpec panel {QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.panel"),
    QStringLiteral("org.devl0rd.sysmon.panel"), QStringLiteral("cpu"), {}};
inline const PlasmoidSpec overlayHost {QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.panel"),
    QStringLiteral("org.devl0rd.sysmon.overlay"), QStringLiteral("cpu"), {}};

inline QString dataPath()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/Linux-System-Monitor/data.json");
}

inline std::unique_ptr<PlasmoidHarness> started(int form, const QVariantMap &config = {}, const PlasmoidSpec &spec = panel)
{
    return PlasmoidHarness::started(spec, form, config);
}

inline bool feed(PlasmoidHarness &harness, const QByteArray &json)
{
    const int tick = harness.root()->property("tick").toInt();
    return harness.deliver(dataPath(), json, [&] { return harness.root()->property("tick").toInt() > tick; });
}

inline QObject *shell(PlasmoidHarness &harness)
{
    const QList<QObject *> shells = harness.findAll("PopupShell");
    return shells.isEmpty() ? nullptr : shells.first();
}

}
