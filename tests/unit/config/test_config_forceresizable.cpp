#include "config/forceresizable.h"
#include "config/loader.h"

#include <QTest>

using namespace Konveyor::Config;

class TestConfigForceResizable : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createsAndUpdatesAppRule();
    void preservesOtherRuleSettings();
    void addsTheOverrideIncludeOnlyOnce_data();
    void addsTheOverrideIncludeOnlyOnce();
    void rejectsInvalidInput();
    void rejectsAConfigThatStopsLoading();
    void appendsADisabledRule();
    void appendsAfterTextWithoutATrailingNewline();
    void keepsTheBlankLineBeforeTheNewRule_data();
    void keepsTheBlankLineBeforeTheNewRule();
    void insertsIntoRulesOfEveryLayout_data();
    void insertsIntoRulesOfEveryLayout();
    void ignoresRulesThatAreNotExactlyTheAppRule_data();
    void ignoresRulesThatAreNotExactlyTheAppRule();
    void leavesLookalikeRulesThatDoNotLoadAlone_data();
    void leavesLookalikeRulesThatDoNotLoadAlone();
    void recognisesOnlyTheOverrideInclude_data();
    void recognisesOnlyTheOverrideInclude();
};

void TestConfigForceResizable::createsAndUpdatesAppRule()
{
    const auto enabled = setForceResizableRule(QString(), QStringLiteral("force-resizable.kdl"), QStringLiteral("game.exe"), true);
    QVERIFY(enabled.has_value());
    auto loaded = loadString(*enabled, QStringLiteral("force-resizable.kdl"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->config.windowRules.size(), 1);
    QCOMPARE(loaded->config.windowRules.first().matches.first().appId->pattern(), QStringLiteral("^game\\.exe$"));
    QCOMPARE(loaded->config.windowRules.first().forceResizable, std::optional(true));

    const auto disabled = setForceResizableRule(*enabled, QStringLiteral("force-resizable.kdl"), QStringLiteral("game.exe"), false);
    QVERIFY(disabled.has_value());
    loaded = loadString(*disabled, QStringLiteral("force-resizable.kdl"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->config.windowRules.size(), 1);
    QCOMPARE(loaded->config.windowRules.first().forceResizable, std::optional(false));
}

void TestConfigForceResizable::preservesOtherRuleSettings()
{
    const QString text = QStringLiteral(R"(window-rule {
    match app-id=r#"^gáme$"#
}
window-rule {
    match app-id=r#"^game$"#
    open-floating false
}
)");
    const auto updated = setForceResizableRule(text, QStringLiteral("force-resizable.kdl"), QStringLiteral("game"), true);
    QVERIFY(updated.has_value());
    const auto loaded = loadString(*updated, QStringLiteral("force-resizable.kdl"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->config.windowRules.size(), 2);
    QCOMPARE(loaded->config.windowRules.last().openFloating, std::optional(false));
    QCOMPARE(loaded->config.windowRules.last().forceResizable, std::optional(true));
}

void TestConfigForceResizable::addsTheOverrideIncludeOnlyOnce_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");
    const QString include = QStringLiteral("include \"force-resizable.kdl\"\n");
    const QString optional = QStringLiteral("include optional=true \"force-resizable.kdl\"\n");
    const QString rule = QStringLiteral("window-rule { match app-id=\"^later$\"; }\n");
    QTest::newRow("missing") << rule << rule + QStringLiteral("\n") + include;
    QTest::newRow("last") << rule + include << rule + include;
    QTest::newRow("moved to the top") << include + rule << include + rule;
    QTest::newRow("between other nodes") << rule + include + rule << rule + include + rule;
    QTest::newRow("optional") << optional + rule << optional + rule;
}

void TestConfigForceResizable::addsTheOverrideIncludeOnlyOnce()
{
    QFETCH(QString, text);
    QFETCH(QString, expected);
    const auto updated = ensureForceResizableInclude(text, QStringLiteral("config.kdl"));
    QVERIFY(updated.has_value());
    QCOMPARE(*updated, expected);
}

void TestConfigForceResizable::rejectsInvalidInput()
{
    QVERIFY(!setForceResizableRule(QStringLiteral("{"), QStringLiteral("force-resizable.kdl"), QStringLiteral("game"), true));
    QVERIFY(!ensureForceResizableInclude(QStringLiteral("{"), QStringLiteral("config.kdl")));
}

void TestConfigForceResizable::rejectsAConfigThatStopsLoading()
{
    const auto result
        = setForceResizableRule(QStringLiteral("layout { gaps -1; }\n"), QStringLiteral("config.kdl"), QStringLiteral("game"), true);
    QVERIFY(!result.has_value());
    QVERIFY(!result.error().isEmpty());
}

void TestConfigForceResizable::appendsADisabledRule()
{
    const auto result = setForceResizableRule(QString(), QStringLiteral("config.kdl"), QStringLiteral("game"), false);
    QVERIFY(result.has_value());
    QCOMPARE(*result, QStringLiteral("window-rule {\n    match app-id=r#\"^game$\"#\n    force-resizable false\n}\n"));
}

void TestConfigForceResizable::appendsAfterTextWithoutATrailingNewline()
{
    const auto result = ensureForceResizableInclude(QStringLiteral("layout { gaps 4; }"), QStringLiteral("config.kdl"));
    QVERIFY(result.has_value());
    QCOMPARE(*result, QStringLiteral("layout { gaps 4; }\n\ninclude \"force-resizable.kdl\"\n"));
}

void TestConfigForceResizable::keepsTheBlankLineBeforeTheNewRule_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("separator");
    QTest::newRow("one newline") << QStringLiteral("layout { gaps 4; }\n") << QStringLiteral("\n");
    QTest::newRow("blank line") << QStringLiteral("layout { gaps 4; }\n\n") << QString();
}

void TestConfigForceResizable::keepsTheBlankLineBeforeTheNewRule()
{
    QFETCH(QString, text);
    QFETCH(QString, separator);
    const auto result = ensureForceResizableInclude(text, QStringLiteral("config.kdl"));
    QVERIFY(result.has_value());
    QCOMPARE(*result, text + separator + QStringLiteral("include \"force-resizable.kdl\"\n"));
}

void TestConfigForceResizable::insertsIntoRulesOfEveryLayout_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");
    const QString line = QStringLiteral("match app-id=r#\"^game$\"#");
    QTest::newRow("single line") << QStringLiteral("window-rule { %1; }\n").arg(line)
                                 << QStringLiteral("window-rule { %1;  force-resizable true;}\n").arg(line);
    QTest::newRow("close on its own line") << QStringLiteral("window-rule {\n    %1\n}\n").arg(line)
                                           << QStringLiteral("window-rule {\n    %1\n    force-resizable true\n}\n").arg(line);
    QTest::newRow("close after content") << QStringLiteral("window-rule {\n    %1; }\n").arg(line)
                                         << QStringLiteral("window-rule {\n    %1; \n    force-resizable true\n}\n").arg(line);
    QTest::newRow("indented with spaces") << QStringLiteral("  window-rule {\n    %1\n  }\n").arg(line)
                                          << QStringLiteral("  window-rule {\n    %1\n      force-resizable true\n  }\n").arg(line);
    QTest::newRow("indented with a tab") << QStringLiteral("\twindow-rule {\n\t\t%1\n\t}\n").arg(line)
                                         << QStringLiteral("\twindow-rule {\n\t\t%1\n\t    force-resizable true\n\t}\n").arg(line);
    QTest::newRow("after another node on the line")
        << QStringLiteral("layout { }; window-rule {\n    %1\n}\n").arg(line)
        << QStringLiteral("layout { }; window-rule {\n    %1\n    force-resizable true\n}\n").arg(line);
    QTest::newRow("carriage returns") << QStringLiteral("window-rule {\r    %1\r}\r").arg(line)
                                      << QStringLiteral("window-rule {\r    %1\r    force-resizable true\n}\r").arg(line);
}

void TestConfigForceResizable::insertsIntoRulesOfEveryLayout()
{
    QFETCH(QString, text);
    QFETCH(QString, expected);
    const auto result = setForceResizableRule(text, QStringLiteral("config.kdl"), QStringLiteral("game"), true);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error()));
    QCOMPARE(*result, expected);
}

void TestConfigForceResizable::ignoresRulesThatAreNotExactlyTheAppRule_data()
{
    QTest::addColumn<QString>("rule");
    QTest::newRow("another node") << QStringLiteral("layout { gaps 4; }");
    QTest::newRow("excluded") << QStringLiteral("window-rule { match app-id=\"^game$\"; exclude title=\"x\"; }");
    QTest::newRow("no match") << QStringLiteral("window-rule { open-floating true; }");
    QTest::newRow("two matches") << QStringLiteral("window-rule { match app-id=\"^game$\"; match title=\"x\"; }");
    QTest::newRow("extra property") << QStringLiteral("window-rule { match app-id=\"^game$\" title=\"x\"; }");
    QTest::newRow("other property") << QStringLiteral("window-rule { match title=\"x\"; }");
    QTest::newRow("other pattern") << QStringLiteral("window-rule { match app-id=\"^other$\"; }");
}

void TestConfigForceResizable::ignoresRulesThatAreNotExactlyTheAppRule()
{
    QFETCH(QString, rule);
    const QString text = rule + QLatin1Char('\n');
    const auto result = setForceResizableRule(text, QStringLiteral("config.kdl"), QStringLiteral("game"), true);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error()));
    QVERIFY(result->startsWith(text + QLatin1Char('\n') + QStringLiteral("window-rule {")));
    const auto loaded = loadString(*result, QStringLiteral("config.kdl"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->config.windowRules.last().forceResizable, std::optional(true));
}

void TestConfigForceResizable::leavesLookalikeRulesThatDoNotLoadAlone_data()
{
    QTest::addColumn<QString>("rule");
    QTest::newRow("match argument") << QStringLiteral("window-rule { match \"x\" app-id=\"^game$\"; }");
    QTest::newRow("match children") << QStringLiteral("window-rule { match app-id=\"^game$\" { x; }; }");
    QTest::newRow("not a string") << QStringLiteral("window-rule { match app-id=null; }");
}

void TestConfigForceResizable::leavesLookalikeRulesThatDoNotLoadAlone()
{
    QFETCH(QString, rule);
    QVERIFY(!setForceResizableRule(rule + QLatin1Char('\n'), QStringLiteral("config.kdl"), QStringLiteral("game"), true));
}

void TestConfigForceResizable::recognisesOnlyTheOverrideInclude_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("expected");
    QTest::newRow("override") << QStringLiteral("include \"force-resizable.kdl\"") << true;
    QTest::newRow("other file") << QStringLiteral("include \"other.kdl\"") << false;
    QTest::newRow("not a string") << QStringLiteral("include 5") << false;
    QTest::newRow("two arguments") << QStringLiteral("include \"force-resizable.kdl\" \"other.kdl\"") << false;
    QTest::newRow("another node") << QStringLiteral("layout \"force-resizable.kdl\"") << false;
}

void TestConfigForceResizable::recognisesOnlyTheOverrideInclude()
{
    QFETCH(QString, text);
    QFETCH(bool, expected);
    const auto document = Konveyor::Kdl::parse(text, QStringLiteral("config.kdl"));
    QVERIFY(document.has_value());
    QCOMPARE(isForceResizableInclude(document->nodes.first()), expected);
}

QTEST_GUILESS_MAIN(TestConfigForceResizable)

#include "test_config_forceresizable.moc"
