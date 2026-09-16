#include "kdl/lexer.h"

#include "kdl/characters.h"
#include "kdl/syntaxerror.h"

#include <optional>
#include <string>

namespace Konveyor::Kdl
{

namespace
{

constexpr int maxUnicodeEscapeDigits = 6;

std::optional<char32_t> simpleEscape(char32_t c)
{
    switch (c) {
    case U'n':
        return U'\n';
    case U'r':
        return U'\r';
    case U't':
        return U'\t';
    case U'b':
        return U'\b';
    case U'f':
        return U'\f';
    case U'\\':
    case U'/':
    case U'"':
        return c;
    default:
        return std::nullopt;
    }
}

}

Lexer::Lexer(Cursor &cursor)
    : m_cursor(cursor)
{ }

bool Lexer::skipNodeSpace()
{
    bool skipped = false;
    while (skipWhitespace() || skipLineContinuation()) {
        skipped = true;
    }
    return skipped;
}

void Lexer::skipLineSpace()
{
    while (skipWhitespace() || consumeNewline() || skipLineComment()) { }
}

bool Lexer::consumeNodeTerminator()
{
    if (m_cursor.atEnd()) {
        return true;
    }
    if (m_cursor.peek() == U';') {
        m_cursor.advance();
        return true;
    }
    return consumeNewline() || skipLineComment();
}

bool Lexer::consumeSlashdash()
{
    if (!m_cursor.lookingAt(U"/-")) {
        return false;
    }
    m_cursor.advance(2);
    skipNodeSpace();
    return true;
}

Word Lexer::readWord(const QString &expected)
{
    const Location start = m_cursor.location();
    if (m_cursor.peek() == U'"') {
        return Word {readQuotedString(), {}, start, true};
    }
    if (atRawString()) {
        return Word {readRawString(), {}, start, true};
    }
    return readBareWord(expected);
}

void Lexer::failUnexpected(const QString &expected) const
{
    raiseSyntaxError(QStringLiteral("expected %1, found %2").arg(expected, describeCharacter(m_cursor.peek())), m_cursor.location());
}

bool Lexer::skipWhitespace()
{
    if (isUnicodeSpace(m_cursor.peek())) {
        m_cursor.advance();
        return true;
    }
    return skipBlockComment();
}

bool Lexer::skipBlockComment()
{
    if (!m_cursor.lookingAt(U"/*")) {
        return false;
    }
    const Location start = m_cursor.location();
    m_cursor.advance(2);
    int depth = 1;
    while (depth > 0) {
        if (m_cursor.atEnd()) {
            raiseSyntaxError(QStringLiteral("unclosed block comment"), start);
        }
        depth += stepInsideBlockComment();
    }
    return true;
}

int Lexer::stepInsideBlockComment()
{
    if (m_cursor.lookingAt(U"/*")) {
        m_cursor.advance(2);
        return 1;
    }
    if (m_cursor.lookingAt(U"*/")) {
        m_cursor.advance(2);
        return -1;
    }
    m_cursor.advance();
    return 0;
}

bool Lexer::skipLineComment()
{
    if (!m_cursor.lookingAt(U"//")) {
        return false;
    }
    while (!m_cursor.atEnd() && !isNewline(m_cursor.peek())) {
        m_cursor.advance();
    }
    consumeNewline();
    return true;
}

bool Lexer::skipLineContinuation()
{
    if (m_cursor.peek() != U'\\') {
        return false;
    }
    m_cursor.advance();
    while (skipWhitespace()) { }
    if (skipLineComment() || consumeNewline()) {
        return true;
    }
    failUnexpected(QStringLiteral("newline after line continuation"));
}

bool Lexer::consumeNewline()
{
    if (!isNewline(m_cursor.peek())) {
        return false;
    }
    m_cursor.advance(m_cursor.lookingAt(U"\r\n") ? 2 : 1);
    return true;
}

bool Lexer::atRawString() const
{
    if (m_cursor.peek() != U'r') {
        return false;
    }
    qsizetype offset = 1;
    while (m_cursor.peek(offset) == U'#') {
        ++offset;
    }
    return m_cursor.peek(offset) == U'"';
}

QString Lexer::readQuotedString()
{
    const Location start = m_cursor.location();
    m_cursor.advance();
    QString text;
    while (m_cursor.peek() != U'"') {
        if (m_cursor.atEnd()) {
            raiseSyntaxError(QStringLiteral("unclosed string"), start);
        }
        if (m_cursor.peek() == U'\\') {
            readEscape(text, start);
            continue;
        }
        appendCodePoint(text, m_cursor.peek());
        m_cursor.advance();
    }
    m_cursor.advance();
    return text;
}

QString Lexer::readRawString()
{
    const Location start = m_cursor.location();
    m_cursor.advance();
    std::u32string closing(1, U'"');
    while (m_cursor.peek() == U'#') {
        closing.push_back(U'#');
        m_cursor.advance();
    }
    m_cursor.advance();
    const qsizetype contentStart = m_cursor.position();
    while (!m_cursor.lookingAt(closing)) {
        if (m_cursor.atEnd()) {
            raiseSyntaxError(QStringLiteral("unclosed raw string"), start);
        }
        m_cursor.advance();
    }
    QString text = fromCodePoints(m_cursor.slice(contentStart));
    m_cursor.advance(static_cast<qsizetype>(closing.size()));
    return text;
}

void Lexer::readEscape(QString &target, const Location &stringStart)
{
    m_cursor.advance();
    const char32_t c = m_cursor.peek();
    if (c == U'u') {
        appendCodePoint(target, readUnicodeEscape());
        return;
    }
    if (m_cursor.atEnd()) {
        raiseSyntaxError(QStringLiteral("unclosed string"), stringStart);
    }
    const std::optional<char32_t> escaped = simpleEscape(c);
    if (!escaped) {
        raiseSyntaxError(QStringLiteral("invalid escape character %1 in string").arg(describeCharacter(c)), m_cursor.location());
    }
    appendCodePoint(target, *escaped);
    m_cursor.advance();
}

char32_t Lexer::readUnicodeEscape()
{
    m_cursor.advance();
    if (m_cursor.peek() != U'{') {
        failUnexpected(QStringLiteral("'{' after \\u"));
    }
    m_cursor.advance();
    const Location digitsStart = m_cursor.location();
    char32_t value = 0;
    int count = 0;
    for (std::optional<int> digit = digitValue(m_cursor.peek(), 16); digit && count < maxUnicodeEscapeDigits;
        digit = digitValue(m_cursor.peek(), 16)) {
        value = value * 16 + static_cast<char32_t>(*digit);
        ++count;
        m_cursor.advance();
    }
    if (count == 0) {
        failUnexpected(QStringLiteral("hexadecimal digit in unicode escape"));
    }
    if (m_cursor.peek() != U'}') {
        failUnexpected(QStringLiteral("'}' to close unicode escape"));
    }
    if (!isScalarValue(value)) {
        raiseSyntaxError(QStringLiteral("unicode escape is not a valid code point"), digitsStart);
    }
    m_cursor.advance();
    return value;
}

Word Lexer::readBareWord(const QString &expected)
{
    const Location start = m_cursor.location();
    const qsizetype from = m_cursor.position();
    while (isIdentifierChar(m_cursor.peek())) {
        m_cursor.advance();
    }
    if (m_cursor.position() == from) {
        failUnexpected(expected);
    }
    const std::u32string_view bare = m_cursor.slice(from);
    return Word {fromCodePoints(bare), bare, start, false};
}

}
