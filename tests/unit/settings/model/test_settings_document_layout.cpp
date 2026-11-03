#include "documenttesthelpers.h"

#include <QTest>

using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

namespace
{

QString removed(const QString &text, const QString &path)
{
    return edited(text, [&](ConfigDocument &d) { return d.remove(path); });
}

QString moved(const QString &text, const QString &path, int delta)
{
    return edited(text, [&](ConfigDocument &d) { return d.move(path, delta); });
}

QString appended(const QString &text, const QString &parent, const QString &name)
{
    return edited(text, [&](ConfigDocument &d) { return d.append(parent, leaf(name, {1})); });
}

}

class TestSettingsDocumentLayout : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void removesLinesEndedByALoneCarriageReturn();
    void removesTheFirstLineAfterABlankOne();
    void removesNodesWithTrailingComments();
    void removesTheLastNodeOfAFileWithoutNewline();
    void movesNodesSharingALine();
    void movesNodesFollowedBySemicolonAndComment();
    void movesTheLastNodeOfAFileWithoutNewline();
    void appendsToNodesThatShareALine();
    void editsAFileWhoseFirstLineIsBlank();
    void appendsBeforeAForceResizableInclude();
};

void TestSettingsDocumentLayout::removesLinesEndedByALoneCarriageReturn()
{
    QCOMPARE(removed(QStringLiteral("layout {\r    gaps 4\r    corners 2\r}\r"), QStringLiteral("layout/gaps")),
        QStringLiteral("layout {\r    corners 2\r}\r"));
    QCOMPARE(removed(QStringLiteral("a 1\r// about b\rb 2\rc 3\r"), QStringLiteral("b")), QStringLiteral("a 1\r// about b\rc 3\r"));
}

void TestSettingsDocumentLayout::removesTheFirstLineAfterABlankOne()
{
    QCOMPARE(removed(QStringLiteral("\nfoo 1\nbar 2\n"), QStringLiteral("foo")), QStringLiteral("\nbar 2\n"));
}

void TestSettingsDocumentLayout::removesNodesWithTrailingComments()
{
    QCOMPARE(removed(QStringLiteral("a 1\nb 2 // note"), QStringLiteral("b")), QStringLiteral("a 1\n"));
    QCOMPARE(removed(QStringLiteral("a 1\nb 2 /* note */\nc 3\n"), QStringLiteral("b")), QStringLiteral("a 1\n/* note */\nc 3\n"));
}

void TestSettingsDocumentLayout::removesTheLastNodeOfAFileWithoutNewline()
{
    QCOMPARE(removed(QStringLiteral("a 1\nb 2"), QStringLiteral("b")), QStringLiteral("a 1\n"));
    QCOMPARE(removed(QStringLiteral("a 1; b 2\n"), QStringLiteral("b")), QStringLiteral("a 1; \n"));
}

void TestSettingsDocumentLayout::movesNodesSharingALine()
{
    QCOMPARE(moved(QStringLiteral("a 1; a 2\nc 3\n"), QStringLiteral("a#1"), -1), QStringLiteral("a 2; a 1\nc 3\n"));
}

void TestSettingsDocumentLayout::movesNodesFollowedBySemicolonAndComment()
{
    QCOMPARE(moved(QStringLiteral("a 1; // one\na 2\n"), QStringLiteral("a#1"), -1), QStringLiteral("a 2; // one\na 1\n"));
}

void TestSettingsDocumentLayout::movesTheLastNodeOfAFileWithoutNewline()
{
    QCOMPARE(moved(QStringLiteral("a 1\na 2"), QStringLiteral("a#1"), -1), QStringLiteral("a 2\na 1"));
}

void TestSettingsDocumentLayout::appendsToNodesThatShareALine()
{
    QCOMPARE(appended(QStringLiteral("a 1; b 2\n"), QStringLiteral("b"), QStringLiteral("c")), QStringLiteral("a 1; b 2 {\n    c 1\n}\n"));
}

void TestSettingsDocumentLayout::editsAFileWhoseFirstLineIsBlank()
{
    QCOMPARE(appended(QStringLiteral("\nlayout {\r\n}\n"), QStringLiteral("layout"), QStringLiteral("gaps")),
        QStringLiteral("\nlayout {\r\n    gaps 1\n}\n"));
}

void TestSettingsDocumentLayout::appendsBeforeAForceResizableInclude()
{
    const QString include = QStringLiteral("include \"force-resizable.kdl\"");
    QCOMPARE(appended(QStringLiteral("a 1\n\n") + include + QLatin1Char('\n'), QString(), QStringLiteral("b")),
        QStringLiteral("a 1\n\nb 1\n\n") + include + QLatin1Char('\n'));
    QCOMPARE(appended(QStringLiteral("a 1; ") + include + QLatin1Char('\n'), QString(), QStringLiteral("b")),
        QStringLiteral("a 1; b 1\n") + include + QLatin1Char('\n'));
}

QTEST_GUILESS_MAIN(TestSettingsDocumentLayout)

#include "test_settings_document_layout.moc"
