#include "kdl/parser.h"

#include "kdl/characters.h"
#include "kdl/numbers.h"
#include "kdl/syntaxerror.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Kdl
{

namespace
{

using ValueData = decltype(Value::data);

constexpr int maxNestingDepth = 256;

std::optional<ValueData> keywordValue(std::u32string_view bare)
{
    if (bare == U"true") {
        return ValueData(true);
    }
    if (bare == U"false") {
        return ValueData(false);
    }
    if (bare == U"null") {
        return ValueData(Null {});
    }
    return std::nullopt;
}

ValueData numberData(const Word &word)
{
    const std::expected<NumberValue, NumberError> number = parseNumber(word.bare);
    if (!number) {
        Location location = word.location;
        location.column += static_cast<int>(number.error().offset);
        raiseSyntaxError(number.error().message, location);
    }
    return std::visit([](auto value) { return ValueData(value); }, *number);
}

void appendArgument(Node &node, Value value, bool discard)
{
    if (!discard) {
        node.arguments.push_back(std::move(value));
    }
}

void addProperty(Node &node, Property property, bool discard)
{
    if (discard) {
        return;
    }
    const auto existing
        = std::ranges::find_if(node.properties, [&property](const Property &candidate) { return candidate.name == property.name; });
    if (existing != node.properties.end()) {
        *existing = std::move(property);
        return;
    }
    node.properties.push_back(std::move(property));
}

}

Parser::Parser(const QString &text, const QString &fileName)
    : m_cursor(text, fileName)
    , m_lexer(m_cursor)
{ }

Document Parser::parseDocument()
{
    Document document {parseNodes()};
    if (!m_cursor.atEnd()) {
        raiseSyntaxError(QStringLiteral("unexpected '}' without matching '{'"), m_cursor.location());
    }
    return document;
}

QList<Node> Parser::parseNodes()
{
    QList<Node> nodes;
    while (true) {
        m_lexer.skipLineSpace();
        if (m_cursor.atEnd() || m_cursor.peek() == U'}') {
            return nodes;
        }
        const bool discard = m_lexer.consumeSlashdash();
        Node node = parseNode();
        if (!discard) {
            nodes.push_back(std::move(node));
        }
    }
}

Node Parser::parseNode()
{
    Node node;
    node.location = m_cursor.location();
    node.typeAnnotation = parseTypeAnnotation();
    node.name = identifierFromWord(m_lexer.readWord(QStringLiteral("node name")), QStringLiteral("node name"));
    parseEntries(node);
    return node;
}

void Parser::parseEntries(Node &node)
{
    while (true) {
        const bool separated = m_lexer.skipNodeSpace();
        if (m_lexer.consumeNodeTerminator()) {
            return;
        }
        const Cursor::Mark entryStart = m_cursor.mark();
        const bool discard = m_lexer.consumeSlashdash();
        if (m_cursor.peek() == U'{') {
            parseChildren(node, discard);
            finishNodeAfterChildren();
            return;
        }
        requireEntrySeparator(separated, entryStart);
        parseEntry(node, discard);
    }
}

void Parser::parseEntry(Node &node, bool discard)
{
    const Location start = m_cursor.location();
    if (m_cursor.peek() == U'(') {
        appendArgument(node, parseValue(), discard);
        return;
    }
    const Word word = m_lexer.readWord(QStringLiteral("argument or property"));
    if (m_cursor.peek() != U'=') {
        appendArgument(node, valueFromWord(word), discard);
        return;
    }
    m_cursor.advance();
    QString name = identifierFromWord(word, QStringLiteral("property name"));
    addProperty(node, Property {std::move(name), parseValue(), start}, discard);
}

void Parser::parseChildren(Node &node, bool discard)
{
    const Location open = m_cursor.location();
    if (m_depth >= maxNestingDepth) {
        raiseSyntaxError(QStringLiteral("children blocks are nested too deeply"), open);
    }
    ++m_depth;
    m_cursor.advance();
    QList<Node> children = parseNodes();
    if (m_cursor.atEnd()) {
        raiseSyntaxError(QStringLiteral("unclosed children block"), open);
    }
    m_cursor.advance();
    --m_depth;
    if (!discard) {
        node.children = std::move(children);
    }
}

void Parser::finishNodeAfterChildren()
{
    m_lexer.skipNodeSpace();
    if (!m_lexer.consumeNodeTerminator()) {
        m_lexer.failUnexpected(QStringLiteral("';' or newline after children block"));
    }
}

void Parser::requireEntrySeparator(bool separated, const Cursor::Mark &entryStart)
{
    if (m_cursor.peek() == U'}') {
        m_lexer.failUnexpected(QStringLiteral("';' or newline"));
    }
    if (!separated) {
        m_cursor.reset(entryStart);
        m_lexer.failUnexpected(QStringLiteral("whitespace, ';' or newline"));
    }
}

Value Parser::parseValue()
{
    const Location start = m_cursor.location();
    std::optional<QString> typeAnnotation = parseTypeAnnotation();
    Value value = valueFromWord(m_lexer.readWord(QStringLiteral("value")));
    value.typeAnnotation = std::move(typeAnnotation);
    value.location = start;
    return value;
}

std::optional<QString> Parser::parseTypeAnnotation()
{
    if (m_cursor.peek() != U'(') {
        return std::nullopt;
    }
    m_cursor.advance();
    QString type = identifierFromWord(m_lexer.readWord(QStringLiteral("type annotation")), QStringLiteral("type annotation"));
    if (m_cursor.peek() != U')') {
        m_lexer.failUnexpected(QStringLiteral("')' after type annotation"));
    }
    m_cursor.advance();
    return type;
}

QString Parser::identifierFromWord(const Word &word, const QString &role)
{
    if (word.quoted) {
        return word.text;
    }
    if (looksLikeNumber(word.bare)) {
        raiseSyntaxError(QStringLiteral("%1 cannot be a number; use a quoted string").arg(role), word.location);
    }
    if (keywordValue(word.bare)) {
        raiseSyntaxError(QStringLiteral("%1 cannot be the keyword '%2'; use a quoted string").arg(role, word.text), word.location);
    }
    return word.text;
}

Value Parser::valueFromWord(const Word &word)
{
    Value value;
    value.location = word.location;
    if (word.quoted) {
        value.data = word.text;
        return value;
    }
    if (std::optional<ValueData> keyword = keywordValue(word.bare)) {
        value.data = std::move(*keyword);
        return value;
    }
    if (!looksLikeNumber(word.bare)) {
        raiseSyntaxError(QStringLiteral("unexpected identifier '%1'; values must be strings, numbers, true, false or null").arg(word.text),
            word.location);
    }
    value.data = numberData(word);
    return value;
}

}
