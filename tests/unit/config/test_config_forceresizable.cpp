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
    void keepsOverrideIncludeLast();
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

void TestConfigForceResizable::keepsOverrideIncludeLast()
{
    const QString text = QStringLiteral("include \"force-resizable.kdl\"\nwindow-rule { match app-id=\"^later$\"; }\n");
    const auto appended = ensureTrailingForceResizableInclude(text, QStringLiteral("config.kdl"));
    QVERIFY(appended.has_value());
    QVERIFY(appended->endsWith(QStringLiteral("include \"force-resizable.kdl\"\n")));
    const auto unchanged = ensureTrailingForceResizableInclude(*appended, QStringLiteral("config.kdl"));
    QVERIFY(unchanged.has_value());
    QCOMPARE(*unchanged, *appended);
}

void TestConfigForceResizable::rejectsInvalidInput()
{
    QVERIFY(!setForceResizableRule(QStringLiteral("{"), QStringLiteral("force-resizable.kdl"), QStringLiteral("game"), true));
    QVERIFY(!ensureTrailingForceResizableInclude(QStringLiteral("{"), QStringLiteral("config.kdl")));
}

QTEST_GUILESS_MAIN(TestConfigForceResizable)

#include "test_config_forceresizable.moc"
