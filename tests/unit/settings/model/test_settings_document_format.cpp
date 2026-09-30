#include "documenttesthelpers.h"

#include <QTest>

using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

class TestSettingsDocumentFormat : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void removeTakesTrailingComment();
    void removeTakesCommentsAboveLastEntry();
    void removeKeepsCommentsAboveWhenMoreFollow();
    void removeKeepsBlockComment();
    void removeKeepsBlankLinesAround();
    void removeSharedLineKeepsNeighbour();
    void setNodeKeepsSurroundingComments();
    void setNodeKeepsCommentsInsideBlock();
    void oneLineBlockBecomesMultiLine();
    void oneLineBlockWithChildrenKeepsThem();
    void keepsTabIndentation();
    void crlfRemoveTakesWholeLine();
    void crlfInsertUsesCrlf();
    void crlfAppendAtEnd();
    void unicodeOffsets();
    void untouchedTextIsByteIdentical();
    void slashDashNodesAreIgnored();
    void moveCarriesAttachedComments();
    void moveInOneLineBlock();
    void moveKeepsCrlf();
};

void TestSettingsDocumentFormat::removeTakesTrailingComment()
{
    QCOMPARE(edited(QStringLiteral("layout {\n    gaps 4 // tight\n    center-focused-column \"never\"\n}\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout {\n    center-focused-column \"never\"\n}\n"));
}

void TestSettingsDocumentFormat::removeTakesCommentsAboveLastEntry()
{
    QCOMPARE(edited(QStringLiteral("layout {\n    gaps 4\n    // why\n    // more\n    struts {\n    }\n}\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/struts")); }),
        QStringLiteral("layout {\n    gaps 4\n}\n"));
    QCOMPARE(edited(QStringLiteral("// header\nlayout {\n}\n\n// widgets\nhide-desktop-widgets\n\nfill-panels-on-maximize\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("hide-desktop-widgets")); }),
        QStringLiteral("// header\nlayout {\n}\n\nfill-panels-on-maximize\n"));
}

void TestSettingsDocumentFormat::removeKeepsCommentsAboveWhenMoreFollow()
{
    QCOMPARE(edited(QStringLiteral("layout {\n    // spacing\n    gaps 4\n    struts {}\n}\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout {\n    // spacing\n    struts {}\n}\n"));
}

void TestSettingsDocumentFormat::removeKeepsBlockComment()
{
    QCOMPARE(edited(QStringLiteral("layout {\n    /* block */\n    gaps 4\n}\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout {\n    /* block */\n}\n"));
}

void TestSettingsDocumentFormat::removeKeepsBlankLinesAround()
{
    QCOMPARE(edited(QStringLiteral("a\n\nb\n\nc\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("b")); }),
        QStringLiteral("a\n\nc\n"));
    QCOMPARE(
        edited(QStringLiteral("a\n\nb\nc\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("b")); }), QStringLiteral("a\n\nc\n"));
    QCOMPARE(
        edited(QStringLiteral("a\nb\n\nc\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("b")); }), QStringLiteral("a\n\nc\n"));
    QCOMPARE(
        edited(QStringLiteral("a\n\nb\n\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("a")); }), QStringLiteral("b\n\n"));
    QCOMPARE(edited(QStringLiteral("x {\n\n    a\n\n    b\n}\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("x/a")); }),
        QStringLiteral("x {\n\n    b\n}\n"));
    QCOMPARE(edited(QStringLiteral("x {\n    a\n\n    b\n}\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("x/a")); }),
        QStringLiteral("x {\n    b\n}\n"));
    QCOMPARE(edited(QStringLiteral("a\r\n\r\nb\r\n\r\nc\r\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("b")); }),
        QStringLiteral("a\r\n\r\nc\r\n"));
    QCOMPARE(edited(QStringLiteral("a\nb"), [](ConfigDocument &d) { return d.remove(QStringLiteral("b")); }), QStringLiteral("a\n"));
}

void TestSettingsDocumentFormat::removeSharedLineKeepsNeighbour()
{
    QCOMPARE(
        edited(QStringLiteral("a; b\nc\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("a")); }), QStringLiteral("b\nc\n"));
    QCOMPARE(
        edited(QStringLiteral("a; b\nc\n"), [](ConfigDocument &d) { return d.remove(QStringLiteral("b")); }), QStringLiteral("a; \nc\n"));
}

void TestSettingsDocumentFormat::setNodeKeepsSurroundingComments()
{
    const QString text = QStringLiteral("// top\nlayout {\n    // about gaps\n    gaps 4 /* inline */ // end\n\n    // after\n}\n");
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {8})); }),
        QStringLiteral("// top\nlayout {\n    // about gaps\n    gaps 8 /* inline */ // end\n\n    // after\n}\n"));
}

void TestSettingsDocumentFormat::setNodeKeepsCommentsInsideBlock()
{
    const QString text = QStringLiteral("output \"DP-1\" {\n    // note\n    off\n}\n");
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.append(QStringLiteral("output"), leaf(QStringLiteral("scale"), {2})); }),
        QStringLiteral("output \"DP-1\" {\n    // note\n    off\n    scale 2\n}\n"));
    QCOMPARE(edited(text,
                 [](ConfigDocument &d) {
                     return d.setNode(QStringLiteral("output"), block(QStringLiteral("output"), {}, {QStringLiteral("DP-1")}));
                 }),
        QStringLiteral("output \"DP-1\" {}\n"));
}

void TestSettingsDocumentFormat::oneLineBlockBecomesMultiLine()
{
    QCOMPARE(edited(QStringLiteral("layout { gaps 4; }\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("float-child-windows"))); }),
        QStringLiteral("layout {\n    gaps 4\n    float-child-windows\n}\n"));
    QCOMPARE(edited(QStringLiteral("layout {}\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("gaps"), {1})); }),
        QStringLiteral("layout {\n    gaps 1\n}\n"));
    QCOMPARE(edited(QStringLiteral("    layout { gaps 4; struts { left 1; }; }\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("x"))); }),
        QStringLiteral("    layout {\n        gaps 4\n        struts { left 1; }\n        x\n    }\n"));
}

void TestSettingsDocumentFormat::oneLineBlockWithChildrenKeepsThem()
{
    QCOMPARE(edited(QStringLiteral("layout { struts { left 1; }; }\n"),
                 [](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/struts/right"), leaf(QStringLiteral("right"), {2})); }),
        QStringLiteral("layout { struts {\n        left 1\n        right 2\n    }; }\n"));
}

void TestSettingsDocumentFormat::keepsTabIndentation()
{
    QCOMPARE(edited(QStringLiteral("layout {\n\tgaps 4\n}\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("x"))); }),
        QStringLiteral("layout {\n\tgaps 4\n    x\n}\n"));
    QCOMPARE(edited(QStringLiteral("\tlayout {\n\t}\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("x"))); }),
        QStringLiteral("\tlayout {\n\t    x\n\t}\n"));
}

void TestSettingsDocumentFormat::crlfRemoveTakesWholeLine()
{
    QCOMPARE(edited(QStringLiteral("layout {\r\n    gaps 4\r\n    x\r\n}\r\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout {\r\n    x\r\n}\r\n"));
    QCOMPARE(edited(QStringLiteral("layout {\r\n    // note\r\n    gaps 4\r\n}\r\n"),
                 [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout {\r\n}\r\n"));
}

void TestSettingsDocumentFormat::crlfInsertUsesCrlf()
{
    QCOMPARE(edited(QStringLiteral("layout {\r\n    gaps 4\r\n}\r\n"),
                 [](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/struts/left"), leaf(QStringLiteral("left"), {1})); }),
        QStringLiteral("layout {\r\n    gaps 4\r\n    struts {\r\n        left 1\r\n    }\r\n}\r\n"));
    QCOMPARE(edited(QStringLiteral("layout { gaps 4; }\r\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("x"))); }),
        QStringLiteral("layout {\r\n    gaps 4\r\n    x\r\n}\r\n"));
    QCOMPARE(edited(QStringLiteral("output \"a\"\r\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("output"), leaf(QStringLiteral("off"))); }),
        QStringLiteral("output \"a\" {\r\n    off\r\n}\r\n"));
    const QVariantMap struts = block(QStringLiteral("struts"), {leaf(QStringLiteral("left"), {1}), leaf(QStringLiteral("top"), {2})});
    QCOMPARE(edited(QStringLiteral("layout {\r\n    struts {}\r\n}\r\n"),
                 [&](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/struts"), struts); }),
        QStringLiteral("layout {\r\n    struts {\r\n        left 1\r\n        top 2\r\n    }\r\n}\r\n"));
}

void TestSettingsDocumentFormat::crlfAppendAtEnd()
{
    QCOMPARE(edited(QStringLiteral("input {\r\n}\r\n"),
                 [](ConfigDocument &d) { return d.append(QString(), leaf(QStringLiteral("disable-minimize"))); }),
        QStringLiteral("input {\r\n}\r\n\r\ndisable-minimize\r\n"));
    QCOMPARE(edited(QStringLiteral("input {\r\n}"),
                 [](ConfigDocument &d) { return d.append(QString(), leaf(QStringLiteral("disable-minimize"))); }),
        QStringLiteral("input {\r\n}\r\n\r\ndisable-minimize\r\n"));
}

void TestSettingsDocumentFormat::unicodeOffsets()
{
    const QString text = QStringLiteral("// 🎮 émoji\nworkspace \"ünï 🎮\" {\n    open-on-output \"DP-1\"\n}\nlayout {\n    gaps 4\n}\n");
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {9})); }),
        QString(text).replace(QStringLiteral("gaps 4"), QStringLiteral("gaps 9")));
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.remove(QStringLiteral("workspace/open-on-output")); }),
        QStringLiteral("// 🎮 émoji\nworkspace \"ünï 🎮\" {\n}\nlayout {\n    gaps 4\n}\n"));
}

void TestSettingsDocumentFormat::untouchedTextIsByteIdentical()
{
    const QString text = QStringLiteral("layout   {\n  gaps    0x10 // hex\n\n\n  struts{left 1;}\n}\ninput{}\n");
    QCOMPARE(edited(text,
                 [](ConfigDocument &d) {
                     return d.setNode(QStringLiteral("input/focus-follows-mouse"), leaf(QStringLiteral("focus-follows-mouse")));
                 }),
        QStringLiteral("layout   {\n  gaps    0x10 // hex\n\n\n  struts{left 1;}\n}\ninput{\n    focus-follows-mouse\n}\n"));
}

void TestSettingsDocumentFormat::slashDashNodesAreIgnored()
{
    const QString text = QStringLiteral("layout {\n    /-gaps 4\n    gaps 8\n}\n");
    const ConfigDocument document(text);
    QCOMPARE(document.find(QStringLiteral("layout/gaps"))->arguments.first().toInteger(), 8);
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout {\n    /-gaps 4\n}\n"));
}

void TestSettingsDocumentFormat::moveCarriesAttachedComments()
{
    const QString text = QStringLiteral("// Firefox\nwindow-rule {\n    match app-id=\"firefox\"\n}\n\n// Steam\n// games\nwindow-rule { "
                                        "match app-id=\"steam\"; } // last\n");
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.move(QStringLiteral("window-rule#1"), -1); }),
        QStringLiteral("// Steam\n// games\nwindow-rule { match app-id=\"steam\"; } // last\n\n// Firefox\nwindow-rule {\n    match "
                       "app-id=\"firefox\"\n}\n"));
    const QString nested = QStringLiteral("layout {\n    // first\n    a 1\n    b 2 // second\n}\n");
    QCOMPARE(edited(nested, [](ConfigDocument &d) { return d.move(QStringLiteral("layout/a"), 0); }), nested);
    const QString siblings = QStringLiteral("binds {\n    // one\n    X { a; }\n    // two\n    X { b; }\n}\n");
    QCOMPARE(edited(siblings, [](ConfigDocument &d) { return d.move(QStringLiteral("binds/X"), 1); }),
        QStringLiteral("binds {\n    // two\n    X { b; }\n    // one\n    X { a; }\n}\n"));
}

void TestSettingsDocumentFormat::moveInOneLineBlock()
{
    QCOMPARE(edited(QStringLiteral("x { w \"a\"; w \"b\"; }\n"), [](ConfigDocument &d) { return d.move(QStringLiteral("x/w#1"), -1); }),
        QStringLiteral("x { w \"b\"; w \"a\"; }\n"));
}

void TestSettingsDocumentFormat::moveKeepsCrlf()
{
    QCOMPARE(edited(QStringLiteral("w \"a\" {\r\n    x\r\n}\r\n// b\r\nw \"b\"\r\n"),
                 [](ConfigDocument &d) { return d.move(QStringLiteral("w"), 1); }),
        QStringLiteral("// b\r\nw \"b\"\r\nw \"a\" {\r\n    x\r\n}\r\n"));
}

QTEST_GUILESS_MAIN(TestSettingsDocumentFormat)
#include "test_settings_document_format.moc"
