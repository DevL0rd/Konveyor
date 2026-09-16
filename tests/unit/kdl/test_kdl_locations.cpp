#include "kdltesthelpers.h"

#include <QTest>

using namespace Konveyor::Kdl;
using namespace Konveyor::Kdl::Testing;

namespace
{

bool hasLocation(const Location &location, int line, int column)
{
    if (location.line == line && location.column == column && location.file == testFileName) {
        return true;
    }
    qWarning("actual location %s:%d:%d, expected %d:%d", qPrintable(location.file), location.line, location.column, line, column);
    return false;
}

}

class TestKdlLocations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodesPropertiesAndValues();
    void childrenAndTypeAnnotations();
    void newlineVariants_data();
    void newlineVariants();
    void columnsCountCharacters();
    void afterMultilineConstructs();
    void fileNameIsPropagated();
};

void TestKdlLocations::nodesPropertiesAndValues()
{
    const auto document = parseText(QStringLiteral("first 1 key=\"v\"\n  second  (t)2.5 other=(u8)3"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    const Node &first = document->nodes[0];
    QVERIFY(hasLocation(first.location, 1, 1));
    QVERIFY(hasLocation(first.arguments[0].location, 1, 7));
    QVERIFY(hasLocation(first.properties[0].location, 1, 9));
    QVERIFY(hasLocation(first.properties[0].value.location, 1, 13));
    const Node &second = document->nodes[1];
    QVERIFY(hasLocation(second.location, 2, 3));
    QVERIFY(hasLocation(second.arguments[0].location, 2, 11));
    QVERIFY(hasLocation(second.properties[0].location, 2, 18));
    QVERIFY(hasLocation(second.properties[0].value.location, 2, 24));
}

void TestKdlLocations::childrenAndTypeAnnotations()
{
    const auto document = parseText(QStringLiteral("(typed)parent {\n    child1; child2 \"x\"\n\n\t(t)child3\n}"));
    QVERIFY2(document, describeFailure(document));
    const Node &parent = document->nodes.first();
    QVERIFY(hasLocation(parent.location, 1, 1));
    QCOMPARE(parent.children.size(), 3);
    QVERIFY(hasLocation(parent.children[0].location, 2, 5));
    QVERIFY(hasLocation(parent.children[1].location, 2, 13));
    QVERIFY(hasLocation(parent.children[1].arguments[0].location, 2, 20));
    QVERIFY(hasLocation(parent.children[2].location, 4, 2));
}

void TestKdlLocations::newlineVariants_data()
{
    QTest::addColumn<QString>("separator");
    QTest::newRow("lf") << QStringLiteral("\n");
    QTest::newRow("crlf") << QStringLiteral("\r\n");
    QTest::newRow("cr") << QStringLiteral("\r");
    QTest::newRow("next line") << QStringLiteral("\u0085");
    QTest::newRow("form feed") << QStringLiteral("\u000C");
    QTest::newRow("line separator") << QStringLiteral("\u2028");
    QTest::newRow("paragraph separator") << QStringLiteral("\u2029");
}

void TestKdlLocations::newlineVariants()
{
    QFETCH(QString, separator);
    const auto document
        = parseText(QStringLiteral("a") + separator + separator + QStringLiteral("  b 1") + separator + QStringLiteral("c"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 3);
    QVERIFY(hasLocation(document->nodes[1].location, 3, 3));
    QVERIFY(hasLocation(document->nodes[1].arguments[0].location, 3, 5));
    QVERIFY(hasLocation(document->nodes[2].location, 4, 1));
}

void TestKdlLocations::columnsCountCharacters()
{
    const auto document = parseText(QStringLiteral("😀🚀 \t\"日本\" é=1 \"\\u{1F600}\" x=2"));
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.name, QStringLiteral("😀🚀"));
    QVERIFY(hasLocation(node.arguments[0].location, 1, 5));
    QVERIFY(hasLocation(node.properties[0].location, 1, 10));
    QVERIFY(hasLocation(node.arguments[1].location, 1, 14));
    QVERIFY(hasLocation(node.properties[1].location, 1, 26));
}

void TestKdlLocations::afterMultilineConstructs()
{
    const auto document = parseText(QStringLiteral("a \"one\ntwo\" /* comment\n spanning */ 1 \\\n   2 r#\"raw\n\"# 3\nb"));
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 2);
    const Node &a = document->nodes[0];
    QCOMPARE(a.arguments.size(), 5);
    QVERIFY(hasLocation(a.arguments[1].location, 3, 14));
    QVERIFY(hasLocation(a.arguments[2].location, 4, 4));
    QVERIFY(hasLocation(a.arguments[3].location, 4, 6));
    QVERIFY(hasLocation(a.arguments[4].location, 5, 4));
    QVERIFY(hasLocation(document->nodes[1].location, 6, 1));
}

void TestKdlLocations::fileNameIsPropagated()
{
    const QString fileName = QStringLiteral("/tmp/some dir/config.kdl");
    const auto document = parse(QStringLiteral("node 1 key=2 {\n    child\n}"), fileName);
    QVERIFY2(document, describeFailure(document));
    const Node &node = document->nodes.first();
    QCOMPARE(node.location.file, fileName);
    QCOMPARE(node.arguments[0].location.file, fileName);
    QCOMPARE(node.properties[0].location.file, fileName);
    QCOMPARE(node.properties[0].value.location.file, fileName);
    QCOMPARE(node.children[0].location.file, fileName);
}

QTEST_GUILESS_MAIN(TestKdlLocations)

#include "test_kdl_locations.moc"
