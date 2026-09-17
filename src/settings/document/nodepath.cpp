#include "document/nodepath.h"

#include <algorithm>

namespace Konveyor::Settings
{

std::expected<NodePath, QString> parsePath(const QString &text)
{
    NodePath path;
    if (text.isEmpty()) {
        return path;
    }
    for (const QString &part : text.split(QLatin1Char('/'))) {
        const qsizetype hash = part.lastIndexOf(QLatin1Char('#'));
        PathSegment segment {hash < 0 ? part : part.left(hash), 0};
        if (hash >= 0) {
            bool ok = false;
            segment.index = part.mid(hash + 1).toLongLong(&ok);
            if (!ok || segment.index < 0) {
                return std::unexpected(QStringLiteral("invalid index in path segment ") + part);
            }
        }
        if (segment.name.isEmpty()) {
            return std::unexpected(QStringLiteral("empty path segment in ") + text);
        }
        path.append(segment);
    }
    return path;
}

QString formatPath(const NodePath &path)
{
    QStringList parts;
    for (const PathSegment &segment : path) {
        parts.append(segment.index == 0 ? segment.name : segment.name + QLatin1Char('#') + QString::number(segment.index));
    }
    return parts.join(QLatin1Char('/'));
}

const Kdl::Node *findChild(const QList<Kdl::Node> &nodes, const PathSegment &segment)
{
    qsizetype seen = 0;
    for (const Kdl::Node &node : nodes) {
        if (node.name != segment.name) {
            continue;
        }
        if (seen == segment.index) {
            return &node;
        }
        ++seen;
    }
    return nullptr;
}

const Kdl::Node *findNode(const Kdl::Document &document, const NodePath &path)
{
    const Kdl::Node *current = nullptr;
    for (const PathSegment &segment : path) {
        current = findChild(childrenOf(document, current), segment);
        if (!current) {
            return nullptr;
        }
    }
    return current;
}

qsizetype countNamed(const QList<Kdl::Node> &nodes, const QString &name)
{
    return std::ranges::count(nodes, name, &Kdl::Node::name);
}

const QList<Kdl::Node> &childrenOf(const Kdl::Document &document, const Kdl::Node *parent)
{
    return parent ? parent->children : document.nodes;
}

}
