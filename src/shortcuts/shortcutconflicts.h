#pragma once

#include <QKeySequence>
#include <QList>
#include <QString>

namespace Konveyor
{

struct ReleasedShortcut
{
    QString component;
    QString action;
    QString componentFriendlyName;
    QString actionFriendlyName;
    QList<QKeySequence> keys;
};

class ShortcutConflicts
{
public:
    static QList<ReleasedShortcut> takeOver(const QList<QKeySequence> &wanted, const QString &ownComponent);
    static QList<ReleasedShortcut> releaseSuperseded();
    static void restore(const QList<ReleasedShortcut> &released);

    static QList<ReleasedShortcut> load();
    static void save(const QList<ReleasedShortcut> &released);
    static void merge(QList<ReleasedShortcut> &into, const QList<ReleasedShortcut> &extra);
};

}
