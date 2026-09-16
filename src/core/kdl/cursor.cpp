#include "kdl/cursor.h"

#include "kdl/characters.h"

namespace Konveyor::Kdl
{

Cursor::Cursor(const QString &text, const QString &fileName)
    : m_fileName(fileName)
{
    const QList<uint> codePoints = text.toUcs4();
    m_text.reserve(static_cast<std::size_t>(codePoints.size()));
    for (const uint codePoint : codePoints) {
        m_text.push_back(static_cast<char32_t>(codePoint));
    }
}

bool Cursor::atEnd() const
{
    return m_position >= static_cast<qsizetype>(m_text.size());
}

char32_t Cursor::peek(qsizetype offset) const
{
    const qsizetype index = m_position + offset;
    if (index < 0 || index >= static_cast<qsizetype>(m_text.size())) {
        return endOfInput;
    }
    return m_text[static_cast<std::size_t>(index)];
}

bool Cursor::lookingAt(std::u32string_view sequence) const
{
    const std::u32string_view rest(m_text.data() + m_position, m_text.size() - static_cast<std::size_t>(m_position));
    return rest.starts_with(sequence);
}

qsizetype Cursor::position() const
{
    return m_position;
}

std::u32string_view Cursor::slice(qsizetype from) const
{
    return std::u32string_view(m_text.data() + from, static_cast<std::size_t>(m_position - from));
}

Location Cursor::location() const
{
    return Location {m_fileName, m_line, m_column};
}

Cursor::Mark Cursor::mark() const
{
    return Mark {m_position, m_line, m_column};
}

void Cursor::reset(const Mark &mark)
{
    m_position = mark.position;
    m_line = mark.line;
    m_column = mark.column;
}

void Cursor::advance(qsizetype count)
{
    for (qsizetype i = 0; i < count && !atEnd(); ++i) {
        step();
    }
}

void Cursor::step()
{
    const char32_t current = peek();
    ++m_position;
    if (isNewline(current) && (current != U'\r' || peek() != U'\n')) {
        ++m_line;
        m_column = 1;
        return;
    }
    ++m_column;
}

}
