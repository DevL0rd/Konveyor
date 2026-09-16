#include "kdltesthelpers.h"

#include <QRandomGenerator>
#include <QTest>

using namespace Konveyor::Kdl;
using namespace Konveyor::Kdl::Testing;

class TestKdlErrors : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void syntaxErrors_data();
    void syntaxErrors();
    void stringErrors_data();
    void stringErrors();
    void numberErrors_data();
    void numberErrors();
    void structureErrors_data();
    void structureErrors();
    void errorToString();
    void deepNestingIsRejected();
    void pathologicalInputs_data();
    void pathologicalInputs();
    void randomInputNeverCrashes();

private:
    static void addColumns();
    static void verifyError();
};

void TestKdlErrors::addColumns()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");
    QTest::addColumn<QString>("message");
}

void TestKdlErrors::verifyError()
{
    QFETCH(QString, text);
    QFETCH(int, line);
    QFETCH(int, column);
    QFETCH(QString, message);
    const auto result = parseText(text);
    QVERIFY(!result);
    QCOMPARE(result.error().message, message);
    QCOMPARE(result.error().location.line, line);
    QCOMPARE(result.error().location.column, column);
    QCOMPARE(result.error().location.file, testFileName.toString());
}

void TestKdlErrors::syntaxErrors_data()
{
    addColumns();
    const QString valueHint = QStringLiteral("; values must be strings, numbers, true, false or null");
    QTest::newRow("bare argument") << QStringLiteral("node a") << 1 << 6 << (QStringLiteral("unexpected identifier 'a'") + valueHint);
    QTest::newRow("dash dash argument") << QStringLiteral("node --") << 1 << 6
                                        << (QStringLiteral("unexpected identifier '--'") + valueHint);
    QTest::newRow("unicode columns") << QStringLiteral("日本語 \"x\" ? é=1") << 1 << 9
                                     << (QStringLiteral("unexpected identifier '?'") + valueHint);
    QTest::newRow("astral columns") << QStringLiteral("😀 x") << 1 << 3 << (QStringLiteral("unexpected identifier 'x'") + valueHint);
    QTest::newRow("keyword node name") << QStringLiteral("true") << 1 << 1
                                       << QStringLiteral("node name cannot be the keyword 'true'; use a quoted string");
    QTest::newRow("number node name") << QStringLiteral("\n  -1 2") << 2 << 3
                                      << QStringLiteral("node name cannot be a number; use a quoted string");
    QTest::newRow("keyword property") << QStringLiteral("node null=1") << 1 << 6
                                      << QStringLiteral("property name cannot be the keyword 'null'; use a quoted string");
    QTest::newRow("number property") << QStringLiteral("node 1=2") << 1 << 6
                                     << QStringLiteral("property name cannot be a number; use a quoted string");
    QTest::newRow("number type") << QStringLiteral("(1)node") << 1 << 2
                                 << QStringLiteral("type annotation cannot be a number; use a quoted string");
    QTest::newRow("space in type") << QStringLiteral("( t)node") << 1 << 2 << QStringLiteral("expected type annotation, found whitespace");
    QTest::newRow("unclosed type") << QStringLiteral("(t node") << 1 << 3
                                   << QStringLiteral("expected ')' after type annotation, found whitespace");
    QTest::newRow("space after type") << QStringLiteral("node (t) 1") << 1 << 9 << QStringLiteral("expected value, found whitespace");
    QTest::newRow("typed property name") << QStringLiteral("node (t)\"k\"=1") << 1 << 12
                                         << QStringLiteral("expected whitespace, ';' or newline, found '='");
    QTest::newRow("missing value") << QStringLiteral("node key=") << 1 << 10 << QStringLiteral("expected value, found end of input");
    QTest::newRow("space after equals") << QStringLiteral("node key= 1") << 1 << 10 << QStringLiteral("expected value, found whitespace");
    QTest::newRow("space before equals") << QStringLiteral("node key =1") << 1 << 6
                                         << (QStringLiteral("unexpected identifier 'key'") + valueHint);
    QTest::newRow("adjacent strings") << QStringLiteral("node \"a\"\"b\"") << 1 << 9
                                      << QStringLiteral("expected whitespace, ';' or newline, found '\"'");
    QTest::newRow("adjacent slashdash") << QStringLiteral("node 1/-2") << 1 << 7
                                        << QStringLiteral("expected whitespace, ';' or newline, found '/'");
    QTest::newRow("reserved character") << QStringLiteral("node [1]") << 1 << 6
                                        << QStringLiteral("expected argument or property, found '['");
    QTest::newRow("control character") << QStringLiteral("node\u0001") << 1 << 5
                                       << QStringLiteral("expected whitespace, ';' or newline, found U+0001");
    QTest::newRow("dangling slashdash") << QStringLiteral("node /-") << 1 << 8
                                        << QStringLiteral("expected argument or property, found end of input");
    QTest::newRow("slashdash before newline") << QStringLiteral("/-\nnode") << 1 << 3
                                              << QStringLiteral("expected node name, found newline");
}

void TestKdlErrors::syntaxErrors()
{
    verifyError();
}

void TestKdlErrors::stringErrors_data()
{
    addColumns();
    QTest::newRow("unclosed string") << QStringLiteral("node\n  \"abc") << 2 << 3 << QStringLiteral("unclosed string");
    QTest::newRow("unclosed after escape") << QStringLiteral("node \"abc\\") << 1 << 6 << QStringLiteral("unclosed string");
    QTest::newRow("invalid escape") << QStringLiteral("node \"a\\qb\"") << 1 << 9
                                    << QStringLiteral("invalid escape character 'q' in string");
    QTest::newRow("escaped newline") << QStringLiteral("node \"a\\\nb\"") << 1 << 9
                                     << QStringLiteral("invalid escape character newline in string");
    QTest::newRow("unicode without brace") << QStringLiteral("node \"\\u1234\"") << 1 << 9
                                           << QStringLiteral("expected '{' after \\u, found '1'");
    QTest::newRow("empty unicode") << QStringLiteral("node \"\\u{}\"") << 1 << 10
                                   << QStringLiteral("expected hexadecimal digit in unicode escape, found '}'");
    QTest::newRow("too many digits") << QStringLiteral("node \"\\u{1234567}\"") << 1 << 16
                                     << QStringLiteral("expected '}' to close unicode escape, found '7'");
    QTest::newRow("non hex digit") << QStringLiteral("node \"\\u{12g}\"") << 1 << 12
                                   << QStringLiteral("expected '}' to close unicode escape, found 'g'");
    QTest::newRow("surrogate") << QStringLiteral("node \"\\u{D800}\"") << 1 << 10
                               << QStringLiteral("unicode escape is not a valid code point");
    QTest::newRow("beyond unicode") << QStringLiteral("node \"\\u{110000}\"") << 1 << 10
                                    << QStringLiteral("unicode escape is not a valid code point");
    QTest::newRow("unclosed raw") << QStringLiteral("node r\"abc") << 1 << 6 << QStringLiteral("unclosed raw string");
    QTest::newRow("unbalanced raw hashes") << QStringLiteral("node r##\"foo\"#") << 1 << 6 << QStringLiteral("unclosed raw string");
    QTest::newRow("extra raw hashes") << QStringLiteral("node r#\"abc\"##") << 1 << 14
                                      << QStringLiteral("expected whitespace, ';' or newline, found '#'");
    QTest::newRow("unclosed block comment") << QStringLiteral("node\n/* open") << 2 << 1 << QStringLiteral("unclosed block comment");
    QTest::newRow("unclosed nested comment") << QStringLiteral("/* a /* b */") << 1 << 1 << QStringLiteral("unclosed block comment");
}

void TestKdlErrors::stringErrors()
{
    verifyError();
}

void TestKdlErrors::numberErrors_data()
{
    addColumns();
    const QString overflow = QStringLiteral("integer does not fit into a signed 64-bit integer");
    QTest::newRow("letters after digits") << QStringLiteral("node 1abc") << 1 << 7 << QStringLiteral("unexpected character 'a' in number");
    QTest::newRow("dot without fraction") << QStringLiteral("node 1.") << 1 << 8 << QStringLiteral("expected digit after decimal point");
    QTest::newRow("dot before exponent") << QStringLiteral("node 1.e7") << 1 << 8 << QStringLiteral("expected digit after decimal point");
    QTest::newRow("underscore fraction") << QStringLiteral("node 1._7") << 1 << 8 << QStringLiteral("expected digit after decimal point");
    QTest::newRow("multiple dots") << QStringLiteral("node\n\n    bad 1.5.5") << 3 << 12
                                   << QStringLiteral("unexpected character '.' in number");
    QTest::newRow("multiple exponents") << QStringLiteral("node 1.0E10e10") << 1 << 12
                                        << QStringLiteral("unexpected character 'e' in number");
    QTest::newRow("empty exponent") << QStringLiteral("node 1e") << 1 << 8 << QStringLiteral("expected digit in exponent");
    QTest::newRow("signed empty exponent") << QStringLiteral("node 1e+") << 1 << 9 << QStringLiteral("expected digit in exponent");
    QTest::newRow("empty hex") << QStringLiteral("node 0x") << 1 << 8 << QStringLiteral("expected hexadecimal digit");
    QTest::newRow("hex underscore start") << QStringLiteral("node 0x_10") << 1 << 8 << QStringLiteral("expected hexadecimal digit");
    QTest::newRow("double x") << QStringLiteral("node 0xx10") << 1 << 8 << QStringLiteral("expected hexadecimal digit");
    QTest::newRow("bad hex digit") << QStringLiteral("node 0x10g10") << 1 << 10
                                   << QStringLiteral("invalid character 'g' in hexadecimal number");
    QTest::newRow("bad octal digit") << QStringLiteral("node 0o45678") << 1 << 12
                                     << QStringLiteral("invalid character '8' in octal number");
    QTest::newRow("bad binary start") << QStringLiteral("node 0bx01") << 1 << 8 << QStringLiteral("expected binary digit");
    QTest::newRow("bad binary digit") << QStringLiteral("node 0b102") << 1 << 10
                                      << QStringLiteral("invalid character '2' in binary number");
    QTest::newRow("decimal overflow") << QStringLiteral("node 9223372036854775808") << 1 << 6 << overflow;
    QTest::newRow("negative overflow") << QStringLiteral("node x=-9223372036854775809") << 1 << 8 << overflow;
    QTest::newRow("hex overflow") << QStringLiteral("node 0x8000000000000000") << 1 << 6 << overflow;
    QTest::newRow("binary overflow") << (QStringLiteral("node 0b1") + QString(63, u'0')) << 1 << 6 << overflow;
    QTest::newRow("underscore overflow") << QStringLiteral("node 1_000_000_000_000_000_000_000") << 1 << 6 << overflow;
}

void TestKdlErrors::numberErrors()
{
    verifyError();
}

void TestKdlErrors::structureErrors_data()
{
    addColumns();
    const QString afterChildren = QStringLiteral("expected ';' or newline after children block, found ");
    QTest::newRow("unclosed children") << QStringLiteral("node {\n  child\n") << 1 << 6 << QStringLiteral("unclosed children block");
    QTest::newRow("unmatched brace") << QStringLiteral("a\n}") << 2 << 1 << QStringLiteral("unexpected '}' without matching '{'");
    QTest::newRow("brace after entry") << QStringLiteral("node 1 }") << 1 << 8 << QStringLiteral("expected ';' or newline, found '}'");
    QTest::newRow("unterminated last child") << QStringLiteral("a {\n  b }\n") << 2 << 5
                                             << QStringLiteral("expected ';' or newline, found '}'");
    QTest::newRow("unterminated nested block") << QStringLiteral("a {\n  b {} }\n") << 2 << 8 << (afterChildren + QStringLiteral("'}'"));
    QTest::newRow("entry after children") << QStringLiteral("a { b; } c") << 1 << 10 << (afterChildren + QStringLiteral("'c'"));
    QTest::newRow("second children block") << QStringLiteral("a {} {}") << 1 << 6 << (afterChildren + QStringLiteral("'{'"));
    QTest::newRow("continuation garbage") << QStringLiteral("node \\ x") << 1 << 8
                                          << QStringLiteral("expected newline after line continuation, found 'x'");
    QTest::newRow("continuation at end") << QStringLiteral("node \\") << 1 << 7
                                         << QStringLiteral("expected newline after line continuation, found end of input");
    QTest::newRow("continuation between nodes")
        << QStringLiteral("a\n  \\// hey\n  b") << 2 << 3 << QStringLiteral("expected node name, found '\\'");
    QTest::newRow("empty statement") << QStringLiteral("node;;") << 1 << 6 << QStringLiteral("expected node name, found ';'");
}

void TestKdlErrors::structureErrors()
{
    verifyError();
}

void TestKdlErrors::errorToString()
{
    const auto result = parse(QStringLiteral("layout {\n    gaps 16 foo\n}"), QStringLiteral("/home/user/.config/konveyor/config.kdl"));
    QVERIFY(!result);
    QCOMPARE(result.error().toString(),
        QStringLiteral("/home/user/.config/konveyor/config.kdl:2:13: unexpected identifier 'foo'; values must be strings, numbers, true, "
                       "false or null"));
    const ParseError manual {QStringLiteral("message with %1 placeholder"), Location {QStringLiteral("a%2.kdl"), 3, 4}};
    QCOMPARE(manual.toString(), QStringLiteral("a%2.kdl:3:4: message with %1 placeholder"));
}

void TestKdlErrors::deepNestingIsRejected()
{
    const auto result = parseText(QStringLiteral("a {\n").repeated(100000));
    QVERIFY(!result);
    QCOMPARE(result.error().message, QStringLiteral("children blocks are nested too deeply"));
    QCOMPARE(result.error().location.line, 257);
    const auto allowed = parseText(QStringLiteral("a {\n").repeated(200) + QStringLiteral("}\n").repeated(200));
    QVERIFY2(allowed, describeFailure(allowed));
}

void TestKdlErrors::pathologicalInputs_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("many comment openers") << QStringLiteral("/*").repeated(100000);
    QTest::newRow("many raw hashes") << (QStringLiteral("node r") + QString(100000, u'#') + QStringLiteral("\"x\""));
    QTest::newRow("many quotes") << QString(100001, u'"');
    QTest::newRow("many slashdashes") << QStringLiteral("/-").repeated(100000);
    QTest::newRow("many continuations") << (QStringLiteral("node") + QStringLiteral(" \\\n").repeated(100000));
    QTest::newRow("lone surrogate") << (QStringLiteral("node ") + QChar(0xD800));
    QTest::newRow("nul character") << (QStringLiteral("node \"a\"") + QChar(u'\0'));
}

void TestKdlErrors::pathologicalInputs()
{
    QFETCH(QString, text);
    const auto result = parseText(text);
    if (!result) {
        QVERIFY(result.error().location.line >= 1);
        QVERIFY(result.error().location.column >= 1);
        QVERIFY(!result.error().message.isEmpty());
    }
}

void TestKdlErrors::randomInputNeverCrashes()
{
    const QString alphabet = QStringLiteral("abr#\"\\/*-+{}();=0123456789.eExob_ \n\r\t\u2028\uFEFFtruenul\u00e9\U0001F600");
    QRandomGenerator generator(42);
    for (int iteration = 0; iteration < 5000; ++iteration) {
        QString text;
        const int length = generator.bounded(80);
        for (int index = 0; index < length; ++index) {
            text.append(alphabet.at(generator.bounded(alphabet.size())));
        }
        const auto result = parseText(text);
        QVERIFY(result || (result.error().location.line >= 1 && result.error().location.column >= 1));
    }
}

QTEST_GUILESS_MAIN(TestKdlErrors)

#include "test_kdl_errors.moc"
