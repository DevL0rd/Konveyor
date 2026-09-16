#pragma once

#include "kdl/cursor.h"

#include <string_view>

namespace Konveyor::Kdl
{

struct Word
{
    QString text;
    std::u32string_view bare;
    Location location;
    bool quoted = false;
};

class Lexer
{
public:
    explicit Lexer(Cursor &cursor);

    bool skipNodeSpace();
    void skipLineSpace();
    bool consumeNodeTerminator();
    bool consumeSlashdash();
    Word readWord(const QString &expected);
    [[noreturn]] void failUnexpected(const QString &expected) const;

private:
    bool skipWhitespace();
    bool skipBlockComment();
    int stepInsideBlockComment();
    bool skipLineComment();
    bool skipLineContinuation();
    bool consumeNewline();
    bool atRawString() const;
    QString readQuotedString();
    QString readRawString();
    void readEscape(QString &target, const Location &stringStart);
    char32_t readUnicodeEscape();
    Word readBareWord(const QString &expected);

    Cursor &m_cursor;
};

}
