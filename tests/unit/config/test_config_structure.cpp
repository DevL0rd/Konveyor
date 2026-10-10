#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigStructure : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void blocksRejectArgumentsAndProperties_data();
    void blocksRejectArgumentsAndProperties();
    void onAndOffTogether_data();
    void onAndOffTogether();
    void onFalseDoesNotConflict();
    void modBindsCollideWithTheirModKey_data();
    void modBindsCollideWithTheirModKey();
    void modBindsResolveAgainstTheFinalModKey();
    void actionArguments_data();
    void actionArguments();
    void actionArgumentsAreRejected_data();
    void actionArgumentsAreRejected();
};

void TestConfigStructure::blocksRejectArgumentsAndProperties_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("error");

    QTest::newRow("experiments property") << QStringLiteral("experiments foo=1 {\n}\n")
                                          << QStringLiteral("1:13: unexpected property `foo`");
    QTest::newRow("experiments argument") << QStringLiteral("experiments 1 {\n}\n")
                                          << QStringLiteral("1:13: no arguments expected for this node");
    QTest::newRow("config-notification property")
        << QStringLiteral("config-notification x=1 {\n}\n") << QStringLiteral("1:21: unexpected property `x`");
    QTest::newRow("input argument") << QStringLiteral("input 1 {\n}\n") << QStringLiteral("1:7: no arguments expected for this node");
    QTest::newRow("layout property") << QStringLiteral("layout a=1 {\n}\n") << QStringLiteral("1:8: unexpected property `a`");
    QTest::newRow("animations property") << QStringLiteral("animations a=1 {\n}\n") << QStringLiteral("1:12: unexpected property `a`");
    QTest::newRow("gestures argument") << QStringLiteral("gestures \"x\" {\n}\n")
                                       << QStringLiteral("1:10: no arguments expected for this node");
    QTest::newRow("binds property") << QStringLiteral("binds a=1 {\n}\n") << QStringLiteral("1:7: unexpected property `a`");
    QTest::newRow("window-rule argument") << QStringLiteral("window-rule \"x\" {\n}\n")
                                          << QStringLiteral("1:13: no arguments expected for this node");
    QTest::newRow("output property") << QStringLiteral("output \"DP-1\" a=1 {\n}\n") << QStringLiteral("1:15: unexpected property `a`");
    QTest::newRow("output second name") << QStringLiteral("output \"DP-1\" \"DP-2\"\n") << QStringLiteral("1:15: unexpected argument");
    QTest::newRow("monitor-profile property") << QStringLiteral("monitor-profile \"p\" a=1 {\n}\n")
                                              << QStringLiteral("1:21: unexpected property `a`");
    QTest::newRow("workspace property") << QStringLiteral("workspace \"w\" a=1\n") << QStringLiteral("1:15: unexpected property `a`");
    QTest::newRow("flag property") << QStringLiteral("hide-desktop-widgets a=1\n") << QStringLiteral("1:22: unexpected property `a`");
    QTest::newRow("flag children") << QStringLiteral("disable-minimize {\n    x\n}\n") << QStringLiteral("2:5: unexpected node `x`");
    QTest::newRow("flag two arguments") << QStringLiteral("fill-panels-on-maximize true false\n")
                                        << QStringLiteral("1:30: unexpected argument");
}

void TestConfigStructure::blocksRejectArgumentsAndProperties()
{
    QFETCH(QString, text);
    QFETCH(QString, error);
    QCOMPARE(failure(text), error);
}

void TestConfigStructure::onAndOffTogether_data()
{
    QTest::addColumn<QString>("marked");

    QTest::newRow("border") << QStringLiteral("layout { border { on; »off; }; }");
    QTest::newRow("focus-ring") << QStringLiteral("layout { focus-ring { off; »on; }; }");
    QTest::newRow("tab-indicator") << QStringLiteral("layout { tab-indicator { on; width 3; »off; }; }");
    QTest::newRow("insert-hint") << QStringLiteral("layout { insert-hint { off; »on true; }; }");
    QTest::newRow("animations") << QStringLiteral("animations { off; slowdown 2; »on; }");
    QTest::newRow("output border") << QStringLiteral("output \"DP-1\" { layout { border { on; »off; }; }; }");
    QTest::newRow("profile tab-indicator") << QStringLiteral("monitor-profile \"p\" { layout { tab-indicator { off; »on; }; }; }");
    QTest::newRow("workspace focus-ring") << QStringLiteral("workspace \"w\" { layout { focus-ring { on; »off; }; }; }");
    QTest::newRow("rule border") << QStringLiteral("window-rule { border { on; »off; }; }");
    QTest::newRow("rule focus-ring") << QStringLiteral("window-rule { focus-ring { off; »on; }; }");
}

void TestConfigStructure::onAndOffTogether()
{
    QFETCH(QString, marked);
    verifyFailure(marked, QStringLiteral("cannot use both `on` and `off`"));
}

void TestConfigStructure::onFalseDoesNotConflict()
{
    QCOMPARE(parsed(QStringLiteral("layout { border { on false; off; }; }")).layout.border.enabled, false);
    QCOMPARE(parsed(QStringLiteral("layout { focus-ring { on; off false; }; }")).layout.focusRing.enabled, true);
    QCOMPARE(parsed(QStringLiteral("layout { tab-indicator { off false; }; }")).layout.tabIndicator.enabled, true);
    QCOMPARE(parsed(QStringLiteral("layout { insert-hint { off; }; }")).layout.insertHint.enabled, false);
    QCOMPARE(parsed(QStringLiteral("animations { on false; off; }")).animations.enabled, false);
    QCOMPARE(parsed(QStringLiteral("window-rule { border { off false; }; }")).windowRules.first().border.enabled, std::nullopt);
}

void TestConfigStructure::modBindsCollideWithTheirModKey_data()
{
    QTest::addColumn<QString>("modKey");
    QTest::addColumn<QString>("explicitKey");

    QTest::newRow("super") << QStringLiteral("Super") << QStringLiteral("Super");
    QTest::newRow("win alias") << QStringLiteral("Win") << QStringLiteral("Super");
    QTest::newRow("alt") << QStringLiteral("Alt") << QStringLiteral("Alt");
    QTest::newRow("control alias") << QStringLiteral("control") << QStringLiteral("Ctrl");
    QTest::newRow("mod5 alias") << QStringLiteral("Mod5") << QStringLiteral("ISO_Level3_Shift");
    QTest::newRow("mod3 alias") << QStringLiteral("mod3") << QStringLiteral("ISO_Level5_Shift");
}

void TestConfigStructure::modBindsCollideWithTheirModKey()
{
    QFETCH(QString, modKey);
    QFETCH(QString, explicitKey);
    const QString text
        = QStringLiteral("input { mod-key \"%1\"; }\nbinds {\n    Mod+A { close-window; }\n    »%2+A { toggle-overview; }\n}\n");
    const Config config = parsed(QStringLiteral("input { mod-key \"%1\"; }").arg(modKey));
    verifyFailure(text.arg(modKey, explicitKey),
        QStringLiteral("keybind `%1+A` is the same as `Mod+A` while the Mod key is %2").arg(explicitKey, config.input.modKey));
    const QString other = explicitKey == QLatin1String("Alt") ? QStringLiteral("Ctrl") : QStringLiteral("Alt");
    verifyLoads(unmarked(text.arg(modKey, other)));
}

void TestConfigStructure::modBindsResolveAgainstTheFinalModKey()
{
    verifyFailure(QStringLiteral("binds {\n    Super+Shift+A { close-window; }\n    »Mod+Shift+A { toggle-overview; }\n}\n"),
        QStringLiteral("keybind `Mod+Shift+A` is the same as `Super+Shift+A` while the Mod key is Super"));
    const Config config = parsed(QStringLiteral("binds {\n    Super+A { close-window; }\n    Mod+A { toggle-overview; }\n}\n"
                                                "input { mod-key \"Alt\"; }\n"));
    QCOMPARE(config.binds.size(), 2);
    QCOMPARE(config.binds.at(1).resolvedModifiers, BindModifiers(BindModifier::Alt));
}

void TestConfigStructure::actionArguments_data()
{
    QTest::addColumn<QString>("action");
    QTest::addColumn<QStringList>("arguments");

    QTest::newRow("spawn several") << QStringLiteral("spawn \"a\" \"\" \"c\"")
                                   << QStringList {QStringLiteral("a"), QString(), QStringLiteral("c")};
    QTest::newRow("workspace index") << QStringLiteral("focus-workspace 255") << QStringList {QStringLiteral("255")};
    QTest::newRow("workspace zero") << QStringLiteral("move-window-to-workspace 0") << QStringList {QStringLiteral("0")};
    QTest::newRow("workspace name") << QStringLiteral("move-column-to-workspace \"chat\" focus=false")
                                    << QStringList {QStringLiteral("chat")};
    QTest::newRow("column index") << QStringLiteral("focus-column 3") << QStringList {QStringLiteral("3")};
    QTest::newRow("taskbar item") << QStringLiteral("focus-taskbar-item 10") << QStringList {QStringLiteral("10")};
    QTest::newRow("window index") << QStringLiteral("focus-window-in-column 1") << QStringList {QStringLiteral("1")};
    QTest::newRow("workspace to index") << QStringLiteral("move-workspace-to-index 2") << QStringList {QStringLiteral("2")};
    QTest::newRow("size percent") << QStringLiteral("set-column-width \"50%\"") << QStringList {QStringLiteral("50%")};
    QTest::newRow("size adjust") << QStringLiteral("set-window-height \"-10\"") << QStringList {QStringLiteral("-10")};
    QTest::newRow("size integer") << QStringLiteral("set-window-width 800") << QStringList {QStringLiteral("800")};
    QTest::newRow("size fraction percent") << QStringLiteral("set-column-width \"+2.5%\"") << QStringList {QStringLiteral("+2.5%")};
    QTest::newRow("display") << QStringLiteral("set-column-display \"tabbed\"") << QStringList {QStringLiteral("tabbed")};
    QTest::newRow("monitor") << QStringLiteral("focus-monitor \"HDMI-A-1\"") << QStringList {QStringLiteral("HDMI-A-1")};
    QTest::newRow("name") << QStringLiteral("set-workspace-name \"mail\"") << QStringList {QStringLiteral("mail")};
    QTest::newRow("focus true") << QStringLiteral("move-window-to-workspace-down focus=true") << QStringList {};
}

void TestConfigStructure::actionArguments()
{
    QFETCH(QString, action);
    QFETCH(QStringList, arguments);
    const Config config = parsed(QStringLiteral("binds { Mod+A { %1; }; }").arg(action));
    QCOMPARE(config.binds.first().action.arguments, arguments);
}

void TestConfigStructure::actionArgumentsAreRejected_data()
{
    QTest::addColumn<QString>("action");
    QTest::addColumn<QString>("message");

    QTest::newRow("spawn nothing") << QStringLiteral("»spawn") << QStringLiteral("action `spawn` requires an argument");
    QTest::newRow("spawn empty") << QStringLiteral("spawn »\"\"") << QStringLiteral("expected a non-empty string");
    QTest::newRow("spawn number") << QStringLiteral("spawn \"echo\" »1") << QStringLiteral("expected a string");
    QTest::newRow("spawn-sh boolean") << QStringLiteral("spawn-sh »true") << QStringLiteral("expected a string");
    QTest::newRow("workspace fraction") << QStringLiteral("focus-workspace »1.5") << QStringLiteral("expected a workspace index or name");
    QTest::newRow("workspace boolean") << QStringLiteral("move-column-to-workspace »true")
                                       << QStringLiteral("expected a workspace index or name");
    QTest::newRow("workspace empty") << QStringLiteral("focus-workspace »\"\"") << QStringLiteral("expected a workspace index or name");
    QTest::newRow("workspace too high") << QStringLiteral("focus-workspace »256") << QStringLiteral("value must be between 0 and 255");
    QTest::newRow("workspace negative") << QStringLiteral("move-window-to-workspace »-1")
                                        << QStringLiteral("value must be between 0 and 255");
    QTest::newRow("index zero") << QStringLiteral("focus-column »0") << QStringLiteral("value must be between 1 and 2147483647");
    QTest::newRow("index string") << QStringLiteral("move-column-to-index »\"2\"") << QStringLiteral("expected an integer");
    QTest::newRow("index fraction") << QStringLiteral("focus-window-in-column »1.5") << QStringLiteral("expected an integer");
    QTest::newRow("size word") << QStringLiteral("set-column-width »\"abc\"") << QStringLiteral("error parsing value");
    QTest::newRow("size trailing") << QStringLiteral("set-window-width »\"10%px\"")
                                   << QStringLiteral("trailing characters after '%' are not allowed");
    QTest::newRow("size empty") << QStringLiteral("set-window-height »\"%\"") << QStringLiteral("value is missing");
    QTest::newRow("size fixed fraction") << QStringLiteral("set-column-width »\"10.5\"") << QStringLiteral("error parsing value");
    QTest::newRow("size float") << QStringLiteral("set-column-width »0.5")
                                << QStringLiteral(R"(expected a size like "+10%", "50%" or "800")");
    QTest::newRow("display") << QStringLiteral("set-column-display »\"tabs\"") << QStringLiteral("expected one of `normal`, `tabbed`");
    QTest::newRow("monitor number") << QStringLiteral("focus-monitor »1") << QStringLiteral("expected a string");
    QTest::newRow("name empty") << QStringLiteral("set-workspace-name »\"\"") << QStringLiteral("expected a non-empty string");
    QTest::newRow("focus number") << QStringLiteral("move-column-to-workspace 1 focus=»1") << QStringLiteral("expected a boolean");
    QTest::newRow("focus string") << QStringLiteral("move-window-to-workspace-up focus=»\"no\"") << QStringLiteral("expected a boolean");
    QTest::newRow("action children") << QStringLiteral("close-window { »x; }") << QStringLiteral("unexpected node `x`");
}

void TestConfigStructure::actionArgumentsAreRejected()
{
    QFETCH(QString, action);
    QFETCH(QString, message);
    verifyFailure(QStringLiteral("binds {\n    Mod+A { %1; }\n}\n").arg(action), message);
}

QTEST_MAIN(TestConfigStructure)
#include "test_config_structure.moc"
