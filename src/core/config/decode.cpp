#include "config/decode.h"

namespace Konveyor::Config
{

namespace
{

QString unexpectedNode(const Kdl::Node &child)
{
    return QStringLiteral("unexpected node ") + quoteName(child.name);
}

QString betweenMessage(Range range)
{
    return QStringLiteral("value must be between %1 and %2").arg(range.min).arg(range.max);
}

}

void failAt(const Kdl::Location &location, const QString &message)
{
    throw DecodeError {message, location};
}

void fail(const Kdl::Node &node, const QString &message)
{
    failAt(node.location, message);
}

QString quoteName(const QString &name)
{
    return QStringLiteral("`") + name + QStringLiteral("`");
}

void expectNoArguments(const Kdl::Node &node)
{
    if (!node.arguments.isEmpty()) {
        failAt(node.arguments.first().location, QStringLiteral("no arguments expected for this node"));
    }
}

void expectNoProperties(const Kdl::Node &node)
{
    if (!node.properties.isEmpty()) {
        const Kdl::Property &property = node.properties.first();
        failAt(property.location, QStringLiteral("unexpected property ") + quoteName(property.name));
    }
}

void expectNoChildren(const Kdl::Node &node)
{
    if (!node.children.isEmpty()) {
        failAt(node.children.first().location, unexpectedNode(node.children.first()));
    }
}

void expectOnlyChildren(const Kdl::Node &node)
{
    expectNoArguments(node);
    expectNoProperties(node);
}

void expectLeafNode(const Kdl::Node &node)
{
    expectNoProperties(node);
    expectNoChildren(node);
}

void expectArgumentLimit(const Kdl::Node &node, int limit)
{
    if (node.arguments.size() > limit) {
        failAt(node.arguments.at(limit).location, QStringLiteral("unexpected argument"));
    }
}

const Kdl::Value &requiredArgument(const Kdl::Node &node, const QString &name)
{
    if (node.arguments.isEmpty()) {
        fail(node, QStringLiteral("additional argument ") + quoteName(name) + QStringLiteral(" is required"));
    }
    return node.arguments.first();
}

double toNumber(const Kdl::Value &value, Range range)
{
    if (!value.isNumber()) {
        failAt(value.location, QStringLiteral("unsupported value, only numbers are recognized"));
    }
    const double number = value.toDouble();
    if (number < static_cast<double>(range.min) || number > static_cast<double>(range.max)) {
        failAt(value.location, betweenMessage(range));
    }
    return number;
}

qint64 toInteger(const Kdl::Value &value, Range range)
{
    if (!value.isInteger()) {
        failAt(value.location, QStringLiteral("expected an integer"));
    }
    const qint64 number = value.toInteger();
    if (number < range.min || number > range.max) {
        failAt(value.location, betweenMessage(range));
    }
    return number;
}

QString toText(const Kdl::Value &value)
{
    if (!value.isString()) {
        failAt(value.location, QStringLiteral("expected a string"));
    }
    return value.toString();
}

bool toBoolean(const Kdl::Value &value)
{
    if (!value.isBool()) {
        failAt(value.location, QStringLiteral("expected a boolean"));
    }
    return value.toBool();
}

QString toWritten(const Kdl::Value &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isInteger()) {
        return QString::number(value.toInteger());
    }
    if (value.isFloat()) {
        return QString::number(value.toDouble());
    }
    return QStringLiteral("null");
}

int toKeyword(const Kdl::Value &value, const QStringList &keywords)
{
    const QString text = toText(value);
    const qsizetype index = keywords.indexOf(text);
    if (index < 0) {
        QStringList quoted;
        for (const QString &keyword : keywords) {
            quoted.append(quoteName(keyword));
        }
        failAt(value.location, QStringLiteral("expected one of ") + quoted.join(QStringLiteral(", ")));
    }
    return static_cast<int>(index);
}

double numberArgument(const Kdl::Node &node, Range range)
{
    expectLeafNode(node);
    expectArgumentLimit(node, 1);
    return toNumber(requiredArgument(node, node.name), range);
}

qint64 integerArgument(const Kdl::Node &node, Range range)
{
    expectLeafNode(node);
    expectArgumentLimit(node, 1);
    return toInteger(requiredArgument(node, node.name), range);
}

QString stringArgument(const Kdl::Node &node)
{
    expectLeafNode(node);
    expectArgumentLimit(node, 1);
    return toText(requiredArgument(node, node.name));
}

bool booleanArgument(const Kdl::Node &node)
{
    expectLeafNode(node);
    expectArgumentLimit(node, 1);
    return toBoolean(requiredArgument(node, node.name));
}

bool flagArgument(const Kdl::Node &node)
{
    expectLeafNode(node);
    expectArgumentLimit(node, 1);
    if (node.arguments.isEmpty()) {
        return true;
    }
    return toBoolean(node.arguments.first());
}

int keywordArgument(const Kdl::Node &node, const QStringList &keywords)
{
    expectLeafNode(node);
    expectArgumentLimit(node, 1);
    return toKeyword(requiredArgument(node, node.name), keywords);
}

void decodeChildren(const Kdl::Node &node, const NodeTable &table, const QSet<QString> &repeatable)
{
    QSet<QString> seen;
    for (const Kdl::Node &child : node.children) {
        const auto handler = table.constFind(child.name);
        if (handler == table.constEnd()) {
            failAt(child.location, unexpectedNode(child));
        }
        if (!repeatable.contains(child.name) && seen.contains(child.name)) {
            failAt(child.location, QStringLiteral("duplicate node ") + quoteName(child.name) + QStringLiteral(", single node expected"));
        }
        seen.insert(child.name);
        (*handler)(child);
    }
}

void decodeProperties(const Kdl::Node &node, const ValueTable &table)
{
    for (const Kdl::Property &property : node.properties) {
        const auto handler = table.constFind(property.name);
        if (handler == table.constEnd()) {
            failAt(property.location, QStringLiteral("unexpected property ") + quoteName(property.name));
        }
        (*handler)(property.value);
    }
}

}
