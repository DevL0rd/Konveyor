#include "config/loader.h"

#include "config/sections.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>

namespace Konveyor::Config
{

namespace
{

Paint solidPaint(int red, int green, int blue, int alpha)
{
    return Paint {ColorSource::Explicit, QColor(red, green, blue, alpha), std::nullopt};
}

QString sourceLineOf(const LoadContext &context, const Kdl::Location &location)
{
    const auto text = context.texts.constFind(location.file);
    if (text == context.texts.constEnd() || location.line <= 0) {
        return {};
    }
    const QStringList lines = text->split(QLatin1Char('\n'));
    if (location.line > lines.size()) {
        return {};
    }
    QString line = lines.at(location.line - 1);
    if (line.endsWith(QLatin1Char('\r'))) {
        line.chop(1);
    }
    return line;
}

void warnAboutHiddenWindowSwipes(LoadContext &context)
{
    const QList<std::pair<QString, const MultiTouch *>> devices {{QStringLiteral("touchpad"), &context.config.gestures.touchpad},
        {QStringLiteral("touchscreen"), &context.config.gestures.touchscreen}};
    for (const auto &[name, touch] : devices) {
        const bool rowSwipes = touch->horizontalSwipe != HorizontalSwipe::Off || touch->verticalSwipe != VerticalSwipe::Off;
        const bool windowSwipes
            = touch->windowHorizontalSwipe != WindowHorizontalSwipe::Off || touch->windowVerticalSwipe != WindowVerticalSwipe::Off;
        if (touch->enabled && rowSwipes && windowSwipes && touch->swipeFingers == touch->windowSwipeFingers) {
            context.warnings.append(QStringLiteral("%1 swipe-fingers and window-swipe-fingers are both %2, so the window swipes never "
                                                   "start; give them different finger counts")
                    .arg(name)
                    .arg(touch->swipeFingers));
        }
    }
}

std::expected<LoadResult, LoadError> runLoad(LoadContext &context, const QString &text, const QString &name, const QString &baseDir)
{
    context.config = defaultConfig();
    context.rootDir = baseDir;
    context.texts.insert(name, text);
    const auto document = Kdl::parse(text, name);
    if (!document) {
        const Kdl::ParseError &error = document.error();
        return std::unexpected(LoadError {error.message, error.location, sourceLineOf(context, error.location), context.files});
    }
    try {
        processDocument(context, *document, baseDir, QStringList {QDir(baseDir).filePath(name)});
        resolveBinds(context);
        warnAboutHiddenWindowSwipes(context);
    } catch (const DecodeError &error) {
        return std::unexpected(LoadError {error.message, error.location, sourceLineOf(context, error.location), context.files});
    }
    return LoadResult {context.config, context.files, context.warnings};
}

}

QString LoadError::toString() const
{
    QString result = QStringLiteral("%1:%2:%3: %4").arg(location.file).arg(location.line).arg(location.column).arg(message);
    if (!sourceLine.isEmpty()) {
        result += QLatin1Char('\n');
        result += sourceLine;
    }
    return result;
}

Config defaultConfig()
{
    Config config;
    config.layout.focusRing.enabled = true;
    config.layout.focusRing.active = solidPaint(127, 200, 255, 255);
    config.layout.focusRing.inactive = solidPaint(80, 80, 80, 255);
    config.layout.focusRing.urgent = solidPaint(155, 0, 0, 255);
    config.layout.border.enabled = false;
    config.layout.border.active = solidPaint(255, 200, 127, 255);
    config.layout.border.inactive = solidPaint(80, 80, 80, 255);
    config.layout.border.urgent = solidPaint(155, 0, 0, 255);
    config.layout.insertHint.paint = solidPaint(127, 200, 255, 128);
    config.layout.presetColumnWidths = defaultColumnWidthPresets();
    config.layout.presetWindowHeights = defaultWindowHeightPresets();
    config.animations = defaultAnimations();
    return config;
}

std::expected<LoadResult, LoadError> loadString(const QString &text, const QString &fileName)
{
    const QFileInfo info(fileName);
    const QString name = info.fileName().isEmpty() ? QStringLiteral("config.kdl") : info.fileName();
    LoadContext context;
    context.files.append(fileName);
    return runLoad(context, text, name, info.path());
}

std::expected<LoadResult, LoadError> loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unexpected(LoadError {QStringLiteral("error reading %1: %2").arg(path, file.errorString()), Kdl::Location {path, 0, 0},
            QString(), QStringList {path}});
    }
    const QString text = QString::fromUtf8(file.readAll());
    const QFileInfo info(path);
    LoadContext context;
    context.files.append(info.absoluteFilePath());
    return runLoad(context, text, info.fileName(), info.absolutePath());
}

QString configPath()
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QString explicitPath = environment.value(QStringLiteral("KONVEYOR_CONFIG"));
    if (!explicitPath.isEmpty()) {
        return explicitPath;
    }
    const QString configHome = environment.value(QStringLiteral("XDG_CONFIG_HOME"));
    const QString base = configHome.isEmpty() ? QDir(QDir::homePath()).filePath(QStringLiteral(".config")) : configHome;
    return QDir(base).filePath(QStringLiteral("konveyor/config.kdl"));
}

}
