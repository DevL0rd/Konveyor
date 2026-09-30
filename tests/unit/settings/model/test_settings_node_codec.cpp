#include "document/nodecodec.h"
#include "document/nodepath.h"
#include "documenttesthelpers.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

class TestSettingsNodeCodec : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesPaths_data();
    void parsesPaths();
    void rejectsBadPaths_data();
    void rejectsBadPaths();
    void findsNodes();
    void writesNumbers_data();
    void writesNumbers();
    void writesScalars();
    void writesStrings_data();
    void writesStrings();
    void quotesIdentifiers_data();
    void quotesIdentifiers();
    void writesProperties();
    void keepsExistingPropertyOrder();
    void writesNodes();
    void readsNodes();
    void roundTripsThroughDocument();
};

void TestSettingsNodeCodec::parsesPaths_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("formatted");
    QTest::addColumn<int>("segments");
    QTest::addColumn<int>("lastIndex");
    QTest::newRow("empty") << QString() << QString() << 0 << -1;
    QTest::newRow("one") << QStringLiteral("layout") << QStringLiteral("layout") << 1 << 0;
    QTest::newRow("zero index dropped") << QStringLiteral("output#0/layout") << QStringLiteral("output/layout") << 2 << 0;
    QTest::newRow("index") << QStringLiteral("window-rule#12") << QStringLiteral("window-rule#12") << 1 << 12;
    QTest::newRow("last hash wins") << QStringLiteral("a#1#2") << QStringLiteral("a#1#2") << 1 << 2;
    QTest::newRow("plus in name") << QStringLiteral("binds/Mod+Shift+T") << QStringLiteral("binds/Mod+Shift+T") << 2 << 0;
}

void TestSettingsNodeCodec::parsesPaths()
{
    QFETCH(QString, text);
    QFETCH(QString, formatted);
    QFETCH(int, segments);
    QFETCH(int, lastIndex);
    const auto path = parsePath(text);
    QVERIFY2(path.has_value(), path ? "" : qPrintable(path.error()));
    QCOMPARE(path->size(), segments);
    QCOMPARE(formatPath(*path), formatted);
    if (segments > 0) {
        QCOMPARE(path->last().index, lastIndex);
    }
}

void TestSettingsNodeCodec::rejectsBadPaths_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("error");
    QTest::newRow("letters") << QStringLiteral("a#x") << QStringLiteral("invalid index in path segment a#x");
    QTest::newRow("negative") << QStringLiteral("a/b#-1") << QStringLiteral("invalid index in path segment b#-1");
    QTest::newRow("missing number") << QStringLiteral("a#") << QStringLiteral("invalid index in path segment a#");
    QTest::newRow("fraction") << QStringLiteral("a#1.5") << QStringLiteral("invalid index in path segment a#1.5");
    QTest::newRow("empty segment") << QStringLiteral("a//b") << QStringLiteral("empty path segment in a//b");
    QTest::newRow("trailing slash") << QStringLiteral("a/") << QStringLiteral("empty path segment in a/");
    QTest::newRow("only index") << QStringLiteral("#1") << QStringLiteral("empty path segment in #1");
}

void TestSettingsNodeCodec::rejectsBadPaths()
{
    QFETCH(QString, text);
    QFETCH(QString, error);
    const auto path = parsePath(text);
    QVERIFY(!path.has_value());
    QCOMPARE(path.error(), error);
    ConfigDocument document(QStringLiteral("a {\n}\n"));
    QCOMPARE(document.setNode(text, leaf(QStringLiteral("x"))).error(), error);
    QCOMPARE(document.remove(text).error(), error);
    QCOMPARE(document.append(text, leaf(QStringLiteral("x"))).error(), error);
    QCOMPARE(document.move(text, 1).error(), error);
    QCOMPARE(document.find(text), nullptr);
    QCOMPARE(document.text(), QStringLiteral("a {\n}\n"));
}

void TestSettingsNodeCodec::findsNodes()
{
    const auto parsed = Kdl::parse(QStringLiteral("a 1\nb\na 2 {\n    c\n    c 3\n}\n"), QStringLiteral("t.kdl"));
    QVERIFY(parsed.has_value());
    QCOMPARE(countNamed(parsed->nodes, QStringLiteral("a")), 2);
    QCOMPARE(countNamed(parsed->nodes, QStringLiteral("z")), 0);
    QCOMPARE(findNode(*parsed, *parsePath(QStringLiteral("a#1/c#1")))->arguments.first().toInteger(), 3);
    QCOMPARE(findNode(*parsed, *parsePath(QStringLiteral("a#1/c#2"))), nullptr);
    QCOMPARE(findNode(*parsed, *parsePath(QStringLiteral("b/c"))), nullptr);
    QCOMPARE(&childrenOf(*parsed, nullptr), &parsed->nodes);
    QCOMPARE(Settings::findChild(parsed->nodes, PathSegment {QStringLiteral("a"), 1})->arguments.first().toInteger(), 2);
}

void TestSettingsNodeCodec::writesNumbers_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("written");
    QTest::newRow("int") << QVariant(16) << QStringLiteral("16");
    QTest::newRow("negative") << QVariant(-3) << QStringLiteral("-3");
    QTest::newRow("whole double") << QVariant(1.0) << QStringLiteral("1");
    QTest::newRow("fraction") << QVariant(0.5) << QStringLiteral("0.5");
    QTest::newRow("float noise") << QVariant(0.1 + 0.2) << QStringLiteral("0.3");
    QTest::newRow("third") << QVariant(1.0 / 3.0) << QStringLiteral("0.333333333333");
    QTest::newRow("long long") << QVariant(qint64(1) << 40) << QStringLiteral("1099511627776");
    QTest::newRow("uint") << QVariant(4000000000U) << QStringLiteral("4000000000");
    QTest::newRow("float") << QVariant(0.25F) << QStringLiteral("0.25");
    QTest::newRow("huge") << QVariant(1e20) << QStringLiteral("1e+20");
    QTest::newRow("tiny") << QVariant(1e-7) << QStringLiteral("1e-07");
    QTest::newRow("negative zero") << QVariant(-0.0) << QStringLiteral("0");
}

void TestSettingsNodeCodec::writesNumbers()
{
    QFETCH(QVariant, value);
    QFETCH(QString, written);
    QCOMPARE(writeValue(value), written);
    const auto parsed = Kdl::parse(QStringLiteral("n ") + written, QStringLiteral("t.kdl"));
    QVERIFY2(parsed.has_value(), qPrintable(written));
    QCOMPARE(parsed->nodes.first().arguments.first().toDouble(), value.toDouble());
}

void TestSettingsNodeCodec::writesScalars()
{
    QCOMPARE(writeValue(QVariant(true)), QStringLiteral("true"));
    QCOMPARE(writeValue(QVariant(false)), QStringLiteral("false"));
    QCOMPARE(writeValue(QVariant::fromValue(nullptr)), QStringLiteral("null"));
    QCOMPARE(writeValue(QVariant()), QStringLiteral("null"));
}

void TestSettingsNodeCodec::writesStrings_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("written");
    QTest::newRow("plain") << QStringLiteral("never") << QStringLiteral("\"never\"");
    QTest::newRow("empty") << QString() << QStringLiteral("\"\"");
    QTest::newRow("newline") << QStringLiteral("a\nb") << QStringLiteral("\"a\\nb\"");
    QTest::newRow("tab") << QStringLiteral("a\tb") << QStringLiteral("\"a\\tb\"");
    QTest::newRow("backslash") << QStringLiteral("^org\\.kde$") << QStringLiteral("r#\"^org\\.kde$\"#");
    QTest::newRow("quote") << QStringLiteral("say \"hi\"") << QStringLiteral("r#\"say \"hi\"\"#");
    QTest::newRow("hash quote") << QStringLiteral("a\"#b") << QStringLiteral("r##\"a\"#b\"##");
    QTest::newRow("unicode") << QStringLiteral("ünï 🎮") << QStringLiteral("\"ünï 🎮\"");
    QTest::newRow("number text") << QStringLiteral("12") << QStringLiteral("\"12\"");
}

void TestSettingsNodeCodec::writesStrings()
{
    QFETCH(QString, value);
    QFETCH(QString, written);
    QCOMPARE(writeValue(value), written);
    const auto parsed = Kdl::parse(QStringLiteral("n ") + written, QStringLiteral("t.kdl"));
    QVERIFY2(parsed.has_value(), qPrintable(written));
    QCOMPARE(parsed->nodes.first().arguments.first().toString(), value);
}

void TestSettingsNodeCodec::quotesIdentifiers_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("written");
    QTest::newRow("bare") << QStringLiteral("focus-ring") << QStringLiteral("focus-ring");
    QTest::newRow("bind") << QStringLiteral("Mod+Shift+T") << QStringLiteral("Mod+Shift+T");
    QTest::newRow("true") << QStringLiteral("true") << QStringLiteral("\"true\"");
    QTest::newRow("false") << QStringLiteral("false") << QStringLiteral("\"false\"");
    QTest::newRow("null") << QStringLiteral("null") << QStringLiteral("\"null\"");
    QTest::newRow("number") << QStringLiteral("1") << QStringLiteral("\"1\"");
    QTest::newRow("signed number") << QStringLiteral("-1") << QStringLiteral("\"-1\"");
    QTest::newRow("space") << QStringLiteral("a b") << QStringLiteral("\"a b\"");
    QTest::newRow("slash") << QStringLiteral("Mod+/") << QStringLiteral("\"Mod+/\"");
    QTest::newRow("brace") << QStringLiteral("a{") << QStringLiteral("\"a{\"");
    QTest::newRow("equals") << QStringLiteral("a=b") << QStringLiteral("\"a=b\"");
    QTest::newRow("empty") << QString() << QStringLiteral("\"\"");
}

void TestSettingsNodeCodec::quotesIdentifiers()
{
    QFETCH(QString, name);
    QFETCH(QString, written);
    QCOMPARE(writeIdentifier(name), written);
    const auto parsed = Kdl::parse(written + QStringLiteral(" k=1"), QStringLiteral("t.kdl"));
    QVERIFY2(parsed.has_value(), qPrintable(written));
    QCOMPARE(parsed->nodes.first().name, name);
    QCOMPARE(writeNodeHead(leaf(QStringLiteral("n"), {}, {{name, 1}})), QStringLiteral("n ") + written + QStringLiteral("=1"));
}

void TestSettingsNodeCodec::writesProperties()
{
    const QVariantMap sorted
        = leaf(QStringLiteral("gradient"), {}, {{QStringLiteral("to"), QStringLiteral("#fff")}, {QStringLiteral("angle"), 45}});
    QCOMPARE(writeNodeHead(sorted), QStringLiteral("gradient angle=45 to=\"#fff\""));
    QVariantMap ordered = leaf(QStringLiteral("gradient"));
    ordered.insert(QStringLiteral("props"),
        QVariantList {
            QVariant(QVariantList {QStringLiteral("to"), QStringLiteral("#fff")}), QVariant(QVariantList {QStringLiteral("angle"), 45})});
    QCOMPARE(writeNodeHead(ordered), QStringLiteral("gradient to=\"#fff\" angle=45"));
    QCOMPARE(writeNodeHead(leaf(QStringLiteral("spawn"), {QStringLiteral("a"), 2, true})), QStringLiteral("spawn \"a\" 2 true"));
}

void TestSettingsNodeCodec::keepsExistingPropertyOrder()
{
    const QString text = QStringLiteral("match title=\"x\" app-id=\"y\"\n");
    const QVariantMap next = leaf(QStringLiteral("match"), {},
        {{QStringLiteral("app-id"), QStringLiteral("z")}, {QStringLiteral("title"), QStringLiteral("x")},
            {QStringLiteral("is-floating"), true}});
    QCOMPARE(edited(text, [&](ConfigDocument &d) { return d.setNode(QStringLiteral("match"), next); }),
        QStringLiteral("match title=\"x\" app-id=\"z\" is-floating=true\n"));
    const QVariantMap fewer = leaf(QStringLiteral("match"), {}, {{QStringLiteral("app-id"), QStringLiteral("y")}});
    QCOMPARE(
        edited(text, [&](ConfigDocument &d) { return d.setNode(QStringLiteral("match"), fewer); }), QStringLiteral("match app-id=\"y\"\n"));
}

void TestSettingsNodeCodec::writesNodes()
{
    QCOMPARE(writeNode(leaf(QStringLiteral("gaps"), {4}), QStringLiteral("    ")), QStringLiteral("gaps 4"));
    QCOMPARE(writeNode(block(QStringLiteral("struts"), {}), QString()), QStringLiteral("struts {}"));
    QCOMPARE(writeNode(block(QStringLiteral("w"), {leaf(QStringLiteral("proportion"), {0.5})}), QString()),
        QStringLiteral("w { proportion 0.5; }"));
    QCOMPARE(writeNode(block(QStringLiteral("r"), {block(QStringLiteral("inner"), {})}), QStringLiteral("  ")),
        QStringLiteral("r {\n      inner {}\n  }"));
    QCOMPARE(writeNode(block(QStringLiteral("b"), {leaf(QStringLiteral("on")), leaf(QStringLiteral("width"), {4})}, {QStringLiteral("x")}),
                 QString()),
        QStringLiteral("b \"x\" {\n    on\n    width 4\n}"));
}

void TestSettingsNodeCodec::readsNodes()
{
    const auto parsed
        = Kdl::parse(QStringLiteral("n 1 2.5 \"s\" true null k=r#\"a\\b\"# {\n    c\n}\nleaf\nempty {}\n"), QStringLiteral("t.kdl"));
    QVERIFY(parsed.has_value());
    const QVariantMap node = nodeToVariant(parsed->nodes.first());
    QCOMPARE(node.value(QStringLiteral("name")).toString(), QStringLiteral("n"));
    const QVariantList args = node.value(QStringLiteral("args")).toList();
    QCOMPARE(args.size(), 5);
    QCOMPARE(args.at(0).typeId(), QMetaType::LongLong);
    QCOMPARE(args.at(1).toDouble(), 2.5);
    QCOMPARE(args.at(2).toString(), QStringLiteral("s"));
    QCOMPARE(args.at(3).toBool(), true);
    QCOMPARE(args.at(4).typeId(), QMetaType::Nullptr);
    QCOMPARE(node.value(QStringLiteral("props")).toMap().value(QStringLiteral("k")).toString(), QStringLiteral("a\\b"));
    QCOMPARE(node.value(QStringLiteral("children")).toList().size(), 1);
    QVERIFY(!nodeToVariant(parsed->nodes.at(1)).contains(QStringLiteral("children")));
    QVERIFY(nodeToVariant(parsed->nodes.at(2)).contains(QStringLiteral("children")));
}

void TestSettingsNodeCodec::roundTripsThroughDocument()
{
    const QString text
        = QStringLiteral("window-rule {\n    match app-id=r#\"^org\\.kde\\.konsole$\"# title=\"a\\\"b\"\n    opacity 0.9\n}\n");
    const ConfigDocument document(text);
    const QVariantMap node = nodeToVariant(*document.find(QStringLiteral("window-rule")));
    QCOMPARE(edited(text, [&](ConfigDocument &d) { return d.setNode(QStringLiteral("window-rule"), node); }),
        QStringLiteral("window-rule {\n    match app-id=r#\"^org\\.kde\\.konsole$\"# title=r#\"a\"b\"#\n    opacity 0.9\n}\n"));
}

QTEST_GUILESS_MAIN(TestSettingsNodeCodec)
#include "test_settings_node_codec.moc"
