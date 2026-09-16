#include "kdl/kdl.h"

#include "kdl/parser.h"
#include "kdl/syntaxerror.h"

#include <algorithm>

namespace Konveyor::Kdl
{

const Property *Node::property(const QString &propertyName) const
{
    const auto found = std::ranges::find(properties, propertyName, &Property::name);
    return found == properties.end() ? nullptr : &*found;
}

const Node *Node::child(const QString &childName) const
{
    const auto found = std::ranges::find(children, childName, &Node::name);
    return found == children.end() ? nullptr : &*found;
}

QList<const Node *> Node::childrenNamed(const QString &childName) const
{
    QList<const Node *> matches;
    for (const Node &candidate : children) {
        if (candidate.name == childName) {
            matches.push_back(&candidate);
        }
    }
    return matches;
}

QString ParseError::toString() const
{
    return QStringLiteral("%1:%2:%3: %4").arg(location.file, QString::number(location.line), QString::number(location.column), message);
}

std::expected<Document, ParseError> parse(const QString &text, const QString &fileName)
{
    try {
        Parser parser(text, fileName);
        return parser.parseDocument();
    } catch (const SyntaxError &error) {
        return std::unexpected(error.error());
    }
}

}
