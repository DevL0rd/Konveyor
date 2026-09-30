#pragma once

#include "plasmoidharness.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

namespace Router
{

inline QString runtime()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/Linux-Router-Monitor");
}

inline QByteArray snapshot()
{
    return PlasmoidHarness::fixture(QStringLiteral("router.json"));
}

inline QByteArray changed(const QVariantMap &changes)
{
    QVariantMap map = QJsonDocument::fromJson(snapshot()).toVariant().toMap();
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        map.insert(it.key(), it.value());
    }
    return QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact);
}

inline PlasmoidSpec variant(const QString &id)
{
    QFile file(PlasmoidHarness::widgetsDir() + QStringLiteral("/router-monitor/plasmoids/") + id + QStringLiteral("/metadata.json"));
    const QString icon = file.open(QIODevice::ReadOnly)
        ? QJsonDocument::fromJson(file.readAll())[QLatin1String("KPlugin")][QLatin1String("Icon")].toString()
        : QString();
    return {
        QStringLiteral("router-monitor/plasmoids/org.devl0rd.routermon.panel"), id, icon, {QStringLiteral("router-monitor/shared/lib")}};
}

inline std::unique_ptr<PlasmoidHarness> started(
    int form, const QVariantMap &config = {}, const QString &id = QStringLiteral("org.devl0rd.routermon.panel"))
{
    auto harness = std::make_unique<PlasmoidHarness>(variant(id));
    if (!harness->load(form, config) || !harness->show("compactRepresentation") || !harness->show("fullRepresentation")) {
        qWarning("%s", qPrintable(harness->error));
        return {};
    }
    harness->resolveRuntime(QStringLiteral("printf %s"));
    return harness;
}

inline bool feed(PlasmoidHarness &harness, const QByteArray &json = snapshot())
{
    QObject *root = harness.root();
    const int tick = root->property("tick").toInt();
    return harness.deliver(runtime() + QStringLiteral("/data.json"), json, [root, tick] { return root->property("tick").toInt() > tick; });
}

inline bool feedState(PlasmoidHarness &harness, const QByteArray &json, const char *property, bool wanted)
{
    QObject *root = harness.root();
    return harness.deliver(runtime() + QStringLiteral("/data.json"), json, [=] { return root->property(property).toBool() == wanted; });
}

inline QVariantList clientMacs(PlasmoidHarness &harness)
{
    return harness
        .eval(QStringLiteral(
            "(function() { const out = []; for (let i = 0; i < clients.count; ++i) out.push(clients.get(i).mac); return out })()"))
        .toList();
}

inline QObject *shell(PlasmoidHarness &harness)
{
    return harness.findAll("PopupShell").value(0);
}

}
