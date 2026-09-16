#pragma once

#include "kdl/cursor.h"
#include "kdl/lexer.h"

namespace Konveyor::Kdl
{

class Parser
{
public:
    Parser(const QString &text, const QString &fileName);

    Document parseDocument();

private:
    QList<Node> parseNodes();
    Node parseNode();
    void parseEntries(Node &node);
    void parseEntry(Node &node, bool discard);
    void parseChildren(Node &node, bool discard);
    void finishNodeAfterChildren();
    void requireEntrySeparator(bool separated, const Cursor::Mark &entryStart);
    Value parseValue();
    std::optional<QString> parseTypeAnnotation();
    static QString identifierFromWord(const Word &word, const QString &role);
    static Value valueFromWord(const Word &word);

    Cursor m_cursor;
    Lexer m_lexer;
    int m_depth = 0;
};

}
