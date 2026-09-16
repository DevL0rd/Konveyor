#pragma once

#include <QList>
#include <QString>

#include <expected>
#include <optional>
#include <variant>

namespace Konveyor::Kdl
{

struct Location
{
    QString file;
    int line = 0;
    int column = 0;
};

struct Null
{
    bool operator==(const Null &) const = default;
};

struct Value
{
    std::variant<Null, bool, qint64, double, QString> data;
    std::optional<QString> typeAnnotation;
    Location location;

    bool isNull() const { return std::holds_alternative<Null>(data); }
    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isInteger() const { return std::holds_alternative<qint64>(data); }
    bool isFloat() const { return std::holds_alternative<double>(data); }
    bool isNumber() const { return isInteger() || isFloat(); }
    bool isString() const { return std::holds_alternative<QString>(data); }

    bool toBool() const { return std::get<bool>(data); }
    qint64 toInteger() const { return std::get<qint64>(data); }
    double toDouble() const { return isInteger() ? static_cast<double>(toInteger()) : std::get<double>(data); }
    QString toString() const { return std::get<QString>(data); }
};

struct Property
{
    QString name;
    Value value;
    Location location;
};

struct Span
{
    qsizetype start = -1;
    qsizetype end = -1;
    qsizetype childrenOpen = -1;
    qsizetype childrenClose = -1;
};

struct Node
{
    QString name;
    std::optional<QString> typeAnnotation;
    QList<Value> arguments;
    QList<Property> properties;
    QList<Node> children;
    Location location;
    Span span;

    const Property *property(const QString &propertyName) const;
    const Node *child(const QString &childName) const;
    QList<const Node *> childrenNamed(const QString &childName) const;
};

struct Document
{
    QList<Node> nodes;
};

struct ParseError
{
    QString message;
    Location location;

    QString toString() const;
};

std::expected<Document, ParseError> parse(const QString &text, const QString &fileName);

}
