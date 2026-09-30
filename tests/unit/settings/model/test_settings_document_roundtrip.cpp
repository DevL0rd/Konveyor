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

QString dumpText(const QString &text)
{
    const auto document = Kdl::parse(text, QStringLiteral("config.kdl"));
    if (!document) {
        return QStringLiteral("<invalid KDL: ") + document.error().toString() + QLatin1Char('>');
    }
    QStringList nodes;
    for (const Kdl::Node &node : document->nodes) {
        nodes.append(dumpNode(node));
    }
    return nodes.join(QLatin1Char('\n'));
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
};

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
