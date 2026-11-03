#include "audio/audiomatch.h"

#include <QFile>
#include <QFileInfo>

namespace Konveyor::Audio
{

namespace
{

constexpr int kDeepestProcessTree = 64;

qint64 parentOf(qint64 pid)
{
    QFile stat(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!stat.open(QIODevice::ReadOnly)) {
        return 0;
    }
    return parentPidFromStat(stat.readAll());
}

bool isHostProcessOf(qint64 pid, const QString &binary)
{
    if (pid <= 0) {
        return false;
    }
    return QFileInfo(QStringLiteral("/proc/%1/exe").arg(pid)).symLinkTarget().endsWith(QLatin1Char('/') + binary);
}

QString applicationOf(const QString &desktopFile)
{
    QString name = desktopFile.toLower();
    if (name.endsWith(QStringLiteral(".desktop"))) {
        name.chop(8);
    }
    return name;
}

bool namesMatch(const StreamIdentity &stream, const QString &desktopFile)
{
    const QString application = applicationOf(desktopFile);
    if (application.isEmpty()) {
        return false;
    }
    if (stream.portalAppId.compare(application, Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (stream.binary.isEmpty() || isHostProcessOf(stream.pid, stream.binary)) {
        return false;
    }
    const QString binary = stream.binary.toLower();
    return binary == application || binary == application.section(QLatin1Char('.'), -1);
}

}

qint64 parentPidFromStat(const QByteArray &stat)
{
    const qsizetype nameEnd = stat.lastIndexOf(')');
    if (nameEnd < 0) {
        return 0;
    }
    const QList<QByteArray> fields = stat.mid(nameEnd + 2).split(' ');
    return fields.size() > 1 ? fields.at(1).toLongLong() : 0;
}

StreamIdentity withFallback(StreamIdentity stream, const StreamIdentity &fallback)
{
    if (stream.pid <= 0) {
        stream.pid = fallback.pid;
    }
    if (stream.binary.isEmpty()) {
        stream.binary = fallback.binary;
    }
    if (stream.portalAppId.isEmpty()) {
        stream.portalAppId = fallback.portalAppId;
    }
    return stream;
}

bool processDescendsFrom(qint64 pid, qint64 ancestor, const ParentLookup &parent)
{
    qint64 current = pid;
    for (int depth = 0; current > 1 && depth < kDeepestProcessTree; ++depth) {
        if (current == ancestor) {
            return true;
        }
        current = parent(current);
    }
    return false;
}

bool processDescendsFrom(qint64 pid, qint64 ancestor)
{
    return processDescendsFrom(pid, ancestor, parentOf);
}

QList<WindowKey> windowsPlaying(const StreamIdentity &stream, const QList<WindowKey> &windows)
{
    QList<WindowKey> matches;
    for (const WindowKey &window : windows) {
        if (window.pid > 1 && stream.pid > 1 && processDescendsFrom(stream.pid, window.pid)) {
            matches.append(window);
        }
    }
    if (!matches.isEmpty()) {
        return matches;
    }
    for (const WindowKey &window : windows) {
        if (namesMatch(stream, window.desktopFile)) {
            matches.append(window);
        }
    }
    return matches;
}

}
