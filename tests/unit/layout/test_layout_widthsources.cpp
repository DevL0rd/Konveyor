#include "helpers.h"

using namespace LayoutTest;

namespace
{

constexpr double Quarter = 460.0;
constexpr double Half = 936.0;
constexpr double ThreeQuarters = 1412.0;
constexpr double Full = 1888.0;

Config::Config withRule(Config::Config config, std::optional<Config::PresetSize> width)
{
    Config::WindowRule rule = ruleFor(QStringLiteral("app"));
    rule.defaultColumnWidth = width;
    config.windowRules.append(rule);
    return config;
}

Config::Config withProfile(Config::Config config, double width)
{
    Config::MonitorProfile profile;
    profile.name = QStringLiteral("any");
    Config::LayoutPart layout;
    layout.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Proportion {width});
    profile.layout = layout;
    config.monitorProfiles.append(profile);
    return config;
}

Config::Config withMemory(Config::Config config)
{
    config.layout.rememberWindowSizes = true;
    return config;
}

void remember(Fixture &fixture, double proportion)
{
    Layout::WindowMemory memory;
    memory[QStringLiteral("app")].columnWidth = Layout::ColumnWidth::proportion(proportion);
    fixture.engine().setWindowMemory(memory);
}

double openedWidth(Fixture &fixture, QSizeF size = QSizeF(700, 500))
{
    fixture.add(QStringLiteral("other"));
    return fixture.frame(fixture.add(QStringLiteral("app"), size)).width();
}

}

class TestLayoutWidthSources : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void globalDefaultIsTheFallback()
    {
        Fixture fixture;
        QCOMPARE(openedWidth(fixture), Half);
    }

    void unsetGlobalDefaultKeepsTheWindowSizeWithItsBorder()
    {
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth.reset();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        Fixture fixture(config);
        QCOMPARE(openedWidth(fixture), 700.0);
    }

    void profileBeatsGlobal()
    {
        Fixture fixture(withProfile(instantConfig(), 0.75));
        QCOMPARE(openedWidth(fixture), ThreeQuarters);
    }

    void memoryBeatsProfileAndGlobal()
    {
        Fixture fixture(withProfile(withMemory(instantConfig()), 0.75));
        remember(fixture, 0.25);
        QCOMPARE(openedWidth(fixture), Quarter);
    }

    void memoryIsIgnoredWhileRememberingIsOff()
    {
        Fixture fixture(withProfile(instantConfig(), 0.75));
        remember(fixture, 0.25);
        QCOMPARE(openedWidth(fixture), ThreeQuarters);
    }

    void ruleBeatsMemoryProfileAndGlobal()
    {
        Fixture fixture(withRule(withProfile(withMemory(instantConfig()), 0.75), Config::Proportion {1.0}));
        remember(fixture, 0.25);
        QCOMPARE(openedWidth(fixture), Full);
    }

    void emptyRuleKeepsTheWindowSizeOverEverything()
    {
        Fixture fixture(withRule(withProfile(withMemory(instantConfig()), 0.75), std::nullopt));
        remember(fixture, 0.25);
        QCOMPARE(openedWidth(fixture), 700.0);
    }

    void openMaximizedBeatsTheRule()
    {
        Config::Config config = withRule(instantConfig(), Config::Proportion {0.25});
        Fixture fixture(config);
        fixture.add(QStringLiteral("other"));
        Layout::WindowProperties properties = makeWindow(QStringLiteral("app"), QStringLiteral("app"));
        properties.wantsMaximized = true;
        const auto id = fixture.addWith(properties);
        QCOMPARE(fixture.frame(id).width(), Full);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(id).width(), Quarter);
    }

    void presetListFromTheProfileReplacesTheGlobalOne()
    {
        Config::Config config = withProfile(instantConfig(), 0.5);
        config.monitorProfiles[0].layout->presetColumnWidths
            = QList<Config::PresetSize> {Config::Proportion {0.25}, Config::Proportion {0.5}};
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(id).widthPresetCount, 2);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(id).width(), Quarter);
    }

    void presetsDoNotChangeTheOpeningWidth()
    {
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth = Config::Proportion {0.4};
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.frame(id).width(), std::floor(1904.0 * 0.4 - 16.0));
        QCOMPARE(fixture.state(id).widthPresetIndex, std::nullopt);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(id).width(), Half);
    }

    void expandingALoneColumnNeverReachesMemory()
    {
        Config::Config config = withMemory(instantConfig());
        config.layout.alwaysExpandSingleColumn = true;
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.frame(id).width(), Full);
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("app")).columnWidth, Layout::ColumnWidth::proportion(0.5));
        fixture.remove(id);
        QCOMPARE(openedWidth(fixture), Half);
    }

    void maximizedColumnsAreNotRemembered()
    {
        Fixture fixture(withMemory(instantConfig()));
        fixture.add(QStringLiteral("app"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")});
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("app")).columnWidth, Layout::ColumnWidth::proportion(0.25));
    }

    void liveDefaultChangeMovesColumnsStillAtTheOldDefault()
    {
        Fixture fixture;
        const auto untouched = fixture.add(QStringLiteral("a"));
        const auto resized = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("30%")});
        const double resizedWidth = fixture.frame(resized).width();
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth = Config::Proportion {0.25};
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(untouched).width(), Quarter);
        QCOMPARE(fixture.frame(resized).width(), resizedWidth);
    }

    void liveDefaultChangeKeepsColumnsSizedByHand()
    {
        Fixture fixture(withRule(instantConfig(), Config::Proportion {0.75}));
        const auto ruled = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.frame(ruled).width(), ThreeQuarters);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("30%")});
        const double resizedWidth = fixture.frame(ruled).width();
        Config::Config config = withRule(instantConfig(), Config::Proportion {0.75});
        config.layout.defaultColumnWidth = Config::Proportion {0.25};
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(ruled).width(), resizedWidth);
    }

    void liveFixedDefaultChangeKeepsTheBorderOutside()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        config.layout.defaultColumnWidth = Config::Fixed {500};
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(first).width(), 500.0);
        config.layout.defaultColumnWidth = Config::Fixed {600};
        fixture.setConfig(config);
        const auto second = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(second).width(), 600.0);
        QCOMPARE(fixture.frame(first).width(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }

    void liveDefaultChangeKeepsMaximizedColumns()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("maximize-column"));
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth = Config::Proportion {0.25};
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(id).width(), Full);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(id).width(), Half);
    }

    void unstackingOnRotationUsesTheFixedDefaultWithItsBorder()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        config.layout.defaultColumnWidth = Config::Fixed {600};
        Config::MonitorProfile portrait;
        portrait.name = QStringLiteral("portrait");
        Config::MonitorMatch match;
        match.aspectRatioBelow = 1.0;
        portrait.matches.append(match);
        portrait.layout = Config::LayoutPart {};
        portrait.layout->newWindowPlacement = Config::NewWindowPlacement::Stack;
        portrait.layout->maxRowsPerColumn = 2;
        config.monitorProfiles.append(portrait);
        Fixture fixture(config, QRectF(0, 0, 1080, 1920));
        const auto top = fixture.add(QStringLiteral("a"));
        const auto bottom = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(bottom).columnIndex, fixture.state(top).columnIndex);
        fixture.engine().updateOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080)));
        fixture.settle();
        QVERIFY(fixture.state(bottom).columnIndex != fixture.state(top).columnIndex);
        QCOMPARE(fixture.frame(top).width(), 600.0);
        QCOMPARE(fixture.frame(bottom).width(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }

    void profileDefaultChangesWithTheMonitorShape()
    {
        Config::Config config = instantConfig();
        Config::MonitorProfile portrait;
        portrait.name = QStringLiteral("portrait");
        Config::MonitorMatch match;
        match.aspectRatioBelow = 1.0;
        portrait.matches.append(match);
        portrait.layout = Config::LayoutPart {};
        portrait.layout->defaultColumnWidth = std::optional<Config::PresetSize>(Config::Proportion {1.0});
        config.monitorProfiles.append(portrait);
        Fixture fixture(config);
        const auto landscape = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(landscape).width(), Half);
        fixture.engine().updateOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1080, 1920)));
        fixture.settle();
        QCOMPARE(fixture.frame(landscape).width(), 1048.0);
        const auto rotated = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(rotated).width(), 1048.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWidthSources)
#include "test_layout_widthsources.moc"
