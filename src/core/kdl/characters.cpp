#include "kdl/characters.h"

#include <utility>

namespace Konveyor::Kdl
{

namespace
{

constexpr std::u32string_view reservedCharacters = U"\\/(){}<>;[]=,\"";

std::optional<int> letterValue(char32_t c)
{
    if (c >= U'a' && c <= U'z') {
        return static_cast<int>(c - U'a') + 10;
    }
    if (c >= U'A' && c <= U'Z') {
        return static_cast<int>(c - U'A') + 10;
    }
    return std::nullopt;
}

}

bool isNewline(char32_t c)
{
    return c == U'\r' || c == U'\n' || c == 0x85 || c == 0x0C || c == 0x2028 || c == 0x2029;
}

bool isUnicodeSpace(char32_t c)
{
    return c == U'\t' || c == U' ' || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x202F || c == 0x205F || c == 0x3000
        || c == 0xFEFF;
}

bool isIdentifierChar(char32_t c)
{
    if (c <= 0x20 || c > maxCodePoint || isNewline(c) || isUnicodeSpace(c)) {
        return false;
    }
    return reservedCharacters.find(c) == std::u32string_view::npos;
}

bool isDecimalDigit(char32_t c)
{
    return c >= U'0' && c <= U'9';
}

bool isScalarValue(char32_t c)
{
    return c <= maxCodePoint && (c < 0xD800 || c > 0xDFFF);
}

std::optional<int> digitValue(char32_t c, int radix)
{
    const std::optional<int> value = isDecimalDigit(c) ? std::optional<int>(static_cast<int>(c - U'0')) : letterValue(c);
    if (value && *value < radix) {
        return value;
    }
    return std::nullopt;
}

void appendCodePoint(QString &target, char32_t c)
{
    if (QChar::requiresSurrogates(c)) {
        target.append(QChar(QChar::highSurrogate(c)));
        target.append(QChar(QChar::lowSurrogate(c)));
        return;
    }
    target.append(QChar(static_cast<char16_t>(c)));
}

QString fromCodePoints(std::u32string_view codePoints)
{
    return QString::fromUcs4(codePoints.data(), static_cast<qsizetype>(codePoints.size()));
}

QString describeCharacter(char32_t c)
{
    if (c == endOfInput) {
        return QStringLiteral("end of input");
    }
    if (isNewline(c)) {
        return QStringLiteral("newline");
    }
    if (isUnicodeSpace(c)) {
        return QStringLiteral("whitespace");
    }
    if (c < 0x20 || c > maxCodePoint) {
        return QStringLiteral("U+%1").arg(QString::number(static_cast<quint32>(c), 16).toUpper().rightJustified(4, QLatin1Char('0')));
    }
    QString text;
    appendCodePoint(text, c);
    return QStringLiteral("'%1'").arg(text);
}

QStringList splitLines(const QString &text)
{
    QStringList lines;
    QString current;
    const QList<uint> codePoints = text.toUcs4();
    for (qsizetype index = 0; index < codePoints.size(); ++index) {
        const auto c = static_cast<char32_t>(codePoints.at(index));
        if (!isNewline(c)) {
            appendCodePoint(current, c);
            continue;
        }
        if (c == U'\r' && index + 1 < codePoints.size() && codePoints.at(index + 1) == U'\n') {
            ++index;
        }
        lines.append(std::exchange(current, QString()));
    }
    lines.append(current);
    return lines;
}

QString rawString(const QString &text)
{
    QString hashes = QStringLiteral("#");
    while (text.contains(QLatin1Char('"') + hashes)) {
        hashes += QLatin1Char('#');
    }
    return QLatin1Char('r') + hashes + QLatin1Char('"') + text + QLatin1Char('"') + hashes;
}

}
