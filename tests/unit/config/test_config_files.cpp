#include "config/loader.h"

#include <QTest>

using namespace Konveyor::Config;

class TestConfigFiles : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadsRepositoryDefaultConfig();
    void reportsMissingFile();
    void configPathHonoursEnvironment();

private:
    static void loadKdlFile(const QString &path);
};

void TestConfigFiles::loadKdlFile(const QString &path)
{
    const auto result = loadFile(path);
    if (!result) {
        QFAIL(qPrintable(result.error().toString()));
    }
    QVERIFY(!result->config.binds.isEmpty());
    QVERIFY(result->files.contains(path));
}

void TestConfigFiles::loadsRepositoryDefaultConfig()
{
    const QString path = QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl");
    loadKdlFile(path);

    const auto result = loadFile(path);
    QVERIFY(result.has_value());
    QCOMPARE(result->config.gestures.titlebarDrag, TitlebarDrag::ScrollView);
    QCOMPARE(result->config.gestures.hotCorners.enabled, false);
    QCOMPARE(result->config.layout.focusRing.active.source, ColorSource::SystemAccent);
    QCOMPARE(result->config.layout.border.enabled, false);
    QVERIFY(!result->config.windowRules.isEmpty());
    QCOMPARE(result->config.monitorProfiles.size(), 1);
    const MonitorProfile &ultrawide = result->config.monitorProfiles.at(0);
    QCOMPARE(ultrawide.matches.value(0).aspectRatioAbove, std::optional(2.0));
    QCOMPARE(std::get<Proportion>(*ultrawide.layout->defaultColumnWidth).value, 0.25);
    QCOMPARE(std::get<Proportion>(*result->config.layout.defaultColumnWidth).value, 0.25);
    QCOMPARE(result->config.layout.rememberWindowSizes, true);
    QCOMPARE(result->config.layout.rememberWindowPositions, false);
    const auto wezterm = std::ranges::find_if(result->config.windowRules, [](const WindowRule &rule) {
        return !rule.matches.isEmpty() && rule.matches.first().appId
            && rule.matches.first().appId->pattern() == QStringLiteral(R"(^org\.wezfurlong\.wezterm$)");
    });
    QVERIFY(wezterm != result->config.windowRules.end());
    QVERIFY(wezterm->defaultColumnWidth.has_value());
    QVERIFY(!wezterm->defaultColumnWidth->has_value());

    const auto showOverlay = std::ranges::find_if(
        result->config.binds, [](const Bind &bind) { return bind.action.name == QStringLiteral("show-hotkey-overlay"); });
    QVERIFY(showOverlay != result->config.binds.end());
    QCOMPARE(showOverlay->keyText, QStringLiteral("K"));
    QCOMPARE(showOverlay->hotkeyOverlayTitle, QStringLiteral("Show Shortcut Cheatsheet"));
}

void TestConfigFiles::reportsMissingFile()
{
    const auto result = loadFile(QStringLiteral("/nonexistent/konveyor/config.kdl"));
    QVERIFY(!result.has_value());
    QVERIFY(result.error().message.startsWith(QStringLiteral("error reading ")));
}

void TestConfigFiles::configPathHonoursEnvironment()
{
    qputenv("KONVEYOR_CONFIG", "/tmp/explicit.kdl");
    QCOMPARE(configPath(), QStringLiteral("/tmp/explicit.kdl"));
    qunsetenv("KONVEYOR_CONFIG");

    qputenv("XDG_CONFIG_HOME", "/tmp/xdg");
    QCOMPARE(configPath(), QStringLiteral("/tmp/xdg/konveyor/config.kdl"));
    qunsetenv("XDG_CONFIG_HOME");
    QVERIFY(configPath().endsWith(QStringLiteral("/.config/konveyor/config.kdl")));
}

QTEST_MAIN(TestConfigFiles)
#include "test_config_files.moc"
