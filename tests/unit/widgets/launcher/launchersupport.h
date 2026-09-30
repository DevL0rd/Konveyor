#pragma once

#include <QByteArrayView>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QStringList>
#include <QVariantMap>

namespace LauncherTest
{

inline QStringList &warnings()
{
    static QStringList list;
    return list;
}

inline QtMessageHandler &forwardTo()
{
    static QtMessageHandler handler = nullptr;
    return handler;
}

inline void collectWarning(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const bool fromQml = message.contains(QLatin1String(".qml")) || message.contains(QLatin1String(".js:"))
        || QByteArrayView(context.file).endsWith(".qml") || QByteArrayView(context.file).endsWith(".js");
    if (type != QtDebugMsg && type != QtInfoMsg && fromQml) {
        warnings().append(message);
    }
    if (!message.startsWith(QLatin1String("KLocalizedString")) && forwardTo()) {
        forwardTo()(type, context, message);
    }
}

inline void captureWarnings()
{
    warnings().clear();
    const QtMessageHandler previous = qInstallMessageHandler(collectWarning);
    if (previous != collectWarning) {
        forwardTo() = previous;
    }
}

inline bool copyTree(const QString &from, const QString &to)
{
    QDirIterator it(from, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString source = it.next();
        const QString target = QDir(to).filePath(QDir(from).relativeFilePath(source));
        if (!QDir().mkpath(QFileInfo(target).absolutePath()) || !(QFile::exists(target) || QFile::copy(source, target))) {
            return false;
        }
    }
    return true;
}

inline QString source(const char *relative)
{
    return QStringLiteral(KONVEYOR_SOURCE_DIR "/") + QLatin1String(relative);
}

inline QVariantMap response(const QString &match, const QByteArray &stdoutText, int exitCode = 0, const QString &stderrText = {})
{
    return {{QStringLiteral("match"), match}, {QStringLiteral("stdout"), QString::fromUtf8(stdoutText)},
        {QStringLiteral("exitCode"), exitCode}, {QStringLiteral("stderr"), stderrText}};
}

inline QByteArray fixture(const char *name)
{
    QFile file(source("tests/unit/widgets/launcher/fixtures/") + QLatin1String(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

}
