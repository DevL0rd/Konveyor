#include "config/loader.h"
#include "config/sections.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace Konveyor::Config
{

namespace
{

constexpr int kRecursionLimit = 10;

const QSet<QString> &multipartSections()
{
    static const QSet<QString> names {QStringLiteral("output"), QStringLiteral("window-rule"), QStringLiteral("workspace"),
        QStringLiteral("include"), QStringLiteral("monitor-profile")};
    return names;
}

QString expandPath(const Kdl::Node &node, const QString &raw, const QString &baseDir)
{
    QString path = raw;
    if (path == QLatin1String("~") || path.startsWith(QLatin1String("~/"))) {
        const QString home = QDir::homePath();
        if (home.isEmpty()) {
            fail(node, QStringLiteral("error retrieving home directory to expand ") + raw);
        }
        path = QDir(home).filePath(path.mid(path.size() > 1 ? 2 : 1));
    } else if (QDir::isRelativePath(path)) {
        path = QDir(baseDir).filePath(path);
    }
    return QDir::cleanPath(path);
}

QString displayName(const LoadContext &context, const QString &path)
{
    const QString prefix = context.rootDir.endsWith(QLatin1Char('/')) ? context.rootDir : context.rootDir + QLatin1Char('/');
    if (!context.rootDir.isEmpty() && path.startsWith(prefix)) {
        return path.mid(prefix.size());
    }
    return path;
}

bool includeOptional(const Kdl::Node &node)
{
    bool optional = false;
    ValueTable table;
    table.insert(QStringLiteral("optional"), [&optional](const Kdl::Value &value) { optional = toBoolean(value); });
    decodeProperties(node, table);
    return optional;
}

void addSimpleHandlers(NodeTable &table, LoadContext &context)
{
    Config &config = context.config;
    table.insert(QStringLiteral("input"), [&config](const Kdl::Node &node) { decodeInput(node, config.input); });
    table.insert(QStringLiteral("animations"), [&config](const Kdl::Node &node) { decodeAnimations(node, config.animations); });
    table.insert(QStringLiteral("gestures"), [&config](const Kdl::Node &node) { decodeGestures(node, config.gestures); });
    table.insert(
        QStringLiteral("hide-desktop-widgets"), [&config](const Kdl::Node &node) { config.hideDesktopWidgets = flagArgument(node); });
    table.insert(
        QStringLiteral("fill-panels-on-maximize"), [&config](const Kdl::Node &node) { config.fillPanelsOnMaximize = flagArgument(node); });
    table.insert(QStringLiteral("disable-minimize"), [&config](const Kdl::Node &node) { config.disableMinimize = flagArgument(node); });
    table.insert(QStringLiteral("experiments"), [&config](const Kdl::Node &node) {
        expectNoArguments(node);
        NodeTable inner;
        inner.insert(QStringLiteral("prevent-fullscreen-minimize"),
            [&config](const Kdl::Node &child) { config.experiments.preventFullscreenMinimize = flagArgument(child); });
        inner.insert(QStringLiteral("prevent-fullscreen-exit"),
            [&config](const Kdl::Node &child) { config.experiments.preventFullscreenExit = flagArgument(child); });
        decodeChildren(node, inner);
    });
    table.insert(QStringLiteral("config-notification"), [&config](const Kdl::Node &node) {
        expectOnlyChildren(node);
        NodeTable inner;
        inner.insert(QStringLiteral("disable-failed"),
            [&config](const Kdl::Node &child) { config.configNotificationDisableFailed = flagArgument(child); });
        decodeChildren(node, inner);
    });
    table.insert(QStringLiteral("output"), [&context](const Kdl::Node &node) { decodeOutput(context, node); });
    table.insert(QStringLiteral("monitor-profile"), [&context](const Kdl::Node &node) { decodeMonitorProfile(context, node); });
    table.insert(QStringLiteral("workspace"), [&context](const Kdl::Node &node) { decodeWorkspace(context, node); });
    table.insert(QStringLiteral("window-rule"), [&config](const Kdl::Node &node) { config.windowRules.append(decodeWindowRule(node)); });
    table.insert(QStringLiteral("binds"), [&context](const Kdl::Node &node) { decodeBinds(context, node); });
}

}

void processNode(LoadContext &context, const Kdl::Node &node, const QString &baseDir, const QStringList &stack)
{
    NodeTable table;
    addSimpleHandlers(table, context);
    table.insert(QStringLiteral("layout"), [&context](const Kdl::Node &child) {
        const LayoutPart part = decodeLayoutPart(child, context.recursion == 0);
        context.config.layout = mergedLayout(context.config.layout, part);
    });
    table.insert(QStringLiteral("include"),
        [&context, &baseDir, &stack](const Kdl::Node &child) { processInclude(context, child, baseDir, stack); });
    const auto handler = table.constFind(node.name);
    if (handler == table.constEnd()) {
        fail(node, QStringLiteral("unexpected node ") + quoteName(node.name));
    }
    (*handler)(node);
}

void processDocument(LoadContext &context, const Kdl::Document &document, const QString &baseDir, const QStringList &stack)
{
    QSet<QString> seen;
    for (const Kdl::Node &node : document.nodes) {
        if (!multipartSections().contains(node.name) && seen.contains(node.name)) {
            fail(node, QStringLiteral("duplicate node ") + quoteName(node.name) + QStringLiteral(", single node expected"));
        }
        seen.insert(node.name);
        processNode(context, node, baseDir, stack);
    }
}

void processInclude(LoadContext &context, const Kdl::Node &node, const QString &baseDir, const QStringList &stack)
{
    expectNoChildren(node);
    expectArgumentLimit(node, 1);
    const QString raw = toText(requiredArgument(node, QStringLiteral("include path")));
    const bool optional = includeOptional(node);
    const QString path = expandPath(node, raw, baseDir);
    if (context.recursion + 1 >= kRecursionLimit) {
        fail(node, QStringLiteral("reached the recursion limit; includes cannot be %1 levels deep").arg(kRecursionLimit));
    }
    if (stack.contains(path)) {
        fail(node, QStringLiteral("recursive include (file includes itself)"));
    }
    context.files.append(path);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (optional && !file.exists()) {
            context.warnings.append(QStringLiteral("optional include not found: ") + path);
            return;
        }
        fail(node, QStringLiteral("failed to read included config from %1: %2").arg(path, file.errorString()));
    }
    const QString text = QString::fromUtf8(file.readAll());
    const QString name = displayName(context, path);
    context.texts.insert(name, text);
    const auto document = Kdl::parse(text, name);
    if (!document) {
        failAt(document.error().location, document.error().message);
    }
    const int previous = context.recursion;
    context.recursion = previous + 1;
    processDocument(context, *document, QFileInfo(path).path(), stack + QStringList {path});
    context.recursion = previous;
}

}
