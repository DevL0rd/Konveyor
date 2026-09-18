#include "config/forceresizable.h"

#include "config/loader.h"
#include "kdl/kdl.h"

#include <QRegularExpression>

#include <ranges>
#include <string>

namespace Konveyor::Config
{

namespace
{

constexpr QLatin1StringView OverrideFile("force-resizable.kdl");

std::u32string toCodePoints(const QString &text)
{
    std::u32string result;
    const QList<uint> points = text.toUcs4();
    result.reserve(static_cast<std::size_t>(points.size()));
    for (const uint point : points) {
        result.push_back(static_cast<char32_t>(point));
    }
    return result;
}

QString fromCodePoints(const std::u32string &text)
{
    return QString::fromUcs4(text.data(), static_cast<qsizetype>(text.size()));
}

void replace(std::u32string &text, qsizetype from, qsizetype to, const QString &replacement)
{
    text.replace(static_cast<std::size_t>(from), static_cast<std::size_t>(to - from), toCodePoints(replacement));
}

QString slice(const std::u32string &text, qsizetype from, qsizetype to)
{
    return QString::fromUcs4(text.data() + from, to - from);
}

qsizetype lineStart(const std::u32string &text, qsizetype position)
{
    while (position > 0) {
        const char32_t previous = text[static_cast<std::size_t>(position - 1)];
        if (previous == U'\n' || previous == U'\r') {
            break;
        }
        --position;
    }
    return position;
}

QString indentAt(const std::u32string &text, qsizetype position)
{
    const qsizetype start = lineStart(text, position);
    qsizetype end = start;
    while (end < position) {
        const char32_t current = text[static_cast<std::size_t>(end)];
        if (current != U' ' && current != U'\t') {
            break;
        }
        ++end;
    }
    return slice(text, start, end);
}

QString rawString(const QString &value)
{
    QString hashes = QStringLiteral("#");
    while (value.contains(QLatin1Char('"') + hashes)) {
        hashes += QLatin1Char('#');
    }
    return QLatin1Char('r') + hashes + QLatin1Char('"') + value + QLatin1Char('"') + hashes;
}

bool isExactAppRule(const Kdl::Node &rule, const QString &pattern)
{
    if (rule.name != QLatin1String("window-rule") || !rule.childrenNamed(QStringLiteral("exclude")).isEmpty()) {
        return false;
    }
    const QList<const Kdl::Node *> matches = rule.childrenNamed(QStringLiteral("match"));
    if (matches.size() != 1) {
        return false;
    }
    const Kdl::Node &match = *matches.first();
    const Kdl::Property *appId = match.property(QStringLiteral("app-id"));
    return match.arguments.isEmpty() && match.children.isEmpty() && match.properties.size() == 1 && appId && appId->value.isString()
        && appId->value.toString() == pattern;
}

const Kdl::Node *lastExactAppRule(const Kdl::Document &document, const QString &pattern)
{
    for (const Kdl::Node &node : std::views::reverse(document.nodes)) {
        if (isExactAppRule(node, pattern)) {
            return &node;
        }
    }
    return nullptr;
}

QString appendBlock(QString text, const QString &block)
{
    if (!text.isEmpty() && !text.endsWith(QLatin1Char('\n'))) {
        text += QLatin1Char('\n');
    }
    if (!text.isEmpty() && !text.endsWith(QLatin1String("\n\n"))) {
        text += QLatin1Char('\n');
    }
    return text + block;
}

QString appendRule(const QString &text, const QString &pattern, bool enabled)
{
    const QString block = QStringLiteral("window-rule {\n    match app-id=%1\n    force-resizable %2\n}\n")
                              .arg(rawString(pattern), enabled ? QStringLiteral("true") : QStringLiteral("false"));
    return appendBlock(text, block);
}

QString updateRule(const QString &text, const Kdl::Node &rule, bool enabled)
{
    const QString value = enabled ? QStringLiteral("true") : QStringLiteral("false");
    std::u32string updated = toCodePoints(text);
    if (const Kdl::Node *setting = rule.child(QStringLiteral("force-resizable"))) {
        replace(updated, setting->span.start, setting->span.end, QStringLiteral("force-resizable ") + value);
        return fromCodePoints(updated);
    }
    const QString indent = indentAt(updated, rule.span.start) + QStringLiteral("    ");
    const QString body = slice(updated, rule.span.childrenOpen, rule.span.childrenClose);
    if (body.contains(QLatin1Char('\n')) || body.contains(QLatin1Char('\r'))) {
        const qsizetype closeStart = lineStart(updated, rule.span.childrenClose);
        const bool closeOwnsLine = slice(updated, closeStart, rule.span.childrenClose).trimmed().isEmpty();
        const qsizetype insertion = closeOwnsLine ? closeStart : rule.span.childrenClose;
        const QString prefix = closeOwnsLine ? QString() : QLatin1String("\n");
        replace(updated, insertion, insertion, prefix + indent + QStringLiteral("force-resizable ") + value + QLatin1Char('\n'));
    } else {
        replace(updated, rule.span.childrenClose, rule.span.childrenClose, QStringLiteral(" force-resizable ") + value + QLatin1Char(';'));
    }
    return fromCodePoints(updated);
}

bool isOverrideInclude(const Kdl::Node &node)
{
    return node.name == QLatin1String("include") && node.arguments.size() == 1 && node.arguments.first().isString()
        && node.arguments.first().toString() == OverrideFile;
}

}

std::expected<QString, QString> setForceResizableRule(const QString &text, const QString &fileName, const QString &appId, bool enabled)
{
    const auto document = Kdl::parse(text, fileName);
    if (!document) {
        return std::unexpected(document.error().toString());
    }
    const QString pattern = QLatin1Char('^') + QRegularExpression::escape(appId) + QLatin1Char('$');
    const Kdl::Node *rule = lastExactAppRule(*document, pattern);
    QString candidate = rule ? updateRule(text, *rule, enabled) : appendRule(text, pattern, enabled);
    const auto validated = loadString(candidate, fileName);
    if (!validated) {
        return std::unexpected(validated.error().toString());
    }
    return candidate;
}

std::expected<QString, QString> ensureTrailingForceResizableInclude(const QString &text, const QString &fileName)
{
    const auto document = Kdl::parse(text, fileName);
    if (!document) {
        return std::unexpected(document.error().toString());
    }
    if (!document->nodes.isEmpty() && isOverrideInclude(document->nodes.last())) {
        return text;
    }
    return appendBlock(text, QStringLiteral("include \"force-resizable.kdl\"\n"));
}

}
