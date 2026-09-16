#include "kdltesthelpers.h"

#include <QDir>
#include <QFile>
#include <QTest>

using namespace Konveyor::Kdl;
using namespace Konveyor::Kdl::Testing;

namespace
{

constexpr qsizetype minimumDocBlockCount = 200;

struct DocBlock
{
    QString location;
    QString text;
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
        QStringList body;
        const qsizetype end = collectBlock(lines, index + 1, *prefix, body);
        blocks.append(DocBlock {QStringLiteral("%1:%2").arg(fileName, QString::number(index + 2)), body.join(u'\n')});
        index = end;
    }
    return blocks;
}

QList<DocBlock> wikiKdlBlocks(const QDir &wiki)
{
    QList<DocBlock> blocks;
    for (const QString &fileName : wiki.entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name)) {
        blocks.append(extractKdlBlocks(readFile(wiki.filePath(fileName)), fileName));
    }
    return blocks;
}

}

class TestKdlReference : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultConfig();
    void documentationBlocks_data();
    void documentationBlocks();
    void fenceExtraction();

private:
    static void verifyLayout(const Node &root);
    static void verifyWindowRules(const Node &root);
    static void verifyBinds(const Node &root, const QString &path);
};

void TestKdlReference::defaultConfig()
{
    const QString path = qEnvironmentVariable("KONVEYOR_REFERENCE_CONFIG");
    if (!QFile::exists(path)) {
        QSKIP("set KONVEYOR_REFERENCE_CONFIG to an external config file to run this test");
    }
    const auto document = parse(readFile(path), path);
    QVERIFY2(document, describeFailure(document));
    Node root;
    root.children = document->nodes;
    const Node *spawn = root.child(QStringLiteral("spawn-at-startup"));
    QVERIFY(spawn != nullptr);
    QCOMPARE(spawn->arguments[0].toString(), QStringLiteral("waybar"));
    QVERIFY(root.child(QStringLiteral("input")) != nullptr);
    QVERIFY(root.child(QStringLiteral("animations")) != nullptr);
    verifyLayout(root);
    verifyWindowRules(root);
    verifyBinds(root, path);
}

void TestKdlReference::verifyLayout(const Node &root)
{
    const Node *layout = root.child(QStringLiteral("layout"));
    QVERIFY(layout != nullptr);
    const Node *gaps = layout->child(QStringLiteral("gaps"));
    QVERIFY(gaps != nullptr);
    QCOMPARE(gaps->arguments.size(), 1);
    QVERIFY(gaps->arguments[0].isInteger());
    QCOMPARE(gaps->arguments[0].toInteger(), 16);
    const Node *defaultWidth = layout->child(QStringLiteral("default-column-width"));
    QVERIFY(defaultWidth != nullptr);
    const Node *proportion = defaultWidth->child(QStringLiteral("proportion"));
    QVERIFY(proportion != nullptr);
    QCOMPARE(proportion->arguments[0].toDouble(), 0.5);
}

void TestKdlReference::verifyWindowRules(const Node &root)
{
    const QList<const Node *> windowRules = root.childrenNamed(QStringLiteral("window-rule"));
    QCOMPARE(windowRules.size(), 2);
    const Node *wezterm = windowRules[0]->child(QStringLiteral("match"));
    QVERIFY(wezterm != nullptr);
    QCOMPARE(wezterm->property(QStringLiteral("app-id"))->value.toString(), QStringLiteral("^org\\.wezfurlong\\.wezterm$"));
    const Node *pictureInPicture = windowRules[1]->child(QStringLiteral("match"));
    QVERIFY(pictureInPicture != nullptr);
    QCOMPARE(pictureInPicture->property(QStringLiteral("title"))->value.toString(), QStringLiteral("^Picture-in-Picture$"));
}

void TestKdlReference::verifyBinds(const Node &root, const QString &path)
{
    const Node *binds = root.child(QStringLiteral("binds"));
    QVERIFY(binds != nullptr);
    QVERIFY(binds->children.size() > 100);
    const Node *terminal = binds->child(QStringLiteral("Mod+T"));
    QVERIFY(terminal != nullptr);
    QCOMPARE(terminal->property(QStringLiteral("hotkey-overlay-title"))->value.toString(), QStringLiteral("Open a Terminal: alacritty"));
    QCOMPARE(terminal->children.size(), 1);
    QCOMPARE(terminal->children[0].name, QStringLiteral("spawn"));
    QCOMPARE(terminal->children[0].arguments[0].toString(), QStringLiteral("alacritty"));
    const Node *volume = binds->child(QStringLiteral("XF86AudioRaiseVolume"));
    QVERIFY(volume != nullptr);
    QVERIFY(volume->property(QStringLiteral("allow-when-locked"))->value.toBool());
    QCOMPARE(terminal->location.file, path);
    QVERIFY(terminal->location.line > binds->location.line);
}

void TestKdlReference::documentationBlocks_data()
{
    const QString docs = qEnvironmentVariable("KONVEYOR_REFERENCE_DOCS");
    const QDir wiki(docs);
    if (docs.isEmpty() || !wiki.exists()) {
        QSKIP("set KONVEYOR_REFERENCE_DOCS to a directory of Markdown config docs to run this test");
    }
    QTest::addColumn<QString>("text");
    const QList<DocBlock> blocks = wikiKdlBlocks(wiki);
    QVERIFY(blocks.size() >= minimumDocBlockCount);
    for (const DocBlock &block : blocks) {
        QTest::newRow(qPrintable(block.location)) << block.text;
    }
}

void TestKdlReference::documentationBlocks()
{
    QFETCH(QString, text);
    const auto document = parse(text, QString::fromUtf8(QTest::currentDataTag()));
    QVERIFY2(document, describeFailure(document));
}

void TestKdlReference::fenceExtraction()
{
    const QString markdown = QStringLiteral("text\n```kdl\na 1\n```\n    ```kdl,must-fail\n    b {\n        c\n    }\n    ```\n"
                                            "> ```kdl\n> d\n>\n> e\n> ```\n```sh\nnot kdl\n```\nprose ```kdl inline\n");
    const QList<DocBlock> blocks = extractKdlBlocks(markdown, QStringLiteral("Doc.md"));
    QCOMPARE(blocks.size(), 3);
    QCOMPARE(blocks[0].location, QStringLiteral("Doc.md:3"));
    QCOMPARE(blocks[0].text, QStringLiteral("a 1"));
    QCOMPARE(blocks[1].text, QStringLiteral("b {\n    c\n}"));
    QCOMPARE(blocks[2].text, QStringLiteral("d\n\ne"));
    const auto document = parseText(blocks[1].text + u'\n' + blocks[2].text);
    QVERIFY2(document, describeFailure(document));
    QCOMPARE(document->nodes.size(), 3);
}

QTEST_GUILESS_MAIN(TestKdlReference)

#include "test_kdl_reference.moc"
