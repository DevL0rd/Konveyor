#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Layout layoutWithGaps(double gaps)
{
    Config::Layout layout = instantConfig().layout;
    layout.gaps = gaps;
    return layout;
}

Config::Config withOutputLayout(Config::Config config, const QString &name, const Config::Layout &layout)
{
    Config::OutputConfig output;
    output.name = name;
    output.layout = layout;
    config.outputs.append(output);
    return config;
}

Config::Config withProfileLayout(Config::Config config, const Config::Layout &layout, std::optional<double> aspectBelow = std::nullopt)
{
    Config::MonitorProfile profile;
    profile.name = QStringLiteral("profile%1").arg(config.monitorProfiles.size());
    if (aspectBelow) {
        Config::MonitorMatch match;
        match.aspectRatioBelow = aspectBelow;
        profile.matches.append(match);
    }
    profile.layout = layout;
    config.monitorProfiles.append(profile);
    return config;
}

Config::Config withWorkspaceLayout(Config::Config config, const QString &name, const Config::Layout &layout)
{
    Config::NamedWorkspace workspace;
    workspace.name = name;
    workspace.layout = layout;
    config.workspaces.append(workspace);
    return config;
}

double gapOf(Fixture &fixture, Layout::WindowId id)
{
    return fixture.frame(id).y();
}

}

class TestLayoutOverrides : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void profileAppliesToMatchingOutputsOnly()
    {
        Fixture fixture(withProfileLayout(instantConfig(), layoutWithGaps(40), 1.0));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1080, 1920));
        const auto landscape = fixture.add(QStringLiteral("a"));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto portrait = fixture.add(QStringLiteral("b"));
        QCOMPARE(gapOf(fixture, landscape), 16.0);
        QCOMPARE(gapOf(fixture, portrait), 40.0);
        VERIFY_INVARIANTS(fixture);
    }

    void firstMatchingProfileWins()
    {
        Config::Config config = withProfileLayout(instantConfig(), layoutWithGaps(24));
        config = withProfileLayout(config, layoutWithGaps(40));
        Fixture fixture(config);
        QCOMPARE(gapOf(fixture, fixture.add()), 24.0);
    }

    void outputLayoutReplacesTheProfileInsteadOfStacking()
    {
        Config::Layout profile = layoutWithGaps(40);
        profile.struts = Config::Struts {100, 0, 0, 0};
        Config::Config config = withProfileLayout(instantConfig(), profile);
        config = withOutputLayout(config, QStringLiteral("DP-1"), layoutWithGaps(8));
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id), QRectF(8, 8, 948, 1064));
    }

    void outputNamesMatchWithoutCase()
    {
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("dp-1"), layoutWithGaps(8)));
        QCOMPARE(gapOf(fixture, fixture.add()), 8.0);
    }

    void outputLayoutsStayOnTheirOwnOutput()
    {
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("DP-2"), layoutWithGaps(0)));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto left = fixture.add(QStringLiteral("a"));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto right = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(left), QRectF(16, 16, 936, 1048));
        QCOMPARE(fixture.frame(right), QRectF(1920, 0, 960, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceLayoutReplacesTheOutputLayout()
    {
        Config::Config config = withOutputLayout(instantConfig(), QStringLiteral("DP-1"), layoutWithGaps(40));
        config = withWorkspaceLayout(config, QStringLiteral("tight"), layoutWithGaps(4));
        Fixture fixture(config);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("tight")}).ok);
        const auto inside = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto outside = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(inside).workspaceIndex, 1);
        QCOMPARE(fixture.state(outside).workspaceIndex, 2);
        QCOMPARE(gapOf(fixture, outside), 40.0);
        fixture.perform(QStringLiteral("focus-workspace-up"));
        QCOMPARE(gapOf(fixture, inside), 4.0);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceLayoutFollowsAReload()
    {
        Config::Config config = withWorkspaceLayout(instantConfig(), QStringLiteral("tight"), layoutWithGaps(4));
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("a"));
        QCOMPARE(gapOf(fixture, id), 4.0);
        config.workspaces[0].layout = layoutWithGaps(30);
        fixture.setConfig(config);
        QCOMPARE(gapOf(fixture, id), 30.0);
        config.workspaces[0].layout.reset();
        fixture.setConfig(config);
        QCOMPARE(gapOf(fixture, id), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void outputLayoutFollowsAReload()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        const auto id = fixture.add();
        fixture.setConfig(withOutputLayout(config, QStringLiteral("DP-1"), layoutWithGaps(30)));
        QCOMPARE(gapOf(fixture, id), 30.0);
        fixture.setConfig(config);
        QCOMPARE(gapOf(fixture, id), 16.0);
    }

    void strutsChangeLive()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        const auto id = fixture.add();
        config.layout.struts = Config::Struts {50, 30, 20, 10};
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(id), QRectF(66, 36, std::floor(1824.0 * 0.5 - 16.0), 1018));
        config.layout.struts = Config::Struts {-20, 0, 0, 0};
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(id).x(), -4.0);
        VERIFY_INVARIANTS(fixture);
    }

    void gapsSnapToPhysicalPixelsOnAScaledOutput()
    {
        Config::Config config = instantConfig();
        config.layout.gaps = 15;
        Fixture fixture(config);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080), 1.5);
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto scaled = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(scaled).y(), 23.0 / 1.5);
        QCOMPARE(fixture.frame(scaled).x(), 1920.0 + 23.0 / 1.5);
        VERIFY_INVARIANTS(fixture);
    }

    void placementSettingsFollowTheOutputLayout()
    {
        Config::Layout stacking = instantConfig().layout;
        stacking.newWindowPlacement = Config::NewWindowPlacement::Stack;
        stacking.maxRowsPerColumn = 2;
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("DP-2"), stacking));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.state(a).columnIndex != fixture.state(b).columnIndex);
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto c = fixture.add(QStringLiteral("c"));
        const auto d = fixture.add(QStringLiteral("d"));
        const auto e = fixture.add(QStringLiteral("e"));
        QCOMPARE(fixture.state(d).columnIndex, fixture.state(c).columnIndex);
        QVERIFY(fixture.state(e).columnIndex != fixture.state(c).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void placementSettingsFollowTheWorkspaceLayout()
    {
        Config::Layout stacking = instantConfig().layout;
        stacking.newWindowPlacement = Config::NewWindowPlacement::Stack;
        Fixture fixture(withWorkspaceLayout(instantConfig(), QStringLiteral("stack"), stacking));
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(a).workspaceIndex, 1);
        QCOMPARE(fixture.state(b).columnIndex, fixture.state(a).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void groupAppWindowsFollowsTheOutputLayout()
    {
        Config::Layout grouping = instantConfig().layout;
        grouping.groupAppWindows = Config::GroupAppWindows::Stack;
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("DP-1"), grouping));
        const auto first = fixture.add(QStringLiteral("app"));
        fixture.add(QStringLiteral("other"));
        const auto second = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(second).columnIndex, fixture.state(first).columnIndex);
    }

    void floatChildWindowsFollowsTheOutputLayout()
    {
        Config::Layout floating = instantConfig().layout;
        floating.floatChildWindows = true;
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("DP-2"), floating));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.add(QStringLiteral("app"));
        QVERIFY(!fixture.state(fixture.add(QStringLiteral("app"))).isFloating);
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto child = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(child).output, QStringLiteral("DP-2"));
        QVERIFY(fixture.state(child).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void defaultColumnDisplayFollowsTheOutputLayout()
    {
        Config::Layout tabbed = instantConfig().layout;
        tabbed.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("DP-1"), tabbed));
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(!fixture.state(first).visible);
    }

    void newColumnPositionFollowsTheOutputLayout()
    {
        Config::Layout left = instantConfig().layout;
        left.newColumnPosition = Config::NewColumnPosition::Left;
        Fixture fixture(withOutputLayout(instantConfig(), QStringLiteral("DP-1"), left));
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 1);
        QCOMPARE(fixture.frame(second).x(), 16.0);
    }
};

QTEST_GUILESS_MAIN(TestLayoutOverrides)
#include "test_layout_overrides.moc"
