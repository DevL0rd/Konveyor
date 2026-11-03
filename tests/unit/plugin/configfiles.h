#pragma once

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QString>

namespace Konveyor::Test
{

inline QString gapsConfig(int gaps)
{
    return QStringLiteral("layout {\n    gaps %1\n}\n").arg(gaps);
}

inline bool writeInPlace(const QString &path, const QString &text)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) && file.write(text.toUtf8()) >= 0;
}

inline bool replaceByRename(const QString &path, const QString &text)
{
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Text) && file.write(text.toUtf8()) >= 0 && file.commit();
}

inline void isolate(const QString &root)
{
    qputenv("XDG_STATE_HOME", QDir(root).filePath(QStringLiteral("state")).toUtf8());
    qputenv("XDG_DATA_HOME", QDir(root).filePath(QStringLiteral("data")).toUtf8());
    qputenv("XDG_DATA_DIRS", QDir(root).filePath(QStringLiteral("system")).toUtf8());
}

}
