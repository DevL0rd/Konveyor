#pragma once

#include <QString>

#include <optional>
#include <string_view>

namespace Konveyor::Kdl
{

inline constexpr char32_t endOfInput = 0xFFFFFFFF;
inline constexpr char32_t maxCodePoint = 0x10FFFF;

bool isNewline(char32_t c);
bool isUnicodeSpace(char32_t c);
bool isIdentifierChar(char32_t c);
bool isDecimalDigit(char32_t c);
bool isScalarValue(char32_t c);
std::optional<int> digitValue(char32_t c, int radix);
void appendCodePoint(QString &target, char32_t c);
QString fromCodePoints(std::u32string_view codePoints);
QString describeCharacter(char32_t c);

}
