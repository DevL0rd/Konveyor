#include "documenttesthelpers.h"

#include "config/decode.h"
#include "config/loader.h"
#include "document/nodecodec.h"
#include "document/nodepath.h"

#include <QFile>
#include <QRegularExpression>
#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;

namespace
{

QString readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString dumpValue(const Kdl::Value &value)
{
    const QString written = value.isNumber() ? QString::number(value.toDouble(), 'g', 17) : Config::toWritten(value);
    const QString kind = value.isString() ? QStringLiteral("s") : value.isNumber() ? QStringLiteral("n") : QStringLiteral("v");
    return QStringLiteral("(%1)%2:%3").arg(value.typeAnnotation.value_or(QString()), kind, written);
}

QString dumpNode(const Kdl::Node &node)
{
    QStringList parts {node.typeAnnotation.value_or(QString()) + node.name};
    for (const Kdl::Value &argument : node.arguments) {
        parts.append(dumpValue(argument));
    }
    for (const Kdl::Property &property : node.properties) {
        parts.append(property.name + QLatin1Char('=') + dumpValue(property.value));
    }
    QStringList children;
    for (const Kdl::Node &child : node.children) {
        children.append(dumpNode(child));
    }
    return parts.join(QLatin1Char(' ')) + QStringLiteral(" {") + children.join(QStringLiteral("; ")) + QLatin1Char('}');
}

QString dumpDocument(const Kdl::Document &document)
{
    QStringList nodes;
    for (const Kdl::Node &node : document.nodes) {
        nodes.append(dumpNode(node));
    }
    return nodes.join(QLatin1Char('\n'));
}

QString dumpText(const QString &text)
{
    const auto document = Kdl::parse(text, QStringLiteral("config.kdl"));
    if (!document) {
        return QStringLiteral("<invalid KDL: ") + document.error().toString() + QLatin1Char('>');
    }
    return dumpDocument(*document);
}

qsizetype indexOf(const QList<Kdl::Node> &siblings, const PathSegment &segment)
{
    qsizetype seen = 0;
    for (qsizetype index = 0; index < siblings.size(); ++index) {
        if (siblings.at(index).name == segment.name && seen++ == segment.index) {
            return index;
        }
    }
    return -1;
}

QList<Kdl::Node> &siblingsOf(Kdl::Document &document, const NodePath &path)
{
    QList<Kdl::Node> *siblings = &document.nodes;
    for (const PathSegment &segment : parentOf(path)) {
        siblings = &(*siblings)[indexOf(*siblings, segment)].children;
    }
    return *siblings;
}

QString defaultText()
{
    return readFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"));
}

QStringList comments(const QString &text)
{
    static const QRegularExpression comment(QStringLiteral("//.*$"), QRegularExpression::MultilineOption);
    QStringList result;
    for (auto iterator = comment.globalMatch(text); iterator.hasNext();) {
        result.append(iterator.next().captured().trimmed());
    }
    return result;
}

void addPaths(const QList<Kdl::Node> &nodes, const NodePath &parent, QStringList &paths)
{
    QHash<QString, qsizetype> seen;
    for (const Kdl::Node &node : nodes) {
        NodePath path = parent;
        path.append(PathSegment {node.name, seen[node.name]++});
        paths.append(formatPath(path));
        addPaths(node.children, path, paths);
    }
}

}

class TestSettingsDocumentRoundTrip : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writingBackAnyDefaultNodeKeepsItsMeaningAndComments_data();
    void writingBackAnyDefaultNodeKeepsItsMeaningAndComments();
    void removingAnyDefaultNodeRemovesOnlyThatNode_data();
    void removingAnyDefaultNodeRemovesOnlyThatNode();
    void movingAnyDefaultNodeSwapsOnlyItWithItsNeighbour_data();
    void movingAnyDefaultNodeSwapsOnlyItWithItsNeighbour();

private:
    static void addDefaultPaths();
};

void TestSettingsDocumentRoundTrip::addDefaultPaths()
{
    QTest::addColumn<QString>("path");
    const auto document = Kdl::parse(defaultText(), QStringLiteral("config.kdl"));
    QVERIFY(document);
    QStringList paths;
    addPaths(document->nodes, {}, paths);
    QVERIFY(paths.size() > 100);
    for (const QString &path : paths) {
        QTest::newRow(qPrintable(path)) << path;
    }
}

void TestSettingsDocumentRoundTrip::removingAnyDefaultNodeRemovesOnlyThatNode_data()
{
    addDefaultPaths();
}

void TestSettingsDocumentRoundTrip::removingAnyDefaultNodeRemovesOnlyThatNode()
{
    QFETCH(QString, path);
    const QString original = defaultText();
    auto expected = Kdl::parse(original, QStringLiteral("config.kdl"));
    const NodePath nodePath = *parsePath(path);
    QList<Kdl::Node> &siblings = siblingsOf(*expected, nodePath);
    siblings.removeAt(indexOf(siblings, nodePath.last()));
    ConfigDocument document(original);
    const EditResult result = document.remove(path);
    QVERIFY2(result, result ? "" : qPrintable(result.error()));
    QCOMPARE(dumpText(document.text()), dumpDocument(*expected));
}

void TestSettingsDocumentRoundTrip::movingAnyDefaultNodeSwapsOnlyItWithItsNeighbour_data()
{
    addDefaultPaths();
}

void TestSettingsDocumentRoundTrip::movingAnyDefaultNodeSwapsOnlyItWithItsNeighbour()
{
    QFETCH(QString, path);
    const QString original = defaultText();
    const NodePath nodePath = *parsePath(path);
    for (const int delta : {-1, 1}) {
        auto expected = Kdl::parse(original, QStringLiteral("config.kdl"));
        QList<Kdl::Node> &siblings = siblingsOf(*expected, nodePath);
        NodePath otherPath = nodePath;
        otherPath.last().index += delta;
        const qsizetype other = otherPath.last().index < 0 ? -1 : indexOf(siblings, otherPath.last());
        ConfigDocument document(original);
        const EditResult result = document.move(path, delta);
        if (other < 0) {
            QVERIFY(!result);
            QCOMPARE(document.text(), original);
            continue;
        }
        QVERIFY2(result, result ? "" : qPrintable(result.error()));
        QCOMPARE(*result, formatPath(otherPath));
        siblings.swapItemsAt(indexOf(siblings, nodePath.last()), other);
        QCOMPARE(dumpText(document.text()), dumpDocument(*expected));
        QCOMPARE(QStringList(comments(document.text())).size(), comments(original).size());
    }
}

void TestSettingsDocumentRoundTrip::writingBackAnyDefaultNodeKeepsItsMeaningAndComments_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("whole");
    const auto document
        = Kdl::parse(readFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl")), QStringLiteral("config.kdl"));
    QVERIFY(document);
    QStringList paths;
    addPaths(document->nodes, {}, paths);
    QVERIFY(paths.size() > 100);
    for (const QString &path : paths) {
        QTest::newRow(qPrintable(path + QStringLiteral(" whole"))) << path << true;
        QTest::newRow(qPrintable(path + QStringLiteral(" head"))) << path << false;
    }
}

void TestSettingsDocumentRoundTrip::writingBackAnyDefaultNodeKeepsItsMeaningAndComments()
{
    QFETCH(QString, path);
    QFETCH(bool, whole);
    const QString original = readFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"));
    ConfigDocument document(original);
    QVariantMap node = nodeToVariant(*document.find(path));
    if (!whole) {
        node.remove(QStringLiteral("children"));
    }
    const EditResult result = document.setNode(path, node);
    QVERIFY2(result, result ? "" : qPrintable(result.error()));
    QCOMPARE(dumpText(document.text()), dumpText(original));
    QCOMPARE(comments(document.text()), comments(original));
    const auto loaded = Config::loadString(document.text(), QStringLiteral("config.kdl"));
    QVERIFY2(loaded, loaded ? "" : qPrintable(loaded.error().toString()));
}

QTEST_GUILESS_MAIN(TestSettingsDocumentRoundTrip)

#include "test_settings_document_roundtrip.moc"
