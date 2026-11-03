#include "kdl/characters.h"
#include "kdl/cursor.h"
#include "kdl/syntaxerror.h"

#include <QTest>

using namespace Konveyor::Kdl;

class TestKdlInternals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void syntaxErrorsCarryTheirParseError();
    void cursorPeekOutsideTheTextIsEndOfInput();
    void cursorAdvanceStopsAtTheEnd();
    void everyUnicodeSpaceIsRecognised_data();
    void everyUnicodeSpaceIsRecognised();
    void nonSpacesAreNotSpaces();
    void codePointsAboveTheRangeAreDescribedByNumber();
    void rawStringsUseEnoughHashesToStayClosed_data();
    void rawStringsUseEnoughHashesToStayClosed();
    void splitLinesFollowsTheKdlNewlines_data();
    void splitLinesFollowsTheKdlNewlines();
};

void TestKdlInternals::syntaxErrorsCarryTheirParseError()
{
    try {
        raiseSyntaxError(QStringLiteral("broken"), Location {QStringLiteral("a.kdl"), 3, 4});
        QFAIL("no error was raised");
    } catch (const SyntaxError &error) {
        QCOMPARE(error.error().message, QStringLiteral("broken"));
        QCOMPARE(error.error().location.line, 3);
        QCOMPARE(error.error().location.column, 4);
        QCOMPARE(QString::fromLatin1(error.what()), QStringLiteral("KDL syntax error"));
    }
}

void TestKdlInternals::cursorPeekOutsideTheTextIsEndOfInput()
{
    const Cursor cursor(QStringLiteral("ab"), QStringLiteral("a.kdl"));
    QCOMPARE(cursor.peek(-1), endOfInput);
    QCOMPARE(cursor.peek(0), U'a');
    QCOMPARE(cursor.peek(1), U'b');
    QCOMPARE(cursor.peek(2), endOfInput);
}

void TestKdlInternals::cursorAdvanceStopsAtTheEnd()
{
    Cursor cursor(QStringLiteral("ab"), QStringLiteral("a.kdl"));
    cursor.advance(100);
    QVERIFY(cursor.atEnd());
    QCOMPARE(cursor.position(), qsizetype(2));
}

void TestKdlInternals::everyUnicodeSpaceIsRecognised_data()
{
    QTest::addColumn<quint32>("codePoint");
    for (const quint32 c : {0x9u, 0x20u, 0xA0u, 0x1680u, 0x2000u, 0x2005u, 0x200Au, 0x202Fu, 0x205Fu, 0x3000u, 0xFEFFu}) {
        QTest::newRow(qPrintable(QString::number(c, 16))) << c;
    }
}

void TestKdlInternals::everyUnicodeSpaceIsRecognised()
{
    QFETCH(quint32, codePoint);
    QVERIFY(isUnicodeSpace(codePoint));
}

void TestKdlInternals::nonSpacesAreNotSpaces()
{
    for (const char32_t c : {U'a', U'\n', char32_t(0x1FFF), char32_t(0x200B), char32_t(0x2060), char32_t(0x3001)}) {
        QVERIFY(!isUnicodeSpace(c));
    }
}

void TestKdlInternals::codePointsAboveTheRangeAreDescribedByNumber()
{
    QCOMPARE(describeCharacter(0x110000), QStringLiteral("U+110000"));
    QCOMPARE(describeCharacter(0x1), QStringLiteral("U+0001"));
}

void TestKdlInternals::rawStringsUseEnoughHashesToStayClosed_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("literal");
    QTest::newRow("plain") << QStringLiteral("abc") << QStringLiteral("r#\"abc\"#");
    QTest::newRow("backslash") << QStringLiteral("a\\b") << QStringLiteral("r#\"a\\b\"#");
    QTest::newRow("quote") << QStringLiteral("a\"b") << QStringLiteral("r#\"a\"b\"#");
    QTest::newRow("quote and hash") << QStringLiteral("a\"#b") << QStringLiteral("r##\"a\"#b\"##");
    QTest::newRow("quote and two hashes") << QStringLiteral("a\"##b\"#") << QStringLiteral("r###\"a\"##b\"#\"###");
}

void TestKdlInternals::rawStringsUseEnoughHashesToStayClosed()
{
    QFETCH(QString, text);
    QFETCH(QString, literal);
    QCOMPARE(rawString(text), literal);
}

void TestKdlInternals::splitLinesFollowsTheKdlNewlines_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QStringList>("lines");
    QTest::newRow("empty") << QString() << QStringList {QString()};
    QTest::newRow("single") << QStringLiteral("one") << QStringList {QStringLiteral("one")};
    QTest::newRow("line feed") << QStringLiteral("a\nb") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("crlf is one break") << QStringLiteral("a\r\nb") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("lone cr") << QStringLiteral("a\rb") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("cr at the end") << QStringLiteral("a\r") << QStringList {QStringLiteral("a"), QString()};
    QTest::newRow("cr then cr") << QStringLiteral("a\r\rb") << QStringList {QStringLiteral("a"), QString(), QStringLiteral("b")};
    QTest::newRow("next line") << QStringLiteral("a\u0085b") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("form feed") << QStringLiteral("a\fb") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("line separator") << QStringLiteral("a b") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("paragraph separator") << QStringLiteral("a b") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    QTest::newRow("trailing line feed") << QStringLiteral("a\n") << QStringList {QStringLiteral("a"), QString()};
    QTest::newRow("astral character") << QStringLiteral("\U0001F600\nx") << QStringList {QStringLiteral("\U0001F600"), QStringLiteral("x")};
}

void TestKdlInternals::splitLinesFollowsTheKdlNewlines()
{
    QFETCH(QString, text);
    QFETCH(QStringList, lines);
    QCOMPARE(splitLines(text), lines);
}

QTEST_GUILESS_MAIN(TestKdlInternals)
#include "test_kdl_internals.moc"
