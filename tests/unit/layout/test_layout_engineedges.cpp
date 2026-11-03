#include "helpers.h"

using namespace LayoutTest;

class TestLayoutEngineEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void widgetToolsNeedAHook()
    {
        Fixture fixture;
        fixture.engine().runWidgetTool(QStringLiteral("tool"), {});
        QStringList seen;
        Layout::Hooks hooks;
        hooks.runWidgetTool = [&seen](const QString &name, const QStringList &) { seen.append(name); };
        Fixture hooked(instantConfig(), QRectF(0, 0, 1920, 1080), hooks);
        hooked.engine().runWidgetTool(QStringLiteral("tool"), {});
        QCOMPARE(seen, QStringList {QStringLiteral("tool")});
    }

    void unknownOutputsAreIgnored()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.engine().focusOutput(QStringLiteral("NOPE"));
        fixture.engine().removeOutput(QStringLiteral("NOPE"));
        fixture.engine().focusWorkspace(QStringLiteral("NOPE"), 1);
        fixture.engine().focusWorkspace(QStringLiteral("DP-1"), 0);
        QCOMPARE(fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-1")));
        QCOMPARE(fixture.focused(), std::optional(id));
        VERIFY_INVARIANTS(fixture);
    }

    void inactiveFocusRingsFollowTheirPaintAndStyle_data()
    {
        QTest::addColumn<bool>("paint");
        QTest::addColumn<QString>("style");
        QTest::addColumn<bool>("shown");
        QTest::newRow("nothing") << false << QString() << false;
        QTest::newRow("paint") << true << QString() << true;
        QTest::newRow("style") << false << QStringLiteral("glow") << true;
        QTest::newRow("none style") << false << QStringLiteral("none") << false;
    }

    void inactiveFocusRingsFollowTheirPaintAndStyle()
    {
        QFETCH(bool, paint);
        QFETCH(QString, style);
        QFETCH(bool, shown);
        Config::Config config = instantConfig();
        config.layout.focusRing.enabled = true;
        config.layout.focusRing.inactiveStyle = style;
        if (paint) {
            config.layout.focusRing.inactive.color = QColor(Qt::red);
        }
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(first).focusRing.enabled, shown);
        QCOMPARE(fixture.state(first).focusRing.paint.isSet(), paint);
    }

    void tabBarsHideForSingleTabsAndExpandedColumns()
    {
        Config::Config config = instantConfig();
        config.layout.tabIndicator.hideWhenSingleTab = true;
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("tabbed")});
        QVERIFY(!fixture.state(first).tabBar.visible);
        const auto second = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(fixture.state(second).tabBar.visible);
        fixture.perform(QStringLiteral("fullscreen-window"));
        QVERIFY(!fixture.state(second).tabBar.visible);
        VERIFY_INVARIANTS(fixture);
    }

    void rememberingSkipsWindowsWithoutAnAppOrASize()
    {
        Config::Config config = instantConfig();
        config.layout.rememberWindowSizes = true;
        config.layout.rememberWindowPositions = true;
        Config::WindowRule rule = ruleFor(QStringLiteral("ruled"));
        rule.openFloating = true;
        rule.defaultWindowHeight = std::optional<Config::PresetSize>(Config::Fixed {300});
        config.windowRules.append(rule);
        Fixture fixture(config);
        fixture.add(QString());
        const auto nameless = fixture.add(QString());
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(nameless).isFloating);
        const auto empty = fixture.addWith(makeWindow(QStringLiteral("empty"), {}, QSizeF()));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(empty).isFloating);
        const auto ruled = fixture.addWith(makeWindow(QStringLiteral("ruled"), {}, QSizeF(500, 400)));
        QVERIFY(fixture.state(ruled).isFloating);
        fixture.advance(1);
        fixture.remove(ruled);
        const auto again = fixture.addWith(makeWindow(QStringLiteral("ruled"), {}, QSizeF(500, 400)));
        QVERIFY(fixture.state(again).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void rememberingPositionsWithoutSizesStoresPositionsOnly()
    {
        Config::Config config = instantConfig();
        config.layout.rememberWindowPositions = true;
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        QVERIFY(fixture.state(id).isFloating);
        fixture.advance(1);
        const Layout::WindowMemory memory = fixture.engine().windowMemory();
        QVERIFY(memory.contains(QStringLiteral("a")));
        QVERIFY(!memory.value(QStringLiteral("a")).floatingSize.has_value());
        QVERIFY(memory.value(QStringLiteral("a")).floatingPosition.has_value());
    }

    void outputsWithoutAScaleUseOne()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080), 0.0);
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto id = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void hitTestingIgnoresPointsOutsideEveryOutput()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(!fixture.engine().windowAt(QPointF(-5000, -5000)).has_value());
        QVERIFY(!fixture.engine().hitAt(QPointF(9000, 9000)).has_value());
        QCOMPARE(fixture.engine().windowAt(fixture.frame(id).center()), std::optional(id));
    }

    void actionArgumentsAreValidated()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        QVERIFY(!fixture.perform(QStringLiteral("focus-taskbar-item"), {QStringLiteral("bogus")}).ok);
        QVERIFY(!fixture.perform(QStringLiteral("move-floating-window"), {}, {{QStringLiteral("y"), QStringLiteral("bogus")}}).ok);
        QVERIFY(!fixture.perform(QStringLiteral("unset-workspace-name"), {QStringLiteral("300")}).ok);
        QVERIFY(!fixture
                .perform(
                    QStringLiteral("set-workspace-name"), {QStringLiteral("web")}, {{QStringLiteral("workspace"), QStringLiteral("300")}})
                .ok);
        QVERIFY(!fixture.perform(QStringLiteral("unset-workspace-name"), {QStringLiteral("nothing")}).ok);
    }

    void movingWindowIsOnlyReportedOnceItDetaches()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(!fixture.engine().movingWindow().has_value());
        QVERIFY(fixture.engine().beginWindowDrag(id, fixture.frame(id).center()));
        QVERIFY(!fixture.engine().movingWindow().has_value());
        fixture.engine().endWindowDrag();
    }

    void outputViewsDescribeEveryWorkspace()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const QList<Layout::OutputViewState> views = fixture.engine().outputViewStates();
        QVERIFY(!views.isEmpty());
        QCOMPARE(views.front().output, QStringLiteral("DP-1"));
        QCOMPARE(views.front().width, 1920.0);
    }

    void outputLayoutsComeFromProfilesAndOutputEntries()
    {
        Config::Config config = instantConfig();
        Config::MonitorProfile plainProfile;
        plainProfile.name = QStringLiteral("plain");
        config.monitorProfiles.append(plainProfile);
        Config::OutputConfig plainOutput;
        plainOutput.name = QStringLiteral("DP-1");
        config.outputs.append(plainOutput);
        Fixture plain(config);
        plain.add(QStringLiteral("a"));
        VERIFY_INVARIANTS(plain);

        Config::Config styled = instantConfig();
        Config::LayoutPart part;
        part.gaps = 40.0;
        Config::MonitorProfile profile;
        profile.name = QStringLiteral("wide");
        profile.layout = part;
        styled.monitorProfiles.append(profile);
        Config::OutputConfig output;
        output.name = QStringLiteral("dp-2");
        Config::LayoutPart outputPart;
        outputPart.gaps = 8.0;
        output.layout = outputPart;
        styled.outputs.append(output);
        Fixture fixture(styled);
        const auto first = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(first).top(), 40.0);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(second).top(), 8.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutEngineEdges)
#include "test_layout_engineedges.moc"
