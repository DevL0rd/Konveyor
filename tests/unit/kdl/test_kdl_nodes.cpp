#include "kdltesthelpers.h"

#include <QTest>

using namespace Konveyor::Kdl;
using namespace Konveyor::Kdl::Testing;

class TestKdlNodes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyDocuments_data();
    void emptyDocuments();
    void nodeNames_data();
    void nodeNames();
    void argumentsAndPropertiesInAnyOrder();
    void duplicatePropertiesLastOneWins();
    void quotedPropertyNames();
    void childrenBlocks();
    void nestedChildren();
    void nodeCounts_data();
    void nodeCounts();
    void lineContinuations();
    void typeAnnotations();
    void comments();
    void slashdashNodes();
    void slashdashEntries();
    void slashdashChildren();
    void unicodeWhitespace();
    void nodeHelpers();
};

void TestKdlNodes::emptyDocuments_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("empty") << QString();
    QTest::newRow("whitespace") << QStringLiteral("  \t \n\n  ");
    QTest::newRow("line comment") << QStringLiteral("// nothing here");
    QTest::newRow("block comment") << QStringLiteral("/* nothing */\n");
    QTest::newRow("nested block comment") << QStringLiteral("/* outer /* inner */ still outer */");
    QTest::newRow("slashdash only") << QStringLiteral("/-node 1 2 {\n    child\n}\n");
    QTest::newRow("byte order mark") << QStringLiteral("\uFEFF\n");
}

void TestKdlNodes::emptyDocuments()
{
    QFETCH(QString, text);
    const auto document = parseText(text);
    QVERIFY2(document, describeFailure(document));
    QVERIFY(document->nodes.isEmpty());
}

void TestKdlNodes::nodeNames_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("name");
    QTest::newRow("plain") << QStringLiteral("node") << QStringLiteral("node");
    QTest::newRow("dashes") << QStringLiteral("focus-ring") << QStringLiteral("focus-ring");
    QTest::newRow("lone minus") << QStringLiteral("-") << QStringLiteral("-");
    QTest::newRow("lone plus") << QStringLiteral("+") << QStringLiteral("+");
    QTest::newRow("double dash digit") << QStringLiteral("--1") << QStringLiteral("--1");
    QTest::newRow("plus letter") << QStringLiteral("+abc") << QStringLiteral("+abc");
    QTest::newRow("leading dot") << QStringLiteral(".hidden") << QStringLiteral(".hidden");
    QTest::newRow("key combination") << QStringLiteral("Mod+Shift+Slash") << QStringLiteral("Mod+Shift+Slash");
    QTest::newRow("digits inside") << QStringLiteral("XF86AudioRaiseVolume") << QStringLiteral("XF86AudioRaiseVolume");
    QTest::newRow("accented") << QStringLiteral("héllo") << QStringLiteral("héllo");
    QTest::newRow("cjk") << QStringLiteral("日本語") << QStringLiteral("日本語");
    QTest::newRow("astral") << QStringLiteral("😀smile") << QStringLiteral("😀smile");
    QTest::newRow("symbols") << QStringLiteral("foo123~!@#$%^&*.:'|?+") << QStringLiteral("foo123~!@#$%^&*.:'|?+");
    QTest::newRow("single r") << QStringLiteral("r") << QStringLiteral("r");
    QTest::newRow("r hash identifier") << QStringLiteral("r#abc") << QStringLiteral("r#abc");
    QTest::newRow("quoted keyword") << QStringLiteral("\"true\"") << QStringLiteral("true");
    QTest::newRow("quoted number") << QStringLiteral("\"123\"") << QStringLiteral("123");
    QTest::newRow("quoted spaces") << QStringLiteral("\"with space\"") << QStringLiteral("with space");
    QTest::newRow("raw string") << QStringLiteral("r#\"raw \"name\"\"#") << QStringLiteral("raw \"name\"");
    QTest::newRow("empty quoted") << QStringLiteral("\"\"") << QString();
}

void TestKdlNodes::nodeNames()
{
    QFETCH(QString, text);
    QFETCH(QString, name);
    const auto document = parseText(text);
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 1);
    QCOMPARE(document->nodes.first().name, name);
}

void TestKdlNodes::argumentsAndPropertiesInAnyOrder()
{
    const auto document = parseText(QStringLiteral("node 1 key=\"value\" 2 other=true \"three\""));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 1);
    const Node &node = document->nodes.first();
    QCOMPARE(node.arguments.size(), 3);
    QCOMPARE(node.arguments[0].toInteger(), 1);
    QCOMPARE(node.arguments[1].toInteger(), 2);
    QCOMPARE(node.arguments[2].toString(), QStringLiteral("three"));
    QCOMPARE(node.properties.size(), 2);
    QCOMPARE(node.properties[0].name, QStringLiteral("key"));
    QCOMPARE(node.properties[0].value.toString(), QStringLiteral("value"));
    QCOMPARE(node.properties[1].name, QStringLiteral("other"));
    QVERIFY(node.properties[1].value.toBool());
}

void TestKdlNodes::duplicatePropertiesLastOneWins()
{
    const auto document = parseText(QStringLiteral("node a=1 b=2 a=3"));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.properties.size(), 2);
    QCOMPARE(node.properties[0].name, QStringLiteral("a"));
    QCOMPARE(node.properties[0].value.toInteger(), 3);
    QCOMPARE(node.properties[0].location.column, 14);
    QCOMPARE(node.properties[1].name, QStringLiteral("b"));
    QCOMPARE(node.property(QStringLiteral("a"))->value.toInteger(), 3);
}

void TestKdlNodes::quotedPropertyNames()
{
    const auto document = parseText(QStringLiteral("node \"key with space\"=1 r\"raw\"=2 \"true\"=3 \"1\"=4"));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.properties.size(), 4);
    QCOMPARE(node.property(QStringLiteral("key with space"))->value.toInteger(), 1);
    QCOMPARE(node.property(QStringLiteral("raw"))->value.toInteger(), 2);
    QCOMPARE(node.property(QStringLiteral("true"))->value.toInteger(), 3);
    QCOMPARE(node.property(QStringLiteral("1"))->value.toInteger(), 4);
    QVERIFY(node.arguments.isEmpty());
}

void TestKdlNodes::childrenBlocks()
{
    const auto document = parseText(QStringLiteral("parent { child1; child2; }\nsecond{\nchild3\n}\n\"x\" 1{\n}\nempty {}\n"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 4);
    const Node &parent = document->nodes[0];
    QCOMPARE(parent.children.size(), 2);
    QCOMPARE(parent.children[0].name, QStringLiteral("child1"));
    QCOMPARE(parent.children[1].name, QStringLiteral("child2"));
    QCOMPARE(document->nodes[1].children.size(), 1);
    QCOMPARE(document->nodes[1].children[0].name, QStringLiteral("child3"));
    QCOMPARE(document->nodes[2].arguments.size(), 1);
    QVERIFY(document->nodes[2].children.isEmpty());
    QVERIFY(document->nodes[3].children.isEmpty());
}

void TestKdlNodes::nestedChildren()
{
    const auto document = parseText(QStringLiteral("a {\n    b {\n        c {\n            d 1\n        }\n    }\n    e\n};\nf"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    const Node &a = document->nodes[0];
    QCOMPARE(a.children.size(), 2);
    QCOMPARE(a.children[0].children.size(), 1);
    QCOMPARE(a.children[0].children[0].children[0].name, QStringLiteral("d"));
    QCOMPARE(a.children[0].children[0].children[0].arguments[0].toInteger(), 1);
    QCOMPARE(a.children[1].name, QStringLiteral("e"));
    QCOMPARE(document->nodes[1].name, QStringLiteral("f"));
}

void TestKdlNodes::nodeCounts_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("count");
    QTest::newRow("semicolons") << QStringLiteral("a; b; c;") << 3;
    QTest::newRow("tight semicolons") << QStringLiteral("a;b;c") << 3;
    QTest::newRow("lf") << QStringLiteral("a\nb\n") << 2;
    QTest::newRow("crlf") << QStringLiteral("a\r\nb\r\n") << 2;
    QTest::newRow("cr") << QStringLiteral("a\rb") << 2;
    QTest::newRow("unicode newlines") << QStringLiteral("a\u0085b\u000Cc\u2028d\u2029e") << 5;
    QTest::newRow("blank lines") << QStringLiteral("\n\na\n\n\nb\n\n") << 2;
    QTest::newRow("comment terminator") << QStringLiteral("a 1 // trailing\nb") << 2;
    QTest::newRow("eof terminator") << QStringLiteral("a 1") << 1;
    QTest::newRow("children then semicolon") << QStringLiteral("a {\n  b\n};c") << 2;
}

void TestKdlNodes::nodeCounts()
{
    QFETCH(QString, text);
    QFETCH(int, count);
    const auto document = parseText(text);
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), count);
}

void TestKdlNodes::lineContinuations()
{
    const auto document = parseText(QStringLiteral("node 1 \\\n    2 \\   // comment\n    key=3 \\\r\n    4\nnext"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    const Node &node = document->nodes[0];
    QCOMPARE(node.arguments.size(), 3);
    QCOMPARE(node.arguments[2].toInteger(), 4);
    QCOMPARE(node.property(QStringLiteral("key"))->value.toInteger(), 3);
    QCOMPARE(document->nodes[1].name, QStringLiteral("next"));
}

void TestKdlNodes::typeAnnotations()
{
    const auto document = parseText(
        QStringLiteral("(published)date (u8)1 key=(date)\"2020-01-01\" (\"quoted type\")\"x\" \"plain\"\n(r#\"raw\"#)\"name\""));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    const Node &node = document->nodes[0];
    QCOMPARE(node.typeAnnotation, std::optional<QString>(QStringLiteral("published")));
    QCOMPARE(node.arguments.size(), 3);
    QCOMPARE(node.arguments[0].typeAnnotation, std::optional<QString>(QStringLiteral("u8")));
    QCOMPARE(node.arguments[1].typeAnnotation, std::optional<QString>(QStringLiteral("quoted type")));
    QVERIFY(!node.arguments[2].typeAnnotation.has_value());
    QCOMPARE(node.property(QStringLiteral("key"))->value.typeAnnotation, std::optional<QString>(QStringLiteral("date")));
    QCOMPARE(document->nodes[1].name, QStringLiteral("name"));
    QCOMPARE(document->nodes[1].typeAnnotation, std::optional<QString>(QStringLiteral("raw")));
}

void TestKdlNodes::comments()
{
    const auto document
        = parseText(QStringLiteral("// leading\nnode /* inline */ 1 /* nested /* deeper */ still */ 2 /*\nmultiline\n*/ 3 // trailing\n"
                                   "/* between\nnodes */ other"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    QCOMPARE(document->nodes[0].arguments.size(), 3);
    QCOMPARE(document->nodes[0].arguments[2].toInteger(), 3);
    QCOMPARE(document->nodes[1].name, QStringLiteral("other"));
}

void TestKdlNodes::slashdashNodes()
{
    const auto document = parseText(
        QStringLiteral("/-first\nsecond\n/- third {\n    child\n}\nfourth {\n    /-hidden 1\n    visible\n    /- (t)gone\n}\n"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    QCOMPARE(document->nodes[0].name, QStringLiteral("second"));
    const Node &fourth = document->nodes[1];
    QCOMPARE(fourth.name, QStringLiteral("fourth"));
    QCOMPARE(fourth.children.size(), 1);
    QCOMPARE(fourth.children[0].name, QStringLiteral("visible"));
}

void TestKdlNodes::slashdashEntries()
{
    const auto document = parseText(QStringLiteral("node /-1 2 /- \"skip\" key=1 /-key=2 /-other=3 (t)4 /-(t)5 /-r#\"raw\"# 6"));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.arguments.size(), 3);
    QCOMPARE(node.arguments[0].toInteger(), 2);
    QCOMPARE(node.arguments[1].toInteger(), 4);
    QCOMPARE(node.arguments[2].toInteger(), 6);
    QCOMPARE(node.properties.size(), 1);
    QCOMPARE(node.property(QStringLiteral("key"))->value.toInteger(), 1);
    QCOMPARE(node.property(QStringLiteral("other")), nullptr);
}

void TestKdlNodes::slashdashChildren()
{
    const auto document = parseText(QStringLiteral("a 1 /-{\n    child\n}\nb /- {\n}\nc/-{\n    d\n}\n"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 3);
    for (const Node &node : document->nodes) {
        QVERIFY(node.children.isEmpty());
    }
    QCOMPARE(document->nodes[0].arguments.size(), 1);
}

void TestKdlNodes::unicodeWhitespace()
{
    const auto document = parseText(QStringLiteral("\uFEFFnode\u3000 1\u00A0\u2003key=2\t\u205F3"));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.name, QStringLiteral("node"));
    QCOMPARE(node.arguments.size(), 2);
    QCOMPARE(node.property(QStringLiteral("key"))->value.toInteger(), 2);
}

void TestKdlNodes::nodeHelpers()
{
    const auto document
        = parseText(QStringLiteral("binds {\n    Mod+T { spawn \"foot\"; }\n    Mod+Q a=1 { close-window; }\n    Mod+T repeat=false\n}\n"));
    QVERIFY2(document, describeFailure(document));
    const Node &binds = document->nodes.first();
    QVERIFY(binds.child(QStringLiteral("missing")) == nullptr);
    const Node *first = binds.child(QStringLiteral("Mod+T"));
    QVERIFY(first != nullptr);
    QCOMPARE(first->children.first().name, QStringLiteral("spawn"));
    const QList<const Node *> matches = binds.childrenNamed(QStringLiteral("Mod+T"));
    QCOMPARE(matches.size(), 2);
    QCOMPARE(matches[0], first);
    QCOMPARE(matches[1], &binds.children[2]);
    QVERIFY(binds.childrenNamed(QStringLiteral("nothing")).isEmpty());
    QVERIFY(binds.children[1].property(QStringLiteral("a")) != nullptr);
    QVERIFY(binds.children[1].property(QStringLiteral("b")) == nullptr);
    QVERIFY(!binds.children[2].property(QStringLiteral("repeat"))->value.toBool());
}

QTEST_GUILESS_MAIN(TestKdlNodes)

#include "test_kdl_nodes.moc"
