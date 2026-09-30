#pragma once

#include "config/loader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

namespace Konveyor::Config::Testing
{

inline LoadResult mustLoad(const QString &text)
{
    const auto result = loadString(text, QStringLiteral("config.kdl"));
    if (!result) {
        QTest::qFail(qPrintable(QStringLiteral("unexpected config error: ") + result.error().toString()), __FILE__, __LINE__);
        return LoadResult {};
    }
    return *result;
}

inline Config parsed(const QString &text)
{
    return mustLoad(text).config;
}

inline LoadError mustFail(const QString &text)
{
    const auto result = loadString(text, QStringLiteral("config.kdl"));
    if (result) {
        return LoadError {QStringLiteral("<config loaded without an error>"), Kdl::Location {}, QString(), QStringList {}};
    }
    return result.error();
}

inline QString failure(const QString &text)
{
    const LoadError error = mustFail(text);
    return QStringLiteral("%1:%2: %3").arg(error.location.line).arg(error.location.column).arg(error.message);
}

inline double proportionOf(const PresetSize &size)
{
    return std::get<Proportion>(size).value;
}

inline double fixedOf(const PresetSize &size)
{
    return std::get<Fixed>(size).value;
}

inline QString unmarked(const QString &text)
{
    return QString(text).remove(u'»');
}

inline QString expectedAt(const QString &marked, const QString &message)
{
    const qsizetype marker = marked.indexOf(u'»');
    const qsizetype lineStart = marked.lastIndexOf(u'\n', marker) + 1;
    const qsizetype line = marked.left(marker).count(u'\n') + 1;
    return QStringLiteral("%1:%2: %3").arg(line).arg(marker - lineStart + 1).arg(message);
}

inline void verifyFailure(const QString &marked, const QString &message)
{
    QCOMPARE(failure(unmarked(marked)), expectedAt(marked, message));
}

inline void verifyLoads(const QString &text)
{
    const auto result = loadString(text, QStringLiteral("config.kdl"));
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error().toString()));
}

class ConfigDir
{
public:
    QString path(const QString &name) const { return QDir(m_dir.path()).filePath(name); }

    void write(const QString &name, const QString &text) const
    {
        QVERIFY(m_dir.isValid());
        const QString full = path(name);
        QVERIFY(QDir().mkpath(QFileInfo(full).path()));
        QFile file(full);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(file.write(text.toUtf8()) >= 0);
    }

    void clear() const
    {
        QVERIFY(m_dir.isValid());
        const QDir dir(m_dir.path());
        for (const QString &entry : dir.entryList(QDir::Files)) {
            QVERIFY(QFile::remove(dir.filePath(entry)));
        }
        for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QVERIFY(QDir(dir.filePath(entry)).removeRecursively());
        }
    }

    std::expected<LoadResult, LoadError> tryLoad() const { return loadFile(path(QStringLiteral("config.kdl"))); }

    LoadResult load() const
    {
        const auto result = tryLoad();
        if (!result) {
            QTest::qFail(qPrintable(QStringLiteral("unexpected config error: ") + result.error().toString()), __FILE__, __LINE__);
            return LoadResult {};
        }
        return *result;
    }

    LoadError failure() const
    {
        const auto result = tryLoad();
        if (result) {
            QTest::qFail("config loaded without an error", __FILE__, __LINE__);
            return LoadError {};
        }
        return result.error();
    }

private:
    QTemporaryDir m_dir;
};

}
