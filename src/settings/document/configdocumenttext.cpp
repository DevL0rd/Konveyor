#include "document/configdocument.h"

#include "document/nodecodec.h"
#include "kdl/characters.h"

namespace Konveyor::Settings
{

namespace
{

bool isInlineSpace(char32_t c)
{
    return c == U' ' || c == U'\t';
}

}

qsizetype ConfigDocument::nextLineStart(qsizetype position) const
{
    while (position < size() && !Kdl::isNewline(at(position))) {
        ++position;
    }
    return std::min(position + ((position + 1 < size() && at(position) == U'\r' && at(position + 1) == U'\n') ? 2 : 1), size());
}

qsizetype ConfigDocument::previousLineStart(qsizetype start) const
{
    return lineStart(start - ((start > 1 && at(start - 1) == U'\n' && at(start - 2) == U'\r') ? 2 : 1));
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
    const QString nextLine = slice(lineEnd, nextLineStart(lineEnd)).trimmed();
    if (!nextLine.isEmpty() && !nextLine.startsWith(QLatin1Char('}'))) {
        return start;
    }
    return commentBlockStart(start);
}

qsizetype ConfigDocument::commentBlockStart(qsizetype start) const
{
    while (start > 0) {
        const qsizetype previous = previousLineStart(start);
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

}
