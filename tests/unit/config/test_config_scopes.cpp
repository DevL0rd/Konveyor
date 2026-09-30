#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

enum Scope : quint8
{
    Output = 1,
    Profile = 2,
    Workspace = 4,
    Everywhere = Output | Profile | Workspace,
    Monitors = Output | Profile,
    Nowhere = 0
};

struct LayoutNode
{
    const char *text;
    quint8 scopes;
};

constexpr LayoutNode kLayoutNodes[] = {
    {"gaps 8", Everywhere},
    {"struts { left 1; }", Everywhere},
    {"preset-column-widths { proportion 0.5; }", Everywhere},
    {"preset-window-heights { proportion 0.5; }", Everywhere},
    {"default-column-width { fixed 100; }", Everywhere},
    {"center-focused-column \"always\"", Everywhere},
    {"new-column-position \"left\"", Everywhere},
    {"always-center-single-column", Everywhere},
    {"always-expand-single-column false", Everywhere},
    {"default-column-display \"tabbed\"", Everywhere},
    {"focus-ring { off; }", Everywhere},
    {"border { on; }", Everywhere},
    {"tab-indicator { off; }", Everywhere},
    {"background-color \"#000000\"", Everywhere},
    {"group-app-windows \"stack\"", Monitors},
    {"max-rows-per-column 2", Monitors},
    {"new-window-placement \"stack\"", Monitors},
    {"empty-workspace-above-first", Monitors},
    {"insert-hint { off; }", Monitors},
    {"remember-window-sizes", Nowhere},
    {"remember-window-positions", Nowhere},
    {"float-child-windows", Nowhere},
};

struct ScopeSpec
{
    Scope scope;
    const char *prefix;
    const char *label;
};

constexpr ScopeSpec kScopes[] = {
    {Output, "output \"DP-1\" {\n    layout {\n        ", "output.layout"},
    {Profile, "monitor-profile \"wide\" {\n    layout {\n        ", "monitor-profile.layout"},
    {Workspace, "workspace \"chat\" {\n    layout {\n        ", "workspace.layout"},
};

std::optional<Layout> scopedLayout(const Config &config, Scope scope)
{
    std::optional<LayoutPart> part;
    switch (scope) {
    case Output:
        part = config.outputs.value(0).layout;
        break;
    case Profile:
        part = config.monitorProfiles.value(0).layout;
        break;
    default:
        part = config.workspaces.value(0).layout;
        break;
    }
    return part ? std::optional(mergedLayout(config.layout, *part)) : std::nullopt;
}

QString inScope(const ScopeSpec &spec, const QString &node)
{
    return QString::fromUtf8(spec.prefix) + node + QStringLiteral("\n    }\n}\n");
}

QString nodeName(const LayoutNode &node)
{
    return QString::fromUtf8(node.text).section(QLatin1Char(' '), 0, 0);
}

}

class TestConfigScopes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void globalLayoutAcceptsEveryNode_data();
    void globalLayoutAcceptsEveryNode();
    void scopedLayoutNodes_data();
    void scopedLayoutNodes();
    void scopesInheritTheGlobalLayout();
    void scopesResolveAfterTheWholeFile();
    void scopeFlagsCanTurnGlobalFlagsOff();
    void scopeFlagForms_data();
    void scopeFlagForms();
    void scopeListsReplaceGlobalLists();
    void scopeBordersMergeFieldByField();
    void emptyBorderOnlyTurnsOnInTheGlobalLayout();
    void scopeCanClearTheDefaultColumnWidth();
    void emptyPresetListsFallBackToDefaults();
    void scopesWithoutLayoutHaveNoLayout();
    void scopesRejectOtherNodes();
    void windowRulesAcceptOnlyPlacementNodes();
};

void TestConfigScopes::globalLayoutAcceptsEveryNode_data()
{
    QTest::addColumn<QString>("node");
    for (const LayoutNode &node : kLayoutNodes) {
        QTest::newRow(node.text) << QString::fromUtf8(node.text);
    }
}

void TestConfigScopes::globalLayoutAcceptsEveryNode()
{
    QFETCH(QString, node);
    QVERIFY(parsed(QStringLiteral("layout {\n    %1\n}\n").arg(node)).layout != defaultConfig().layout);
}

void TestConfigScopes::scopedLayoutNodes_data()
{
    QTest::addColumn<int>("scope");
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");
    QTest::addColumn<bool>("allowed");
    for (const ScopeSpec &spec : kScopes) {
        for (const LayoutNode &node : kLayoutNodes) {
            const QString name = QStringLiteral("%1 %2").arg(QString::fromLatin1(spec.label), QString::fromUtf8(node.text));
            QTest::newRow(qPrintable(name))
                << static_cast<int>(spec.scope) << inScope(spec, QStringLiteral("»") + QString::fromUtf8(node.text))
                << QStringLiteral("node `%1` is not allowed inside `%2`").arg(nodeName(node), QString::fromLatin1(spec.label))
                << ((node.scopes & spec.scope) != 0);
        }
    }
}

void TestConfigScopes::scopedLayoutNodes()
{
    QFETCH(int, scope);
    QFETCH(QString, marked);
    QFETCH(QString, message);
    QFETCH(bool, allowed);
    if (!allowed) {
        verifyFailure(marked, message);
        return;
    }
    const Config config = parsed(unmarked(marked));
    const std::optional<Layout> layout = scopedLayout(config, static_cast<Scope>(scope));
    QVERIFY(layout.has_value());
    QVERIFY(*layout != config.layout);
    QCOMPARE(config.layout, defaultConfig().layout);
}

void TestConfigScopes::scopesInheritTheGlobalLayout()
{
    for (const ScopeSpec &spec : kScopes) {
        const Config config = parsed(QStringLiteral("layout { gaps 4; center-focused-column \"on-overflow\"; }\n")
            + inScope(spec, QStringLiteral("center-focused-column \"always\"")));
        const Layout layout = *scopedLayout(config, spec.scope);
        QCOMPARE(layout.gaps, 4.0);
        QCOMPARE(layout.centerFocusedColumn, CenterFocusedColumn::Always);
        QCOMPARE(config.layout.centerFocusedColumn, CenterFocusedColumn::OnOverflow);
    }
}

void TestConfigScopes::scopesResolveAfterTheWholeFile()
{
    for (const ScopeSpec &spec : kScopes) {
        const Config config
            = parsed(inScope(spec, QStringLiteral("gaps 2")) + QStringLiteral("layout { new-column-position \"left\"; }\n"));
        const Layout layout = *scopedLayout(config, spec.scope);
        QCOMPARE(layout.gaps, 2.0);
        QCOMPARE(layout.newColumnPosition, NewColumnPosition::Left);
    }
}

void TestConfigScopes::scopeFlagsCanTurnGlobalFlagsOff()
{
    for (const ScopeSpec &spec : kScopes) {
        const Config config = parsed(QStringLiteral("layout { always-center-single-column; }\n")
            + inScope(spec, QStringLiteral("always-center-single-column false")));
        QCOMPARE(config.layout.alwaysCenterSingleColumn, true);
        QCOMPARE(scopedLayout(config, spec.scope)->alwaysCenterSingleColumn, false);
    }
}

void TestConfigScopes::scopeFlagForms_data()
{
    QTest::addColumn<int>("scope");
    QTest::addColumn<QString>("node");
    QTest::addColumn<int>("flag");
    const QStringList flags {QStringLiteral("always-center-single-column"), QStringLiteral("always-expand-single-column"),
        QStringLiteral("empty-workspace-above-first"), QStringLiteral("tab-indicator { hide-when-single-tab%1; }"),
        QStringLiteral("tab-indicator { place-within-column%1; }")};
    for (const ScopeSpec &spec : kScopes) {
        for (qsizetype flag = 0; flag < flags.size(); ++flag) {
            if (flag == 2 && spec.scope == Workspace) {
                continue;
            }
            for (const QString &form : {QString(), QStringLiteral(" true"), QStringLiteral(" false")}) {
                const QString node = flags.at(flag).contains(QStringLiteral("%1")) ? flags.at(flag).arg(form) : flags.at(flag) + form;
                QTest::newRow(qPrintable(QStringLiteral("%1 %2").arg(QString::fromLatin1(spec.label), node)))
                    << static_cast<int>(spec.scope) << node << static_cast<int>(flag);
            }
        }
    }
}

void TestConfigScopes::scopeFlagForms()
{
    QFETCH(int, scope);
    QFETCH(QString, node);
    QFETCH(int, flag);
    const Scope target = static_cast<Scope>(scope);
    const ScopeSpec &spec = *std::ranges::find_if(kScopes, [target](const ScopeSpec &entry) { return entry.scope == target; });
    const Layout layout = *scopedLayout(parsed(inScope(spec, node)), target);
    const QList<bool> values {layout.alwaysCenterSingleColumn, layout.alwaysExpandSingleColumn, layout.emptyWorkspaceAboveFirst,
        layout.tabIndicator.hideWhenSingleTab, layout.tabIndicator.placeWithinColumn};
    QCOMPARE(values.at(flag), !node.contains(QStringLiteral("false")));
}

void TestConfigScopes::scopeListsReplaceGlobalLists()
{
    for (const ScopeSpec &spec : kScopes) {
        const Config config = parsed(QStringLiteral("layout { preset-column-widths { proportion 0.2; proportion 0.4; }; }\n")
            + inScope(spec, QStringLiteral("preset-column-widths { fixed 640; }")));
        const Layout layout = *scopedLayout(config, spec.scope);
        QCOMPARE(layout.presetColumnWidths.size(), 1);
        QCOMPARE(fixedOf(layout.presetColumnWidths.first()), 640.0);
        QCOMPARE(config.layout.presetColumnWidths.size(), 2);
    }
}

void TestConfigScopes::scopeBordersMergeFieldByField()
{
    for (const ScopeSpec &spec : kScopes) {
        const Config config = parsed(QStringLiteral("layout { border { width 7; active-color \"#ff0000\"; }; }\n")
            + inScope(spec, QStringLiteral("border { on; inactive-color \"#00ff00\"; }")));
        const Layout layout = *scopedLayout(config, spec.scope);
        QCOMPARE(layout.border.enabled, true);
        QCOMPARE(layout.border.width, 7.0);
        QCOMPARE(layout.border.active.color, QColor(255, 0, 0));
        QCOMPARE(layout.border.inactive.color, QColor(0, 255, 0));
    }
}

void TestConfigScopes::emptyBorderOnlyTurnsOnInTheGlobalLayout()
{
    QCOMPARE(parsed(QStringLiteral("layout { border {}; }")).layout.border.enabled, true);
    QCOMPARE(parsed(QStringLiteral("layout { border { width 3; }; }")).layout.border.enabled, true);
    QCOMPARE(parsed(QStringLiteral("layout { border { off; }; }")).layout.border.enabled, false);
    for (const ScopeSpec &spec : kScopes) {
        QCOMPARE(scopedLayout(parsed(inScope(spec, QStringLiteral("border {}"))), spec.scope)->border.enabled, false);
        QCOMPARE(scopedLayout(parsed(inScope(spec, QStringLiteral("border { width 3; }"))), spec.scope)->border.enabled, false);
    }
    QCOMPARE(parsed(QStringLiteral("window-rule { border {}; }")).windowRules.first().border.enabled, std::nullopt);
}

void TestConfigScopes::scopeCanClearTheDefaultColumnWidth()
{
    for (const ScopeSpec &spec : kScopes) {
        const Config config = parsed(inScope(spec, QStringLiteral("default-column-width {}")));
        QCOMPARE(scopedLayout(config, spec.scope)->defaultColumnWidth, std::nullopt);
        QCOMPARE(proportionOf(*config.layout.defaultColumnWidth), 0.5);
    }
    QCOMPARE(parsed(QStringLiteral("layout { default-column-width {}; }")).layout.defaultColumnWidth, std::nullopt);
    verifyFailure(
        QStringLiteral("layout { default-column-width { fixed 1; »proportion 1; }; }"), QStringLiteral("expected no more than one child"));
}

void TestConfigScopes::emptyPresetListsFallBackToDefaults()
{
    const Config config = parsed(QStringLiteral("layout { preset-column-widths {}; preset-window-heights {}; }\n")
        + inScope(kScopes[0], QStringLiteral("preset-column-widths {}")));
    QCOMPARE(config.layout.presetColumnWidths, defaultConfig().layout.presetColumnWidths);
    QCOMPARE(config.layout.presetWindowHeights, defaultConfig().layout.presetWindowHeights);
    QCOMPARE(scopedLayout(config, Output)->presetColumnWidths, defaultConfig().layout.presetColumnWidths);
}

void TestConfigScopes::scopesWithoutLayoutHaveNoLayout()
{
    const Config config = parsed(QStringLiteral("output \"DP-1\"\nmonitor-profile \"p\"\nworkspace \"w\"\n"));
    QCOMPARE(config.outputs.first().layout, std::nullopt);
    QCOMPARE(config.outputs.first().hotCorners, std::nullopt);
    QCOMPARE(config.monitorProfiles.first().layout, std::nullopt);
    QVERIFY(config.monitorProfiles.first().matches.isEmpty());
    QCOMPARE(config.workspaces.first().layout, std::nullopt);
    QCOMPARE(config.workspaces.first().openOnOutput, std::nullopt);
}

void TestConfigScopes::scopesRejectOtherNodes()
{
    verifyFailure(
        QStringLiteral("output \"DP-1\" { layout {}; »layout {}; }"), QStringLiteral("duplicate node `layout`, single node expected"));
    verifyFailure(QStringLiteral("output \"DP-1\" { »match name=\"x\"; }"), QStringLiteral("unexpected node `match`"));
    verifyFailure(QStringLiteral("output »1"), QStringLiteral("expected a string"));
    verifyFailure(QStringLiteral("»output"), QStringLiteral("additional argument `name` is required"));
    verifyFailure(QStringLiteral("monitor-profile \"p\" { »hot-corners { off; }; }"), QStringLiteral("unexpected node `hot-corners`"));
    verifyFailure(QStringLiteral("»monitor-profile {}"), QStringLiteral("additional argument `name` is required"));
    verifyFailure(QStringLiteral("workspace \"w\" { »hot-corners { off; }; }"), QStringLiteral("unexpected node `hot-corners`"));
    verifyFailure(QStringLiteral("workspace \"w\" { open-on-output »2; }"), QStringLiteral("expected a string"));
    verifyFailure(QStringLiteral("workspace \"w\" { open-on-output \"a\"; »open-on-output \"b\"; }"),
        QStringLiteral("duplicate node `open-on-output`, single node expected"));
    verifyFailure(QStringLiteral("workspace »3"), QStringLiteral("expected a string"));
    verifyFailure(QStringLiteral("output \"DP-1\" { layout { »bogus; }; }"), QStringLiteral("unexpected node `bogus`"));
    verifyFailure(QStringLiteral("output \"DP-1\" { layout »1 {}; }"), QStringLiteral("no arguments expected for this node"));
}

void TestConfigScopes::windowRulesAcceptOnlyPlacementNodes()
{
    for (const LayoutNode &node : kLayoutNodes) {
        const QString text = QStringLiteral("window-rule {\n    »%1\n}\n").arg(QString::fromUtf8(node.text));
        const QStringList ruleNodes {QStringLiteral("default-column-width"), QStringLiteral("default-column-display"),
            QStringLiteral("focus-ring"), QStringLiteral("border"), QStringLiteral("group-app-windows"),
            QStringLiteral("max-rows-per-column"), QStringLiteral("new-window-placement"), QStringLiteral("float-child-windows")};
        if (ruleNodes.contains(nodeName(node))) {
            continue;
        }
        verifyFailure(text, QStringLiteral("unexpected node `%1`").arg(nodeName(node)));
    }
}

QTEST_MAIN(TestConfigScopes)
#include "test_config_scopes.moc"
