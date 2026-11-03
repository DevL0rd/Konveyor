#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QtGlobal>

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
bool processDescendsFrom(qint64 pid, qint64 ancestor);
QList<WindowKey> windowsPlaying(const StreamIdentity &stream, const QList<WindowKey> &windows);

}
