#include "configtesthelpers.h"

#include <QDir>
#include <QFile>
#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

constexpr qsizetype minimumDocBlockCount = 5;

struct DocBlock
{
    QString location;
    QString text;
    bool mustFail = false;
};

QString readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

std::optional<QString> kdlFencePrefix(const QString &line)
{
    const qsizetype fence = line.indexOf(QStringLiteral("```kdl"));
    if (fence < 0) {
        return std::nullopt;
    }
    const QString prefix = line.left(fence);
    const bool onlyIndentation = std::ranges::all_of(prefix, [](QChar c) { return c == u' ' || c == u'>'; });
    return onlyIndentation ? std::optional<QString>(prefix) : std::nullopt;
}

QString stripPrefix(const QString &line, const QString &prefix)
{
    if (line.startsWith(prefix)) {
        return line.mid(prefix.size());
    }
    return line.trimmed() == prefix.trimmed() ? QString() : line;
}

qsizetype collectBlock(const QStringList &lines, qsizetype start, const QString &prefix, QStringList &body)
{
    qsizetype index = start;
    while (index < lines.size() && !stripPrefix(lines[index], prefix).trimmed().startsWith(QStringLiteral("```"))) {
        body.append(stripPrefix(lines[index], prefix));
        ++index;
    }
    return index;
}

QList<DocBlock> extractKdlBlocks(const QString &markdown, const QString &fileName)
{
    QList<DocBlock> blocks;
    const QStringList lines = markdown.split(u'\n');
    for (qsizetype index = 0; index < lines.size(); ++index) {
        const std::optional<QString> prefix = kdlFencePrefix(lines[index]);
        if (!prefix) {
            continue;
        }
        const bool mustFail = lines[index].contains(QStringLiteral("must-fail"));
        QStringList body;
        const qsizetype end = collectBlock(lines, index + 1, *prefix, body);
        blocks.append(DocBlock {QStringLiteral("%1:%2").arg(fileName, QString::number(index + 2)), body.join(u'\n'), mustFail});
        index = end;
    }
    return blocks;
}

QList<DocBlock> repositoryKdlBlocks()
{
    const QDir source(QStringLiteral(KONVEYOR_SOURCE_DIR));
    QList<DocBlock> blocks = extractKdlBlocks(readFile(source.filePath(QStringLiteral("README.md"))), QStringLiteral("README.md"));
    const QDir docs(source.filePath(QStringLiteral("docs")));
    for (const QString &fileName : docs.entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name)) {
        blocks.append(extractKdlBlocks(readFile(docs.filePath(fileName)), QStringLiteral("docs/") + fileName));
    }
    return blocks;
}

}

class TestConfigDocumentation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentationBlocksLoad_data();
    void documentationBlocksLoad();
    void defaultConfigLoadsCleanly();
    void defaultConfigCommentsAreValidExamples_data();
    void defaultConfigCommentsAreValidExamples();
    void fenceExtraction();
};

void TestConfigDocumentation::documentationBlocksLoad_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("mustFail");
    const QList<DocBlock> blocks = repositoryKdlBlocks();
    QVERIFY(blocks.size() >= minimumDocBlockCount);
    for (const DocBlock &block : blocks) {
        QTest::newRow(qPrintable(block.location)) << block.text << block.mustFail;
    }
}

void TestConfigDocumentation::documentationBlocksLoad()
{
    QFETCH(QString, text);
    QFETCH(bool, mustFail);
    const auto result = loadString(text, QString::fromUtf8(QTest::currentDataTag()));
    QVERIFY2(result.has_value() != mustFail, result ? "loaded without an error" : qPrintable(result.error().toString()));
    if (result) {
        QVERIFY(result->warnings.isEmpty());
    }
}

void TestConfigDocumentation::defaultConfigLoadsCleanly()
{
    const QString path = QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl");
    const auto result = loadFile(path);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error().toString()));
    QCOMPARE(result->files, QStringList {path});
    QVERIFY(result->warnings.isEmpty());
    QCOMPARE(result->config.gestures.touchpad, defaultConfig().gestures.touchpad);
    QCOMPARE(result->config.gestures.touchscreen, defaultConfig().gestures.touchscreen);
    QCOMPARE(result->config.layout.presetColumnWidths, defaultConfig().layout.presetColumnWidths);
    QCOMPARE(result->config.animations, defaultConfig().animations);
    QCOMPARE(result->config.input, defaultConfig().input);
}

void TestConfigDocumentation::defaultConfigCommentsAreValidExamples_data()
{
    QTest::addColumn<QString>("text");
    const QStringList lines = readFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl")).split(u'\n');
    QStringList block;
    qsizetype start = 0;
    for (qsizetype index = 0; index <= lines.size(); ++index) {
        const QString line = index < lines.size() ? lines.at(index) : QString();
        const bool code = line.startsWith(QStringLiteral("// ")) && line.mid(3).startsWith(QStringLiteral("window-rule"));
        if (code || (!block.isEmpty() && line.startsWith(QStringLiteral("//")))) {
            if (block.isEmpty()) {
                start = index + 1;
            }
            block.append(line.mid(3));
            continue;
        }
        if (!block.isEmpty()) {
            QTest::newRow(qPrintable(QStringLiteral("default-config.kdl:%1").arg(start))) << block.join(u'\n');
            block.clear();
        }
    }
}

void TestConfigDocumentation::defaultConfigCommentsAreValidExamples()
{
    QFETCH(QString, text);
    verifyLoads(text);
}

void TestConfigDocumentation::fenceExtraction()
{
    const QString markdown = QStringLiteral("text\n```kdl\na 1\n```\n    ```kdl,must-fail\n    b {\n        c\n    }\n    ```\n"
                                            "> ```kdl\n> d\n>\n> e\n> ```\n```sh\nnot kdl\n```\nprose ```kdl inline\n");
    const QList<DocBlock> blocks = extractKdlBlocks(markdown, QStringLiteral("Doc.md"));
    QCOMPARE(blocks.size(), 3);
    QCOMPARE(blocks[0].location, QStringLiteral("Doc.md:3"));
    QCOMPARE(blocks[0].text, QStringLiteral("a 1"));
    QCOMPARE(blocks[0].mustFail, false);
    QCOMPARE(blocks[1].text, QStringLiteral("b {\n    c\n}"));
    QCOMPARE(blocks[1].mustFail, true);
    QCOMPARE(blocks[2].text, QStringLiteral("d\n\ne"));
}

QTEST_MAIN(TestConfigDocumentation)
#include "test_config_documentation.moc"
