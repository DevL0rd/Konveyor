#include "document/configdocument.h"

#include "document/nodecodec.h"
#include "kdl/characters.h"

namespace Konveyor::Settings
{

namespace
{

constexpr QLatin1StringView IndentStep("    ");

bool isInlineSpace(char32_t c)
{
    return c == U' ' || c == U'\t';
}

NodePath parentOf(const NodePath &path)
{
    return path.mid(0, path.size() - 1);
}

}

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
    const qsizetype lineEnd = std::min(end + ((end + 1 < size() && at(end) == U'\r' && at(end + 1) == U'\n') ? 2 : 1), size());
    replace(text, attachedCommentStart(start, lineEnd), lineEnd, QString());
    return commit(std::move(text), QString());
}

qsizetype ConfigDocument::size() const
{
    return static_cast<qsizetype>(m_text.size());
}

char32_t ConfigDocument::at(qsizetype index) const
{
    return m_text[static_cast<std::size_t>(index)];
}

qsizetype ConfigDocument::entryEnd(qsizetype position) const
{
    const auto skipSpaces = [this](qsizetype index) {
        while (index < size() && isInlineSpace(at(index))) {
            ++index;
        }
        return index;
    };
    qsizetype end = skipSpaces(position);
    if (end < size() && at(end) == U';') {
        end = skipSpaces(end + 1);
    }
    if (end + 1 < size() && at(end) == U'/' && at(end + 1) == U'/') {
        while (end < size() && !Kdl::isNewline(at(end))) {
            ++end;
        }
    }
    return end;
}

qsizetype ConfigDocument::attachedCommentStart(qsizetype start, qsizetype lineEnd) const
{
    qsizetype nextLineEnd = lineEnd;
    while (nextLineEnd < size() && !Kdl::isNewline(at(nextLineEnd))) {
        ++nextLineEnd;
    }
    const QString nextLine = slice(lineEnd, nextLineEnd).trimmed();
    if (!nextLine.isEmpty() && !nextLine.startsWith(QLatin1Char('}'))) {
        return start;
    }
    return commentBlockStart(start);
}

qsizetype ConfigDocument::commentBlockStart(qsizetype start) const
{
    while (start > 0) {
        const qsizetype previous = lineStart(start - ((start > 1 && at(start - 1) == U'\n' && at(start - 2) == U'\r') ? 2 : 1));
        if (!slice(previous, start).trimmed().startsWith(QLatin1String("//"))) {
            break;
        }
        start = previous;
    }
    return start;
}

std::pair<qsizetype, qsizetype> ConfigDocument::ownedRange(const Kdl::Span &span) const
{
    const qsizetype line = lineStart(span.start);
    if (!slice(line, span.start).trimmed().isEmpty()) {
        return {span.start, span.end};
    }
    qsizetype start = commentBlockStart(line);
    while (isInlineSpace(at(start))) {
        ++start;
    }
    const qsizetype end = entryEnd(span.end);
    const bool trailingComment = (end >= size() || Kdl::isNewline(at(end))) && !slice(span.end, end).contains(QLatin1Char(';'));
    return {start, trailingComment ? end : span.end};
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
    std::u32string text = m_text;
    if (!parent) {
        QString prefix;
        if (!m_text.empty()) {
            prefix = m_text.back() == U'\n' ? QStringLiteral("\n") : QStringLiteral("\n\n");
        }
        replace(text, static_cast<qsizetype>(m_text.size()), static_cast<qsizetype>(m_text.size()),
            prefix + writeNode(node, QString()) + QLatin1Char('\n'));
        return commit(std::move(text), formatPath(resultPath));
    }
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

QString ConfigDocument::slice(qsizetype from, qsizetype to) const
{
    return QString::fromUcs4(m_text.data() + from, to - from);
}

QString ConfigDocument::indentAt(qsizetype position) const
{
    const qsizetype start = lineStart(position);
    qsizetype end = start;
    while (end < position && isInlineSpace(m_text[static_cast<std::size_t>(end)])) {
        ++end;
    }
    return slice(start, end);
}

qsizetype ConfigDocument::lineStart(qsizetype position) const
{
    while (position > 0 && !Kdl::isNewline(m_text[static_cast<std::size_t>(position - 1)])) {
        --position;
    }
    return position;
}

QString ConfigDocument::indentFor(const NodePath &path) const
{
    const Kdl::Node *node = findNode(m_document, path);
    const qsizetype start = lineStart(node->span.start);
    if (path.size() > 1 && !slice(start, node->span.start).trimmed().isEmpty()) {
        return indentFor(parentOf(path)) + IndentStep;
    }
    return indentAt(node->span.start);
}

bool ConfigDocument::usesCrlf() const
{
    const auto newline = m_text.find(U'\n');
    return newline != std::u32string::npos && newline > 0 && m_text[newline - 1] == U'\r';
}

void ConfigDocument::replace(std::u32string &text, qsizetype from, qsizetype to, const QString &replacement) const
{
    const QString written = usesCrlf()
        ? QString(replacement).replace(QLatin1String("\r\n"), QLatin1String("\n")).replace(QLatin1Char('\n'), QLatin1String("\r\n"))
        : replacement;
    text.replace(static_cast<std::size_t>(from), static_cast<std::size_t>(to - from), toCodePoints(written));
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
