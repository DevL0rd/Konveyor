#include "documenttesthelpers.h"

#include <QTest>

using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

class TestSettingsDocumentEdits : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void setNodeReplacesHeadAndKeepsChildren();
    void setNodeReplacesChildrenWhenGiven();
    void setNodeCreatesMissingParents();
    void setNodeAppendsNextSibling();
    void setNodeRejectsGapInSiblings();
    void setNodeRejectsWholeDocument();
    void setNodeReturnsPath();
    void appendReturnsIndexedPath();
    void appendIntoLeafOpensBlock();
    void appendIntoEmptyDocument();
    void appendAfterTextWithoutTrailingNewline();
    void appendKeepsCloseBraceOnItsLine();
    void appendWhenCloseBraceSharesLastLine();
    void appendRejectsGapInParentSiblings();
    void removeMissingNodeIsNoOp();
    void removeFromOneLineBlock();
    void removeNestedByIndex();
    void moveSwapsSiblings();
    void moveRefusesPastEnds();
    void moveMissingNodeFails();
    void childPathsListsNamedChildren();
    void parseErrorBlocksEveryEdit();
    void invalidResultIsRejected();
    void findUsesIndexes();
};

void TestSettingsDocumentEdits::setNodeReplacesHeadAndKeepsChildren()
{
    const QString text = QStringLiteral("output \"DP-1\" {\n    // keep\n    off\n}\n");
    QCOMPARE(edited(text,
                 [](ConfigDocument &d) {
                     return d.setNode(QStringLiteral("output"), leaf(QStringLiteral("output"), {QStringLiteral("HDMI-A-1")}));
                 }),
        QStringLiteral("output \"HDMI-A-1\" {\n    // keep\n    off\n}\n"));
    QCOMPARE(edited(QStringLiteral("layout {\n    gaps 4 // tight\n}\n"),
                 [](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {12})); }),
        QStringLiteral("layout {\n    gaps 12 // tight\n}\n"));
}

void TestSettingsDocumentEdits::setNodeReplacesChildrenWhenGiven()
{
    const QString text = QStringLiteral("layout {\n    struts {\n        left 1\n    }\n}\n");
    const QVariantMap struts = block(QStringLiteral("struts"), {leaf(QStringLiteral("left"), {4}), leaf(QStringLiteral("right"), {8})});
    QCOMPARE(edited(text, [&](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/struts"), struts); }),
        QStringLiteral("layout {\n    struts {\n        left 4\n        right 8\n    }\n}\n"));
    const QVariantMap single = block(QStringLiteral("default-column-width"), {leaf(QStringLiteral("proportion"), {0.5})});
    QCOMPARE(edited(QStringLiteral("layout {\n    default-column-width {}\n}\n"),
                 [&](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/default-column-width"), single); }),
        QStringLiteral("layout {\n    default-column-width { proportion 0.5; }\n}\n"));
    const QVariantMap empty = block(QStringLiteral("default-column-width"), {});
    QCOMPARE(edited(QStringLiteral("layout {\n    default-column-width { proportion 0.5; }\n}\n"),
                 [&](ConfigDocument &d) { return d.setNode(QStringLiteral("layout/default-column-width"), empty); }),
        QStringLiteral("layout {\n    default-column-width {}\n}\n"));
}

void TestSettingsDocumentEdits::setNodeCreatesMissingParents()
{
    QCOMPARE(edited(QString(),
                 [](ConfigDocument &d) {
                     return d.setNode(QStringLiteral("gestures/touchpad/swipe-fingers"), leaf(QStringLiteral("swipe-fingers"), {4}));
                 }),
        QStringLiteral("gestures {\n    touchpad {\n        swipe-fingers 4\n    }\n}\n"));
    QCOMPARE(edited(QStringLiteral("input {\n}\n"),
                 [](ConfigDocument &d) { return d.setNode(QStringLiteral("gestures/hot-corners/off"), leaf(QStringLiteral("off"))); }),
        QStringLiteral("input {\n}\n\ngestures {\n    hot-corners {\n        off\n    }\n}\n"));
}

void TestSettingsDocumentEdits::setNodeAppendsNextSibling()
{
    const QString text = QStringLiteral("workspace \"a\"\n");
    QCOMPARE(edited(text,
                 [](ConfigDocument &d) {
                     return d.setNode(QStringLiteral("workspace#1"), leaf(QStringLiteral("workspace"), {QStringLiteral("b")}));
                 }),
        QStringLiteral("workspace \"a\"\n\nworkspace \"b\"\n"));
}

void TestSettingsDocumentEdits::setNodeRejectsGapInSiblings()
{
    QCOMPARE(failure(QStringLiteral("workspace \"a\"\n"),
                 [](ConfigDocument &d) { return d.setNode(QStringLiteral("workspace#2"), leaf(QStringLiteral("workspace"))); }),
        QStringLiteral("cannot create workspace#2: earlier siblings are missing"));
    QCOMPARE(failure(QString(),
                 [](ConfigDocument &d) { return d.setNode(QStringLiteral("output#1/layout/gaps"), leaf(QStringLiteral("gaps"), {1})); }),
        QStringLiteral("cannot create output#1: earlier siblings are missing"));
}

void TestSettingsDocumentEdits::setNodeRejectsWholeDocument()
{
    QCOMPARE(failure(QStringLiteral("a\n"), [](ConfigDocument &d) { return d.setNode(QString(), leaf(QStringLiteral("a"))); }),
        QStringLiteral("cannot replace the whole document"));
}

void TestSettingsDocumentEdits::setNodeReturnsPath()
{
    ConfigDocument document(QStringLiteral("layout {\n}\n"));
    QCOMPARE(document.setNode(QStringLiteral("layout/gaps#0"), leaf(QStringLiteral("gaps"), {3})).value(), QStringLiteral("layout/gaps"));
    QCOMPARE(document.setNode(QStringLiteral("layout/gaps#0"), leaf(QStringLiteral("gaps"), {4})).value(), QStringLiteral("layout/gaps#0"));
}

void TestSettingsDocumentEdits::appendReturnsIndexedPath()
{
    ConfigDocument document(QStringLiteral("window-rule {\n}\n"));
    QCOMPARE(document.append(QString(), block(QStringLiteral("window-rule"), {leaf(QStringLiteral("match"))})).value(),
        QStringLiteral("window-rule#1"));
    QCOMPARE(
        document.append(QStringLiteral("window-rule#1"), leaf(QStringLiteral("match"))).value(), QStringLiteral("window-rule#1/match#1"));
    QCOMPARE(document.text(), QStringLiteral("window-rule {\n}\n\nwindow-rule {\n    match\n    match\n}\n"));
}

void TestSettingsDocumentEdits::appendIntoLeafOpensBlock()
{
    QCOMPARE(edited(QStringLiteral("output \"DP-1\"\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("output"), leaf(QStringLiteral("off"))); }),
        QStringLiteral("output \"DP-1\" {\n    off\n}\n"));
    QCOMPARE(edited(QStringLiteral("layout {\n    border\n}\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout/border"), leaf(QStringLiteral("on"))); }),
        QStringLiteral("layout {\n    border {\n        on\n    }\n}\n"));
}

void TestSettingsDocumentEdits::appendIntoEmptyDocument()
{
    QCOMPARE(edited(QString(), [](ConfigDocument &d) { return d.append(QString(), leaf(QStringLiteral("disable-minimize"))); }),
        QStringLiteral("disable-minimize\n"));
}

void TestSettingsDocumentEdits::appendAfterTextWithoutTrailingNewline()
{
    QCOMPARE(edited(QStringLiteral("input {\n}"),
                 [](ConfigDocument &d) { return d.append(QString(), leaf(QStringLiteral("disable-minimize"))); }),
        QStringLiteral("input {\n}\n\ndisable-minimize\n"));
}

void TestSettingsDocumentEdits::appendKeepsCloseBraceOnItsLine()
{
    QCOMPARE(edited(QStringLiteral("layout {\n    gaps 4\n}\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("float-child-windows"))); }),
        QStringLiteral("layout {\n    gaps 4\n    float-child-windows\n}\n"));
}

void TestSettingsDocumentEdits::appendWhenCloseBraceSharesLastLine()
{
    QCOMPARE(edited(QStringLiteral("layout {\n    gaps 4; }\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("float-child-windows"))); }),
        QStringLiteral("layout {\n    gaps 4; \n    float-child-windows\n}\n"));
}

void TestSettingsDocumentEdits::appendRejectsGapInParentSiblings()
{
    QCOMPARE(failure(QStringLiteral("window-rule {\n}\n"),
                 [](ConfigDocument &d) { return d.append(QStringLiteral("window-rule#3"), leaf(QStringLiteral("match"))); }),
        QStringLiteral("cannot create window-rule#3: earlier siblings are missing"));
}

void TestSettingsDocumentEdits::removeMissingNodeIsNoOp()
{
    ConfigDocument document(QStringLiteral("layout {\n}\n"));
    QCOMPARE(document.remove(QStringLiteral("layout/gaps")).value(), QString());
    QCOMPARE(document.remove(QStringLiteral("missing/child#4")).value(), QString());
    QCOMPARE(document.text(), QStringLiteral("layout {\n}\n"));
}

void TestSettingsDocumentEdits::removeFromOneLineBlock()
{
    const QString text = QStringLiteral("layout { gaps 4; center-focused-column \"never\"; }\n");
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/gaps")); }),
        QStringLiteral("layout { center-focused-column \"never\"; }\n"));
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.remove(QStringLiteral("layout/center-focused-column")); }),
        QStringLiteral("layout { gaps 4; }\n"));
}

void TestSettingsDocumentEdits::removeNestedByIndex()
{
    const QString text = QStringLiteral("window-rule {\n    match app-id=\"a\"\n    match app-id=\"b\"\n    match app-id=\"c\"\n}\n");
    QCOMPARE(edited(text, [](ConfigDocument &d) { return d.remove(QStringLiteral("window-rule/match#1")); }),
        QStringLiteral("window-rule {\n    match app-id=\"a\"\n    match app-id=\"c\"\n}\n"));
}

void TestSettingsDocumentEdits::moveSwapsSiblings()
{
    const QString text = QStringLiteral("workspace \"a\"\n// between\nlayout {\n}\nworkspace \"b\" {\n    open-on-output \"DP-1\"\n}\n");
    ConfigDocument document(text);
    QCOMPARE(document.move(QStringLiteral("workspace#1"), -1).value(), QStringLiteral("workspace"));
    QCOMPARE(
        document.text(), QStringLiteral("workspace \"b\" {\n    open-on-output \"DP-1\"\n}\n// between\nlayout {\n}\nworkspace \"a\"\n"));
    QCOMPARE(document.move(QStringLiteral("workspace"), 1).value(), QStringLiteral("workspace#1"));
    QCOMPARE(document.text(), text);
}

void TestSettingsDocumentEdits::moveRefusesPastEnds()
{
    const QString text = QStringLiteral("workspace \"a\"\nworkspace \"b\"\n");
    QCOMPARE(failure(text, [](ConfigDocument &d) { return d.move(QStringLiteral("workspace"), -1); }),
        QStringLiteral("cannot move workspace further"));
    QCOMPARE(failure(text, [](ConfigDocument &d) { return d.move(QStringLiteral("workspace#1"), 1); }),
        QStringLiteral("cannot move workspace#1 further"));
    QCOMPARE(failure(text, [](ConfigDocument &d) { return d.move(QStringLiteral("workspace"), 5); }),
        QStringLiteral("cannot move workspace further"));
}

void TestSettingsDocumentEdits::moveMissingNodeFails()
{
    QCOMPARE(failure(QString(), [](ConfigDocument &d) { return d.move(QStringLiteral("workspace#2"), -1); }),
        QStringLiteral("nothing to move at workspace#2"));
}

void TestSettingsDocumentEdits::childPathsListsNamedChildren()
{
    const ConfigDocument document(QStringLiteral("window-rule { match app-id=\"a\"; }\nlayout {}\nwindow-rule\n"));
    const QVariantList rules = document.childPaths(QString(), QStringLiteral("window-rule"));
    QCOMPARE(rules.size(), 2);
    QCOMPARE(rules.at(0).toMap().value(QStringLiteral("path")).toString(), QStringLiteral("window-rule"));
    QCOMPARE(rules.at(1).toMap().value(QStringLiteral("path")).toString(), QStringLiteral("window-rule#1"));
    QVERIFY(rules.at(0).toMap().value(QStringLiteral("node")).toMap().contains(QStringLiteral("children")));
    QVERIFY(!rules.at(1).toMap().value(QStringLiteral("node")).toMap().contains(QStringLiteral("children")));
    const QVariantList matches = document.childPaths(QStringLiteral("window-rule"), QStringLiteral("match"));
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().toMap().value(QStringLiteral("path")).toString(), QStringLiteral("window-rule/match"));
    QVERIFY(document.childPaths(QStringLiteral("window-rule#5"), QStringLiteral("match")).isEmpty());
    QVERIFY(document.childPaths(QStringLiteral("window-rule#x"), QStringLiteral("match")).isEmpty());
}

void TestSettingsDocumentEdits::parseErrorBlocksEveryEdit()
{
    const QString broken = QStringLiteral("layout {\n    gaps 4\n");
    QVERIFY(failure(broken, [](ConfigDocument &d) {
        return d.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {1}));
    }).contains(QStringLiteral("config.kdl")));
    QVERIFY(!failure(broken, [](ConfigDocument &d) { return d.remove(QStringLiteral("layout")); }).startsWith(QLatin1Char('<')));
    QVERIFY(!failure(broken, [](ConfigDocument &d) { return d.move(QStringLiteral("layout"), 1); }).startsWith(QLatin1Char('<')));
    QCOMPARE(failure(broken, [](ConfigDocument &d) { return d.append(QString(), leaf(QStringLiteral("x"))); }),
        failure(broken, [](ConfigDocument &d) { return d.remove(QStringLiteral("layout")); }));
    QCOMPARE(failure(broken, [](ConfigDocument &d) { return d.append(QStringLiteral("layout"), leaf(QStringLiteral("x"))); }),
        failure(broken, [](ConfigDocument &d) { return d.remove(QStringLiteral("layout")); }));
    const ConfigDocument document(broken);
    QCOMPARE(document.find(QStringLiteral("layout")), nullptr);
    QCOMPARE(document.text(), broken);
}

void TestSettingsDocumentEdits::invalidResultIsRejected()
{
    QVERIFY(failure(QStringLiteral("layout {\n}\n"), [](ConfigDocument &d) {
        return d.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {qQNaN()}));
    }).startsWith(QStringLiteral("edit produced invalid KDL: ")));
}

void TestSettingsDocumentEdits::findUsesIndexes()
{
    const ConfigDocument document(QStringLiteral("output \"a\"\noutput \"b\" {\n    layout { gaps 3; }\n}\n"));
    QCOMPARE(document.find(QStringLiteral("output#1"))->arguments.first().toString(), QStringLiteral("b"));
    QCOMPARE(document.find(QStringLiteral("output#1/layout/gaps"))->arguments.first().toInteger(), 3);
    QCOMPARE(document.find(QStringLiteral("output#2")), nullptr);
    QCOMPARE(document.find(QStringLiteral("output#-1")), nullptr);
    QCOMPARE(document.find(QString()), nullptr);
}

QTEST_GUILESS_MAIN(TestSettingsDocumentEdits)
#include "test_settings_document_edits.moc"
