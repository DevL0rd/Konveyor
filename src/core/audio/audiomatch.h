#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <functional>

namespace Konveyor::Audio
{

struct WindowKey
{
    qint64 pid = 0;
    QString desktopFile;

    bool operator==(const WindowKey &other) const = default;
};

inline size_t qHash(const WindowKey &key, size_t seed = 0)
{
    return qHashMulti(seed, key.pid, key.desktopFile);
}

struct StreamIdentity
{
    qint64 pid = 0;
    QString binary;
    QString portalAppId;
};

StreamIdentity withFallback(StreamIdentity stream, const StreamIdentity &fallback);
qint64 parentPidFromStat(const QByteArray &stat);
using ParentLookup = std::function<qint64(qint64)>;

bool processDescendsFrom(qint64 pid, qint64 ancestor, const ParentLookup &parent);
bool processDescendsFrom(qint64 pid, qint64 ancestor);
QList<WindowKey> windowsPlaying(const StreamIdentity &stream, const QList<WindowKey> &windows);

}
