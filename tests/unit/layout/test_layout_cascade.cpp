#include "helpers.h"

#include "config/loader.h"

#include <functional>

using namespace LayoutTest;

namespace
{

using Probe = std::function<QVariant(Fixture &)>;

struct Field
{
    const char *name;
    QString a;
    QString b;
    bool inWorkspace;
    Probe probe;
};

struct Levels
{
    QStringList global;
    QStringList profile;
    QStringList output;
    std::optional<QStringList> workspace;
};

QVariant frameOf(Fixture &fixture, Layout::WindowId id)
{
    return fixture.frame(id);
}

QVariant columnOfLast(Fixture &fixture, const QStringList &apps)
{
    Layout::WindowId last = 0;
    for (const QString &app : apps) {
        last = fixture.add(app);
    }
    return fixture.state(last).columnIndex;
}

QVariant decoration(const Layout::DecorationState &state)
{
    return QVariantList {state.enabled, state.width};
}

QVariant tabbedPair(Fixture &fixture, const std::function<QVariant(Fixture &, Layout::WindowId, Layout::WindowId)> &read)
{
    const auto first = fixture.add(QStringLiteral("a"));
    const auto second = fixture.add(QStringLiteral("b"));
    fixture.perform(QStringLiteral("consume-or-expel-window-left"));
    return read(fixture, first, second);
}

const QList<Field> &fields()
{
    static const QList<Field> list {
        {"gaps", QStringLiteral("gaps 40"), QStringLiteral("gaps 4"), true, [](Fixture &f) { return frameOf(f, f.add()); }},
        {"struts", QStringLiteral("struts { top 100; }"), QStringLiteral("struts { left 50; }"), true,
            [](Fixture &f) { return frameOf(f, f.add()); }},
        {"center-focused-column", QStringLiteral("center-focused-column \"always\""), QStringLiteral("center-focused-column \"never\""),
            true,
            [](Fixture &f) {
                f.add();
                f.add();
                return frameOf(f, f.add());
            }},
        {"new-column-position", QStringLiteral("new-column-position \"left\""), QStringLiteral("new-column-position \"right\""), true,
            [](Fixture &f) { return columnOfLast(f, {QStringLiteral("a"), QStringLiteral("b")}); }},
        {"always-center-single-column", QStringLiteral("always-center-single-column"), QStringLiteral("always-center-single-column false"),
            true, [](Fixture &f) { return frameOf(f, f.add()); }},
        {"always-expand-single-column", QStringLiteral("always-expand-single-column true"),
            QStringLiteral("always-expand-single-column false"), true, [](Fixture &f) { return frameOf(f, f.add()); }},
        {"empty-workspace-above-first", QStringLiteral("empty-workspace-above-first"), QStringLiteral("empty-workspace-above-first false"),
            false, [](Fixture &f) { return f.state(f.add()).workspaceIndex; }},
        {"default-column-display", QStringLiteral("default-column-display \"tabbed\""), QStringLiteral("default-column-display \"normal\""),
            true, [](Fixture &f) { return tabbedPair(f, [](Fixture &g, auto first, auto) { return QVariant(g.state(first).visible); }); }},
        {"preset-column-widths", QStringLiteral("preset-column-widths { proportion 0.4; proportion 0.9; }"),
            QStringLiteral("preset-column-widths { fixed 500; }"), true,
            [](Fixture &f) {
                const auto id = f.add();
                f.perform(QStringLiteral("switch-preset-column-width"));
                return frameOf(f, id);
            }},
        {"default-column-width", QStringLiteral("default-column-width { proportion 0.25; }"),
            QStringLiteral("default-column-width { fixed 700; }"), true, [](Fixture &f) { return frameOf(f, f.add()); }},
        {"group-app-windows", QStringLiteral("group-app-windows \"stack\""), QStringLiteral("group-app-windows \"off\""), false,
            [](Fixture &f) { return columnOfLast(f, {QStringLiteral("app"), QStringLiteral("other"), QStringLiteral("app")}); }},
        {"max-rows-per-column", QStringLiteral("max-rows-per-column 1"), QStringLiteral("max-rows-per-column 2"), false,
            [](Fixture &f) { return columnOfLast(f, {QStringLiteral("stacker"), QStringLiteral("stacker"), QStringLiteral("stacker")}); }},
        {"new-window-placement", QStringLiteral("new-window-placement \"stack\""), QStringLiteral("new-window-placement \"column\""), false,
            [](Fixture &f) { return columnOfLast(f, {QStringLiteral("a"), QStringLiteral("b")}); }},
        {"preset-window-heights", QStringLiteral("preset-window-heights { fixed 300; }"),
            QStringLiteral("preset-window-heights { fixed 500; }"), true,
            [](Fixture &f) {
                return tabbedPair(f, [](Fixture &g, auto, auto second) {
                    g.perform(QStringLiteral("switch-preset-window-height"));
                    return frameOf(g, second);
                });
            }},
        {"focus-ring", QStringLiteral("focus-ring { width 9; }"), QStringLiteral("focus-ring { off; width 2; }"), true,
            [](Fixture &f) { return decoration(f.state(f.add()).focusRing); }},
        {"border", QStringLiteral("border { on; width 6; }"), QStringLiteral("border { on; width 2; }"), true,
            [](Fixture &f) {
                const auto id = f.add();
                return QVariantList {decoration(f.state(id).border), frameOf(f, id)};
            }},
        {"tab-indicator", QStringLiteral("tab-indicator { width 12; }"), QStringLiteral("tab-indicator { off; width 3; }"), true,
            [](Fixture &f) {
                return tabbedPair(f, [](Fixture &g, auto, auto second) {
                    g.perform(QStringLiteral("toggle-column-tabbed-display"));
                    const Layout::TabBarState bar = g.state(second).tabBar;
                    return QVariant(QVariantList {bar.visible, bar.rect, bar.tabRects.isEmpty() ? QRectF() : bar.tabRects.first()});
                });
            }},
        {"insert-hint", QStringLiteral("insert-hint { color \"#00ff00\"; }"), QStringLiteral("insert-hint { color \"#0000ff\"; }"), false,
            [](Fixture &f) { return QVariant(f.engine().outputStates().first().dropHintPaint.color); }},
        {"background-color", QStringLiteral("background-color \"#112233\""), QStringLiteral("background-color \"#445566\""), true,
            [](Fixture &f) { return QVariant(f.engine().outputStates().first().backgroundColor); }},
    };
    return list;
}

const Field &fieldNamed(const QString &name)
{
    return *std::ranges::find_if(fields(), [&name](const Field &field) { return name == QLatin1String(field.name); });
}

QString unrelatedTo(const Field &field)
{
    return QLatin1String(field.name) == QLatin1String("background-color") ? QStringLiteral("focus-ring { width 7; }")
                                                                          : QStringLiteral("background-color \"#010203\"");
}

QString block(const QString &head, const QStringList &children)
{
    return head + QStringLiteral(" {\n    layout {\n        ") + children.join(QStringLiteral("\n        "))
        + QStringLiteral("\n    }\n}\n");
}

Config::Config configFor(const Levels &levels)
{
    QString text
        = QStringLiteral("animations { off; }\nwindow-rule {\n    match app-id=\"^stacker$\"\n    new-window-placement \"stack\"\n}\n");
    QStringList global = levels.global;
    if (!global.join(QString()).contains(QStringLiteral("always-expand-single-column"))) {
        global.prepend(QStringLiteral("always-expand-single-column false"));
    }
    text += QStringLiteral("layout {\n    ") + global.join(QStringLiteral("\n    ")) + QStringLiteral("\n}\n");
    if (!levels.profile.isEmpty()) {
        text += block(QStringLiteral("monitor-profile \"all\""), levels.profile);
    }
    if (!levels.output.isEmpty()) {
        text += block(QStringLiteral("output \"DP-1\""), levels.output);
    }
    if (levels.workspace) {
        text += levels.workspace->isEmpty() ? QStringLiteral("workspace \"w\"\n")
                                            : block(QStringLiteral("workspace \"w\""), *levels.workspace);
    }
    const auto loaded = Config::loadString(text, QStringLiteral("config.kdl"));
    if (!loaded) {
        qFatal("%s\n%s", qPrintable(loaded.error().toString()), qPrintable(text));
    }
    return loaded->config;
}

QVariant observe(const Field &field, const Levels &levels)
{
    Fixture fixture(configFor(levels));
    const QVariant value = field.probe(fixture);
    const QString problems = fixture.invariants();
    if (!problems.isEmpty()) {
        return problems;
    }
    return value;
}

void addFieldRows(bool workspaceOnly)
{
    QTest::addColumn<QString>("name");
    for (const Field &field : fields()) {
        if (!workspaceOnly || field.inWorkspace) {
            QTest::newRow(field.name) << QString::fromLatin1(field.name);
        }
    }
}

}

class TestLayoutCascade : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everyFieldChangesTheLayout_data() { addFieldRows(false); }

    void everyFieldChangesTheLayout()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        const QVariant plain = observe(field, {{}, {}, {}, {}});
        const QVariant a = observe(field, {{field.a}, {}, {}, {}});
        QVERIFY2(a != plain, qPrintable(a.toString()));
        QVERIFY(a != observe(field, {{field.b}, {}, {}, {}}));
    }

    void profileFieldSurvivesAnOutputLayout_data() { addFieldRows(false); }

    void profileFieldSurvivesAnOutputLayout()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        const QString other = unrelatedTo(field);
        QCOMPARE(observe(field, {{}, {field.a}, {other}, {}}), observe(field, {{field.a}, {}, {other}, {}}));
    }

    void outputFieldSurvivesAWorkspaceLayout_data() { addFieldRows(false); }

    void outputFieldSurvivesAWorkspaceLayout()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        const QStringList other {unrelatedTo(field)};
        QCOMPARE(observe(field, {{}, {}, {field.a}, other}), observe(field, {{field.a}, {}, {}, other}));
    }

    void profileFieldReachesAWorkspaceThroughAnOutput_data() { addFieldRows(false); }

    void profileFieldReachesAWorkspaceThroughAnOutput()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        const QString other = unrelatedTo(field);
        QCOMPARE(observe(field, {{}, {field.a}, {other}, QStringList {other}}), observe(field, {{field.a}, {}, {}, QStringList {other}}));
    }

    void outputFieldBeatsTheProfile_data() { addFieldRows(false); }

    void outputFieldBeatsTheProfile()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        QCOMPARE(observe(field, {{}, {field.a}, {field.b}, {}}), observe(field, {{field.b}, {}, {}, {}}));
    }

    void workspaceFieldBeatsTheOutput_data() { addFieldRows(true); }

    void workspaceFieldBeatsTheOutput()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        QCOMPARE(observe(field, {{}, {field.a}, {field.a}, QStringList {field.b}}), observe(field, {{field.b}, {}, {}, QStringList {}}));
    }

    void globalFieldReachesEveryLevel_data() { addFieldRows(false); }

    void globalFieldReachesEveryLevel()
    {
        QFETCH(QString, name);
        const Field &field = fieldNamed(name);
        const QString other = unrelatedTo(field);
        QCOMPARE(observe(field, {{field.a}, {other}, {other}, QStringList {other}}), observe(field, {{field.a}, {}, {}, QStringList {}}));
    }

    void partsOfADecorationCascadeOneByOne()
    {
        Fixture fixture(configFor({{}, {QStringLiteral("focus-ring { width 9; }"), QStringLiteral("border { width 6; }")},
            {QStringLiteral("focus-ring { off; }"), QStringLiteral("border { on; }")},
            QStringList {QStringLiteral("border { width 3; }")}}));
        const auto id = fixture.add();
        QCOMPARE(decoration(fixture.state(id).focusRing), QVariant(QVariantList {false, 9.0}));
        QCOMPARE(decoration(fixture.state(id).border), QVariant(QVariantList {true, 3.0}));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        const auto below = fixture.add();
        QCOMPARE(decoration(fixture.state(below).border), QVariant(QVariantList {true, 6.0}));
        VERIFY_INVARIANTS(fixture);
    }

    void aReloadRecomputesEveryLevel()
    {
        Fixture fixture(configFor({{}, {QStringLiteral("gaps 40")}, {QStringLiteral("struts { left 100; }")}, {}}));
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(140, 40));
        fixture.setConfig(
            configFor({{QStringLiteral("gaps 10")}, {QStringLiteral("struts { top 50; }")}, {QStringLiteral("gaps 20")}, {}}));
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(20, 70));
        fixture.setConfig(configFor({{QStringLiteral("gaps 10")}, {}, {}, {}}));
        QCOMPARE(fixture.frame(id).topLeft(), QPointF(10, 10));
        VERIFY_INVARIANTS(fixture);
    }

    void profileForAnotherMonitorShapeDoesNotApply()
    {
        Config::Config config = configFor({{}, {}, {QStringLiteral("struts { left 100; }")}, {}});
        Config::MonitorMatch tall;
        tall.aspectRatioBelow = 1.0;
        config.monitorProfiles = {Config::MonitorProfile {QStringLiteral("tall"), {tall}, Config::LayoutPart {}}};
        config.monitorProfiles[0].layout->gaps = 40;
        Fixture fixture(config);
        QCOMPARE(fixture.frame(fixture.add()).topLeft(), QPointF(116, 16));
        fixture.engine().updateOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1080, 1920)));
        fixture.settle();
        QCOMPARE(fixture.frame(fixture.add()).y(), 40.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutCascade)
#include "test_layout_cascade.moc"
