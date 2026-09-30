#include "store/includeoverrides.h"

#include "document/nodecodec.h"
#include "document/nodepath.h"
#include "kdl/kdl.h"
#include "store/configfile.h"

#include <QDir>
#include <QFileInfo>

#include <functional>

namespace Konveyor::Settings
{

namespace
{

using Sets = std::function<bool(const Kdl::Document &)>;

struct Search
{
    Sets sets;
    QDir root;
    QStringList visited;
    QStringList found;
};

bool isListSection(const QString &name)
{
    static const QStringList names {QStringLiteral("output"), QStringLiteral("window-rule"), QStringLiteral("workspace"),
        QStringLiteral("monitor-profile"), QStringLiteral("include")};
    return names.contains(name);
}

QStringList ruleMatches(const Kdl::Node &rule)
{
    QStringList matches;
    for (const Kdl::Node &child : rule.children) {
        if (child.name == QLatin1String("match") || child.name == QLatin1String("exclude")) {
            matches.append(writeNode(nodeToVariant(child), QString()));
        }
    }
    matches.sort();
    return matches;
}

QString includedFile(const Kdl::Node &node, const QString &baseDir)
{
    if (node.name != QLatin1String("include") || node.arguments.isEmpty() || !node.arguments.first().isString()) {
        return QString();
    }
    QString file = node.arguments.first().toString();
    if (file == QLatin1String("~") || file.startsWith(QLatin1String("~/"))) {
        file = QDir::home().filePath(file.mid(file.size() > 1 ? 2 : 1));
    } else if (QDir::isRelativePath(file)) {
        file = QDir(baseDir).filePath(file);
    }
    return QDir::cleanPath(file);
}

void searchIncludes(const QList<Kdl::Node> &nodes, qsizetype from, const QString &baseDir, Search &search)
{
    for (qsizetype index = from; index < nodes.size(); ++index) {
        const QString file = includedFile(nodes.at(index), baseDir);
        if (file.isEmpty() || search.visited.contains(file)) {
            continue;
        }
        search.visited.append(file);
        const std::optional<QString> text = readConfigText(file);
        const auto parsed = text ? Kdl::parse(*text, file) : std::unexpected(Kdl::ParseError {});
        if (!parsed) {
            continue;
        }
        if (search.sets(*parsed)) {
            search.found.append(search.root.relativeFilePath(file));
        }
        searchIncludes(parsed->nodes, 0, QFileInfo(file).path(), search);
    }
}

qsizetype positionAfter(const QList<Kdl::Node> &nodes, const Kdl::Node *node, const QString &name)
{
    qsizetype after = 0;
    for (qsizetype index = 0; index < nodes.size(); ++index) {
        if (node ? &nodes.at(index) == node : nodes.at(index).name == name) {
            after = index + 1;
        }
    }
    return after;
}

}

QStringList includesOverriding(const QString &mainText, const QString &configPath, const QString &path)
{
    const auto parsedPath = parsePath(path);
    const auto main = Kdl::parse(mainText, configPath);
    if (!parsedPath || parsedPath->isEmpty() || !main) {
        return {};
    }
    const QString top = parsedPath->first().name;
    const Kdl::Node *rule
        = top == QLatin1String("window-rule") && parsedPath->size() > 1 ? findNode(*main, parsedPath->mid(0, 1)) : nullptr;
    if (isListSection(top) && !rule) {
        return {};
    }
    Sets sets = [&parsedPath](const Kdl::Document &document) { return findNode(document, *parsedPath) != nullptr; };
    if (rule) {
        sets = [matches = ruleMatches(*rule), setting = parsedPath->mid(1)](const Kdl::Document &document) {
            return std::ranges::any_of(document.nodes, [&](const Kdl::Node &node) {
                const Kdl::Document ruleBody {node.children};
                return node.name == QLatin1String("window-rule") && ruleMatches(node) == matches && findNode(ruleBody, setting);
            });
        };
    }
    const QString baseDir = QFileInfo(configPath).path();
    Search search {sets, QDir(baseDir), {QDir::cleanPath(configPath)}, {}};
    searchIncludes(main->nodes, positionAfter(main->nodes, rule, top), baseDir, search);
    return search.found;
}

}
