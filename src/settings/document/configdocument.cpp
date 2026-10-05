#include "document/configdocument.h"

#include "config/forceresizable.h"
#include "document/nodecodec.h"
#include "kdl/characters.h"

#include <algorithm>

namespace Konveyor::Settings
{

ConfigDocument::ConfigDocument(const QString &text)
    : m_text(toCodePoints(text))
{
    reparse();
}

QString ConfigDocument::text() const
{
    return QString::fromUcs4(m_text.data(), static_cast<qsizetype>(m_text.size()));
}

const Kdl::Node *ConfigDocument::find(const QString &path) const
{
    const auto located = locate(path);
    return located ? located->node : nullptr;
}

QVariantList ConfigDocument::childPaths(const QString &parentPath, const QString &name) const
{
    QVariantList result;
    const auto parent = parsePath(parentPath);
    if (!parent) {
        return result;
    }
    const Kdl::Node *parentNode = findNode(m_document, *parent);
    if (!parent->isEmpty() && !parentNode) {
        return result;
    }
    qsizetype index = 0;
    for (const Kdl::Node &child : childrenOf(m_document, parentNode)) {
        if (child.name != name) {
            continue;
        }
        NodePath path = *parent;
        path.append(PathSegment {name, index++});
        result.append(QVariantMap {{QStringLiteral("path"), formatPath(path)}, {QStringLiteral("node"), nodeToVariant(child)}});
    }
    return result;
}

EditResult ConfigDocument::setNode(const QString &path, const QVariantMap &node)
{
    auto located = locate(path);
    if (!located) {
        return std::unexpected(located.error());
    }
    if (located->path.isEmpty()) {
        return std::unexpected(QStringLiteral("cannot replace the whole document"));
    }
    if (!located->node) {
        const NodePath parent = parentOf(located->path);
        if (const EditResult ensured = ensure(parent); !ensured) {
            return ensured;
        }
        const Kdl::Node *parentNode = findNode(m_document, parent);
        if (located->path.last().index != countNamed(childrenOf(m_document, parentNode), located->path.last().name)) {
            return std::unexpected(QStringLiteral("cannot create ") + path + QStringLiteral(": earlier siblings are missing"));
        }
        return insertChild(parent, node);
    }
    const Kdl::Span &span = located->node->span;
    const QVariantMap ordered = withPropertyOrder(node, *located->node);
    const bool hadChildren = !located->node->children.isEmpty();
    const bool wantsChildren = !ordered.value(QStringLiteral("children")).toList().isEmpty();
    if (span.childrenOpen >= 0 && ordered.contains(QStringLiteral("children")) && hadChildren == wantsChildren) {
        return updateInPlace(located->path, ordered);
    }
    std::u32string text = m_text;
    if (ordered.contains(QStringLiteral("children"))) {
        replace(text, span.start, span.end, writeNode(ordered, indentFor(located->path)));
    } else if (span.childrenOpen >= 0) {
        replace(text, span.start, span.childrenOpen, writeNodeHead(ordered) + QLatin1Char(' '));
    } else {
        replace(text, span.start, span.end, writeNodeHead(ordered));
    }
    return commit(std::move(text), path);
}

EditResult ConfigDocument::updateInPlace(const NodePath &path, const QVariantMap &node)
{
    const std::u32string originalText = m_text;
    const Kdl::Document originalDocument = m_document;
    const Kdl::Node *existing = findNode(m_document, path);
    QHash<QString, qsizetype> existingCounts;
    for (const Kdl::Node &child : existing->children) {
        ++existingCounts[child.name];
    }
    const Kdl::Span &span = existing->span;
    std::u32string text = m_text;
    replace(text, span.start, span.childrenOpen, writeNodeHead(node) + QLatin1Char(' '));
    EditResult result = commit(std::move(text), formatPath(path));
    QHash<QString, qsizetype> seen;
    QStringList names;
    for (const QVariant &child : node.value(QStringLiteral("children")).toList()) {
        const QVariantMap childNode = child.toMap();
        const QString name = childNode.value(QStringLiteral("name")).toString();
        names.append(name);
        if (result) {
            result = setNode(formatPath(path + NodePath {PathSegment {name, seen[name]++}}), childNode);
        }
    }
    for (auto count = existingCounts.constBegin(); result && count != existingCounts.constEnd(); ++count) {
        for (qsizetype index = count.value() - 1; result && index >= seen.value(count.key()); --index) {
            result = remove(formatPath(path + NodePath {PathSegment {count.key(), index}}));
        }
    }
    if (result) {
        result = orderChildren(path, names);
    }
    if (!result) {
        m_text = originalText;
        m_document = originalDocument;
        return result;
    }
    return formatPath(path);
}

EditResult ConfigDocument::orderChildren(const NodePath &path, const QStringList &names)
{
    const QList<Kdl::Node> &children = findNode(m_document, path)->children;
    QHash<QString, QList<qsizetype>> byName;
    for (qsizetype index = 0; index < children.size(); ++index) {
        byName[children.at(index).name].append(index);
    }
    QHash<QString, qsizetype> taken;
    QList<qsizetype> order;
    for (const QString &name : names) {
        order.append(byName.value(name).at(taken[name]++));
    }
    if (std::ranges::is_sorted(order)) {
        return formatPath(path);
    }
    QList<std::pair<qsizetype, qsizetype>> owned;
    QStringList texts;
    for (const Kdl::Node &child : children) {
        owned.append(ownedRange(child.span));
    }
    for (const qsizetype index : std::as_const(order)) {
        texts.append(slice(owned.at(index).first, owned.at(index).second));
    }
    std::u32string text = m_text;
    for (qsizetype slot = owned.size() - 1; slot >= 0; --slot) {
        replace(text, owned.at(slot).first, owned.at(slot).second, texts.at(slot));
    }
    return commit(std::move(text), formatPath(path));
}

EditResult ConfigDocument::remove(const QString &path)
{
    auto located = locate(path);
    if (!located) {
        return std::unexpected(located.error());
    }
    if (!located->node) {
        return QString();
    }
    const Kdl::Span &span = located->node->span;
    const qsizetype end = entryEnd(span.end);
    std::u32string text = m_text;
    const qsizetype start = lineStart(span.start);
    const bool ownsLine = slice(start, span.start).trimmed().isEmpty() && (end >= size() || Kdl::isNewline(at(end)));
    if (!ownsLine) {
        replace(text, span.start, end, QString());
        return commit(std::move(text), QString());
    }
    const qsizetype lineEnd = nextLineStart(end);
    const qsizetype from = attachedCommentStart(start, lineEnd);
    const qsizetype following = nextLineStart(lineEnd);
    const QString above = from > 0 ? slice(previousLineStart(from), from).trimmed() : QString();
    const bool blankAround
        = lineEnd < size() && slice(lineEnd, following).trimmed().isEmpty() && (above.isEmpty() || above.endsWith(QLatin1Char('{')));
    replace(text, from, blankAround ? following : lineEnd, QString());
    return commit(std::move(text), QString());
}

EditResult ConfigDocument::append(const QString &parentPath, const QVariantMap &node)
{
    const auto parent = locate(parentPath);
    if (!parent) {
        return std::unexpected(parent.error());
    }
    if (const EditResult ensured = ensure(parent->path); !ensured) {
        return ensured;
    }
    return insertChild(parent->path, node);
}

EditResult ConfigDocument::move(const QString &path, int delta)
{
    auto located = locate(path);
    if (!located) {
        return std::unexpected(located.error());
    }
    if (!located->node) {
        return std::unexpected(QStringLiteral("nothing to move at ") + path);
    }
    NodePath target = located->path;
    target.last().index += delta;
    const Kdl::Node *other = target.last().index < 0 ? nullptr : findNode(m_document, target);
    if (!other) {
        return std::unexpected(QStringLiteral("cannot move ") + path + QStringLiteral(" further"));
    }
    const Kdl::Node *first = delta < 0 ? other : located->node;
    const Kdl::Node *second = delta < 0 ? located->node : other;
    const auto [firstStart, firstEnd] = ownedRange(first->span);
    const auto [secondStart, secondEnd] = ownedRange(second->span);
    const QString firstText = slice(firstStart, firstEnd);
    const QString secondText = slice(secondStart, secondEnd);
    std::u32string text = m_text;
    replace(text, secondStart, secondEnd, firstText);
    replace(text, firstStart, firstEnd, secondText);
    return commit(std::move(text), formatPath(target));
}

std::expected<ConfigDocument::Located, QString> ConfigDocument::locate(const QString &path) const
{
    if (!m_parseError.isEmpty()) {
        return std::unexpected(m_parseError);
    }
    const auto parsed = parsePath(path);
    if (!parsed) {
        return std::unexpected(parsed.error());
    }
    return Located {*parsed, parsed->isEmpty() ? nullptr : findNode(m_document, *parsed)};
}

EditResult ConfigDocument::ensure(const NodePath &path)
{
    if (path.isEmpty() || findNode(m_document, path)) {
        return formatPath(path);
    }
    const NodePath parent = parentOf(path);
    if (const EditResult ensured = ensure(parent); !ensured) {
        return ensured;
    }
    if (path.last().index != countNamed(childrenOf(m_document, findNode(m_document, parent)), path.last().name)) {
        return std::unexpected(QStringLiteral("cannot create ") + formatPath(path) + QStringLiteral(": earlier siblings are missing"));
    }
    return insertChild(parent, QVariantMap {{QStringLiteral("name"), path.last().name}, {QStringLiteral("children"), QVariantList()}});
}

EditResult ConfigDocument::insertChild(const NodePath &parentPath, const QVariantMap &node)
{
    const Kdl::Node *parent = findNode(m_document, parentPath);
    const QString name = node.value(QStringLiteral("name")).toString();
    NodePath resultPath = parentPath;
    resultPath.append(PathSegment {name, countNamed(childrenOf(m_document, parent), name)});
    if (!parent) {
        return insertTopLevel(node, formatPath(resultPath));
    }
    std::u32string text = m_text;
    const Kdl::Span &span = parent->span;
    const QString parentIndent = indentFor(parentPath);
    const QString indent = parentIndent + IndentStep;
    const QString nodeText = writeNode(node, indent);
    if (span.childrenOpen < 0) {
        replace(text, span.end, span.end, QStringLiteral(" {\n") + indent + nodeText + QLatin1Char('\n') + parentIndent + QLatin1Char('}'));
    } else if (slice(span.childrenOpen, span.childrenClose).contains(QLatin1Char('\n'))) {
        const qsizetype closeLine = lineStart(span.childrenClose);
        if (slice(closeLine, span.childrenClose).trimmed().isEmpty()) {
            replace(text, closeLine, closeLine, indent + nodeText + QLatin1Char('\n'));
        } else {
            replace(text, span.childrenClose, span.childrenClose, QLatin1Char('\n') + indent + nodeText + QLatin1Char('\n') + parentIndent);
        }
    } else {
        QString block = QStringLiteral("{\n");
        for (const Kdl::Node &child : parent->children) {
            block += indent + slice(child.span.start, child.span.end) + QLatin1Char('\n');
        }
        block += indent + nodeText + QLatin1Char('\n') + parentIndent + QLatin1Char('}');
        replace(text, span.childrenOpen, span.childrenClose + 1, block);
    }
    return commit(std::move(text), formatPath(resultPath));
}

EditResult ConfigDocument::insertTopLevel(const QVariantMap &node, const QString &resultPath)
{
    std::u32string text = m_text;
    if (!m_document.nodes.isEmpty() && Config::isForceResizableInclude(m_document.nodes.last())) {
        const qsizetype includeStart = m_document.nodes.last().span.start;
        const qsizetype line = lineStart(includeStart);
        const bool ownsLine = slice(line, includeStart).trimmed().isEmpty();
        const qsizetype position = ownsLine ? line : includeStart;
        replace(text, position, position, writeNode(node, QString()) + (ownsLine ? QStringLiteral("\n\n") : QStringLiteral("\n")));
        return commit(std::move(text), resultPath);
    }
    QString prefix;
    if (!m_text.empty()) {
        prefix = m_text.back() == U'\n' ? QStringLiteral("\n") : QStringLiteral("\n\n");
    }
    replace(text, size(), size(), prefix + writeNode(node, QString()) + QLatin1Char('\n'));
    return commit(std::move(text), resultPath);
}

EditResult ConfigDocument::commit(std::u32string text, const QString &resultPath)
{
    const QString candidate = QString::fromUcs4(text.data(), static_cast<qsizetype>(text.size()));
    auto parsed = Kdl::parse(candidate, QStringLiteral("config.kdl"));
    if (!parsed) {
        return std::unexpected(QStringLiteral("edit produced invalid KDL: ") + parsed.error().toString());
    }
    m_text = std::move(text);
    m_document = std::move(*parsed);
    return resultPath;
}

void ConfigDocument::reparse()
{
    auto parsed = Kdl::parse(text(), QStringLiteral("config.kdl"));
    if (!parsed) {
        m_parseError = parsed.error().toString();
        m_document = {};
        return;
    }
    m_parseError.clear();
    m_document = std::move(*parsed);
}

}
