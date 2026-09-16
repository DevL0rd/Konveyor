#include "kdltesthelpers.h"

#include <QTest>

#include <cmath>
#include <limits>

using namespace Konveyor::Kdl;
using namespace Konveyor::Kdl::Testing;

namespace
{

std::optional<Value> firstArgument(const QString &text)
{
    const auto document = parseText(QStringLiteral("node ") + text);
    if (!document || document->nodes.isEmpty() || document->nodes.first().arguments.isEmpty()) {
        return std::nullopt;
    }
    return document->nodes.first().arguments.first();
}

}

class TestKdlValues : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strings_data();
    void strings();
    void multilineStrings();
    void integers_data();
    void integers();
    void floats_data();
    void floats();
    void keywords();
    void valueAccessors();
};

void TestKdlValues::strings_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");
    QTest::newRow("plain") << QStringLiteral("\"hello\"") << QStringLiteral("hello");
    QTest::newRow("empty") << QStringLiteral("\"\"") << QString();
    QTest::newRow("simple escapes") << QStringLiteral(R"("\n\r\t\\\/\"\b\f")") << QStringLiteral("\n\r\t\\/\"\b\f");
    QTest::newRow("unicode escape short") << QStringLiteral(R"("\u{41}")") << QStringLiteral("A");
    QTest::newRow("unicode escape accented") << QStringLiteral(R"("\u{00e9}t\u{00E9}")") << QStringLiteral("été");
    QTest::newRow("unicode escape astral") << QStringLiteral(R"("\u{1F680}")") << QStringLiteral("🚀");
    QTest::newRow("unicode escape max") << QStringLiteral(R"("\u{10FFFF}")") << QString::fromUcs4(U"\U0010FFFF", 1);
    QTest::newRow("unicode escape zero") << QStringLiteral(R"("a\u{0}b")")
                                         << QString(QStringLiteral("a") + QChar(u'\0') + QStringLiteral("b"));
    QTest::newRow("literal unicode") << QStringLiteral("\"日本語 😀\"") << QStringLiteral("日本語 😀");
    QTest::newRow("slashes and braces") << QStringLiteral("\"a/b {c} (d) [e] =;\"") << QStringLiteral("a/b {c} (d) [e] =;");
    QTest::newRow("raw no hashes") << QStringLiteral(R"(r"\n will be literal")") << QStringLiteral(R"(\n will be literal)");
    QTest::newRow("raw one hash") << QStringLiteral(R"(r#"hello\n\r\asd"world"#)") << QStringLiteral(R"(hello\n\r\asd"world)");
    QTest::newRow("raw nested hashes") << QStringLiteral(R"(r###"a\n"##b"###)") << QStringLiteral(R"(a\n"##b)");
    QTest::newRow("raw empty") << QStringLiteral(R"(r"")") << QString();
    QTest::newRow("raw empty hashes") << QStringLiteral(R"(r##""##)") << QString();
    QTest::newRow("raw regex") << QStringLiteral(R"(r#"^org\.telegram\.desktop$"#)") << QStringLiteral(R"(^org\.telegram\.desktop$)");
}

void TestKdlValues::strings()
{
    QFETCH(QString, text);
    QFETCH(QString, expected);
    const std::optional<Value> value = firstArgument(text);
    QVERIFY(value.has_value());
    QVERIFY(value->isString());
    QCOMPARE(value->toString(), expected);
}

void TestKdlValues::multilineStrings()
{
    const auto document = parseText(QStringLiteral("node \"line one\nline two\r\nline three\" r#\"raw\none\"#\nnext \"x\""));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    const Node &node = document->nodes[0];
    QCOMPARE(node.arguments.size(), 2);
    QCOMPARE(node.arguments[0].toString(), QStringLiteral("line one\nline two\r\nline three"));
    QCOMPARE(node.arguments[1].toString(), QStringLiteral("raw\none"));
    QCOMPARE(document->nodes[1].location.line, 5);
    QCOMPARE(document->nodes[1].arguments[0].location.column, 6);
}

void TestKdlValues::integers_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<qint64>("expected");
    QTest::newRow("zero") << QStringLiteral("0") << qint64(0);
    QTest::newRow("positive") << QStringLiteral("16") << qint64(16);
    QTest::newRow("negative") << QStringLiteral("-12") << qint64(-12);
    QTest::newRow("explicit plus") << QStringLiteral("+12") << qint64(12);
    QTest::newRow("leading zero") << QStringLiteral("012") << qint64(12);
    QTest::newRow("negative zero") << QStringLiteral("-0") << qint64(0);
    QTest::newRow("underscores") << QStringLiteral("1_000_000") << qint64(1000000);
    QTest::newRow("trailing underscores") << QStringLiteral("1__0_") << qint64(10);
    QTest::newRow("max") << QStringLiteral("9223372036854775807") << std::numeric_limits<qint64>::max();
    QTest::newRow("min") << QStringLiteral("-9223372036854775808") << std::numeric_limits<qint64>::min();
    QTest::newRow("hex lower") << QStringLiteral("0xff") << qint64(255);
    QTest::newRow("hex upper") << QStringLiteral("0xABCDEF") << qint64(0xABCDEF);
    QTest::newRow("hex negative") << QStringLiteral("-0x10") << qint64(-16);
    QTest::newRow("hex plus underscore") << QStringLiteral("+0x1_0") << qint64(16);
    QTest::newRow("hex max") << QStringLiteral("0x7fff_ffff_ffff_ffff") << std::numeric_limits<qint64>::max();
    QTest::newRow("hex min") << QStringLiteral("-0x8000000000000000") << std::numeric_limits<qint64>::min();
    QTest::newRow("octal") << QStringLiteral("0o17") << qint64(15);
    QTest::newRow("octal negative") << QStringLiteral("-0o7_7") << qint64(-63);
    QTest::newRow("binary") << QStringLiteral("0b1010") << qint64(10);
    QTest::newRow("binary negative") << QStringLiteral("-0b1") << qint64(-1);
    QTest::newRow("binary underscore") << QStringLiteral("0b1_0_1") << qint64(5);
    QTest::newRow("binary max") << QStringLiteral("0b") + QString(63, u'1') << std::numeric_limits<qint64>::max();
}

void TestKdlValues::integers()
{
    QFETCH(QString, text);
    QFETCH(qint64, expected);
    const std::optional<Value> value = firstArgument(text);
    QVERIFY(value.has_value());
    QVERIFY(value->isInteger());
    QCOMPARE(value->toInteger(), expected);
}

void TestKdlValues::floats_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<double>("expected");
    QTest::newRow("simple") << QStringLiteral("1.5") << 1.5;
    QTest::newRow("negative") << QStringLiteral("-1.5") << -1.5;
    QTest::newRow("plus") << QStringLiteral("+2.25") << 2.25;
    QTest::newRow("zero") << QStringLiteral("0.0") << 0.0;
    QTest::newRow("exponent") << QStringLiteral("1.0e10") << 1.0e10;
    QTest::newRow("exponent without fraction") << QStringLiteral("1e10") << 1.0e10;
    QTest::newRow("upper exponent negative") << QStringLiteral("1E-3") << 1.0e-3;
    QTest::newRow("exponent plus") << QStringLiteral("2.5e+2") << 250.0;
    QTest::newRow("underscore in integer part") << QStringLiteral("1_1.0") << 11.0;
    QTest::newRow("underscore in fraction") << QStringLiteral("1.0_5") << 1.05;
    QTest::newRow("underscore in exponent") << QStringLiteral("1.0e-1_0") << 1.0e-10;
    QTest::newRow("proportion") << QStringLiteral("0.33333") << 0.33333;
    QTest::newRow("large exponent") << QStringLiteral("1.23E+300") << 1.23e300;
    QTest::newRow("subnormal") << QStringLiteral("1e-310") << 1e-310;
    const double infinity = std::numeric_limits<double>::infinity();
    QTest::newRow("overflow") << QStringLiteral("1.23E+1000") << infinity;
    QTest::newRow("negative overflow") << QStringLiteral("-1e999") << -infinity;
    QTest::newRow("huge exponent") << QStringLiteral("1e99999999999999999999999") << infinity;
    QTest::newRow("overflow without exponent") << QStringLiteral("1") + QString(400, u'0') + QStringLiteral(".0") << infinity;
    QTest::newRow("underflow") << QStringLiteral("1.23E-1000") << 0.0;
    QTest::newRow("underflow without exponent") << QStringLiteral("0.") + QString(400, u'0') + QStringLiteral("1") << 0.0;
}

void TestKdlValues::floats()
{
    QFETCH(QString, text);
    QFETCH(double, expected);
    const std::optional<Value> value = firstArgument(text);
    QVERIFY(value.has_value());
    QVERIFY(value->isFloat());
    QCOMPARE(value->toDouble(), expected);
    QCOMPARE(std::signbit(value->toDouble()), std::signbit(expected));
}

void TestKdlValues::keywords()
{
    const auto document = parseText(QStringLiteral("node true false null key=(t)null other=true"));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.arguments.size(), 3);
    QVERIFY(node.arguments[0].isBool());
    QVERIFY(node.arguments[0].toBool());
    QVERIFY(node.arguments[1].isBool());
    QVERIFY(!node.arguments[1].toBool());
    QVERIFY(node.arguments[2].isNull());
    const Property *key = node.property(QStringLiteral("key"));
    QVERIFY(key != nullptr);
    QVERIFY(key->value.isNull());
    QCOMPARE(key->value.typeAnnotation, std::optional<QString>(QStringLiteral("t")));
    QVERIFY(node.property(QStringLiteral("other"))->value.toBool());
}

void TestKdlValues::valueAccessors()
{
    const auto document = parseText(QStringLiteral("node 3 2.5 \"3\""));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QVERIFY(node.arguments[0].isNumber());
    QVERIFY(!node.arguments[0].isFloat());
    QCOMPARE(node.arguments[0].toDouble(), 3.0);
    QVERIFY(node.arguments[1].isNumber());
    QVERIFY(!node.arguments[1].isInteger());
    QVERIFY(!node.arguments[2].isNumber());
    QVERIFY(!node.arguments[2].isNull());
    QVERIFY(!node.arguments[2].isBool());
}

QTEST_GUILESS_MAIN(TestKdlValues)

#include "test_kdl_values.moc"
