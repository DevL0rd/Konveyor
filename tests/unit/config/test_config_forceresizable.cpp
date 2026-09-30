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

QTEST_GUILESS_MAIN(TestConfigForceResizable)

#include "test_config_forceresizable.moc"
