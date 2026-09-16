#include "document/nodecodec.h"

#include "kdl/characters.h"
#include "kdl/numbers.h"

#include <cmath>

namespace Konveyor::Settings
{

namespace
{

QVariant valueToVariant(const Kdl::Value &value)
{
    return std::visit(
        [](const auto &data) -> QVariant {
            using T = std::decay_t<decltype(data)>;
            if (std::is_same_v<T, Kdl::Null>) {
                return QVariant::fromValue(nullptr);
            }
            return QVariant::fromValue(data);
        },
        value.data);
}

bool isBareIdentifier(const QString &name)
{
    if (name.isEmpty() || name == QLatin1String("true") || name == QLatin1String("false") || name == QLatin1String("null")) {
        return false;
    }
    const std::u32string codePoints = toCodePoints(name);
    if (Kdl::looksLikeNumber(codePoints)) {
        return false;
    }
    return std::ranges::all_of(codePoints, [](char32_t c) { return Kdl::isIdentifierChar(c); });
}

QString writeString(const QString &text)
{
    if (text.contains(QLatin1Char('\\')) || text.contains(QLatin1Char('"'))) {
        QString hashes = QStringLiteral("#");
        while (text.contains(QLatin1Char('"') + hashes)) {
            hashes += QLatin1Char('#');
        }
        return QLatin1Char('r') + hashes + QLatin1Char('"') + text + QLatin1Char('"') + hashes;
    }
    QString escaped = text;
    escaped.replace(QLatin1Char('\n'), QLatin1String("\\n")).replace(QLatin1Char('\t'), QLatin1String("\\t"));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

QString writeNumber(double number)
{
    if (std::isfinite(number) && number == std::trunc(number) && std::abs(number) < 1e15) {
        return QString::number(static_cast<qint64>(number));
    }
    return QString::number(number, 'g', 12);
}

QString writeProperties(const QVariant &properties)
{
    QString text;
    if (properties.typeId() == QMetaType::QVariantList) {
        for (const QVariant &entry : properties.toList()) {
            const QVariantList pair = entry.toList();
            text += QLatin1Char(' ') + writeIdentifier(pair.value(0).toString()) + QLatin1Char('=') + writeValue(pair.value(1));
        }
        return text;
    }
    const QVariantMap map = properties.toMap();
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        text += QLatin1Char(' ') + writeIdentifier(it.key()) + QLatin1Char('=') + writeValue(it.value());
    }
    return text;
}

}

std::u32string toCodePoints(const QString &text)
{
    std::u32string result;
    for (const uint codePoint : text.toUcs4()) {
        result.push_back(static_cast<char32_t>(codePoint));
    }
    return result;
}

QVariantMap nodeToVariant(const Kdl::Node &node)
{
    QVariantList arguments;
    for (const Kdl::Value &value : node.arguments) {
        arguments.append(valueToVariant(value));
    }
    QVariantMap properties;
    for (const Kdl::Property &property : node.properties) {
        properties.insert(property.name, valueToVariant(property.value));
    }
    QVariantList children;
    for (const Kdl::Node &child : node.children) {
        children.append(nodeToVariant(child));
    }
    QVariantMap result {
        {QStringLiteral("name"), node.name},
        {QStringLiteral("args"), arguments},
        {QStringLiteral("props"), properties},
    };
    if (node.span.childrenOpen >= 0) {
        result.insert(QStringLiteral("children"), children);
    }
    return result;
}

QString writeIdentifier(const QString &name)
{
    return isBareIdentifier(name) ? name : writeString(name);
}

QString writeValue(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Nullptr:
    case QMetaType::UnknownType:
        return QStringLiteral("null");
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Float:
        return writeNumber(value.toDouble());
    default:
        return writeString(value.toString());
    }
}

QString writeNodeHead(const QVariantMap &node)
{
    QString text = writeIdentifier(node.value(QStringLiteral("name")).toString());
    for (const QVariant &argument : node.value(QStringLiteral("args")).toList()) {
        text += QLatin1Char(' ') + writeValue(argument);
    }
    return text + writeProperties(node.value(QStringLiteral("props")));
}

QString writeNode(const QVariantMap &node, const QString &indent)
{
    QString text = writeNodeHead(node);
    if (!node.contains(QStringLiteral("children"))) {
        return text;
    }
    const QVariantList children = node.value(QStringLiteral("children")).toList();
    if (children.isEmpty()) {
        return text + QStringLiteral(" {}");
    }
    const QVariantMap onlyChild = children.size() == 1 ? children.first().toMap() : QVariantMap();
    if (!onlyChild.isEmpty() && !onlyChild.contains(QStringLiteral("children"))) {
        return text + QStringLiteral(" { ") + writeNodeHead(onlyChild) + QStringLiteral("; }");
    }
    const QString inner = indent + QStringLiteral("    ");
    text += QStringLiteral(" {\n");
    for (const QVariant &child : children) {
        text += inner + writeNode(child.toMap(), inner) + QLatin1Char('\n');
    }
    return text + indent + QLatin1Char('}');
}

}
