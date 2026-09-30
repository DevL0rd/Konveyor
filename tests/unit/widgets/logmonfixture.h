#pragma once

#include "plasmoidharness.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>

namespace Logmon
{

inline const PlasmoidSpec journal {QStringLiteral("system-log/plasmoids/org.devl0rd.logmon.journal"),
    QStringLiteral("org.devl0rd.logmon.journal"), QStringLiteral("utilities-log-viewer"), {QStringLiteral("system-log/shared/lib")}};

inline QString logPath()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/Linux-Log-Monitor/log.json");
}

inline QJsonObject lineAt(qint64 micros, int priority, const QString &ident, const QString &message)
{
    return {{QStringLiteral("t"), micros}, {QStringLiteral("p"), priority}, {QStringLiteral("id"), ident}, {QStringLiteral("u"), QString()},
        {QStringLiteral("pid"), QStringLiteral("12")}, {QStringLiteral("m"), message}};
}

inline QJsonObject line(qint64 millisAgo, int priority, const QString &ident, const QString &message)
{
    return lineAt((QDateTime::currentMSecsSinceEpoch() - millisAgo) * 1000, priority, ident, message);
}

inline QByteArray journalOf(const QJsonArray &lines, qint64 age = 0, bool alive = true)
{
    const QJsonObject object {{QStringLiteral("ts"), double(QDateTime::currentSecsSinceEpoch() - age)}, {QStringLiteral("alive"), alive},
        {QStringLiteral("lines"), lines}};
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

inline QByteArray snapshot(qint64 age = 0, bool alive = true)
{
    return journalOf({line(67000, 6, QStringLiteral("kernel"), QStringLiteral("boot ok")),
                         line(37000, 3, QStringLiteral("sshd"), QStringLiteral("Failed password\nsecond line")),
                         line(22000, 4, QStringLiteral("kwin_wayland"), QStringLiteral("slow frame")),
                         line(7000, 6, QStringLiteral("kded6"), QStringLiteral("\x1b[31mred\x1b[0m text"))},
        age, alive);
}

inline std::unique_ptr<PlasmoidHarness> started(int form, const QVariantMap &config = {})
{
    return PlasmoidHarness::started(journal, form, config);
}

inline bool feed(PlasmoidHarness &harness, const QByteArray &json = snapshot())
{
    QSignalSpy updated(harness.eval(QStringLiteral("logData")).value<QObject *>(), SIGNAL(updated()));
    return harness.deliver(logPath(), json, [&updated] { return updated.count() > 0; });
}

inline QStringList rowField(PlasmoidHarness &harness, const char *field)
{
    return harness
        .eval(QStringLiteral(
            "(function() { const out = []; for (let i = 0; i < rows.count; ++i) out.push(String(rows.get(i).%1)); return out })()")
                .arg(QLatin1String(field)))
        .toStringList();
}

}
