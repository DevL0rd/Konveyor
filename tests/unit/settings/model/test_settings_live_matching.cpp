#include "config/loader.h"
#include "values/livematching.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;

namespace
{

QVariantMap output(const QString &name, int width, int height)
{
    return {{QStringLiteral("name"), name},
        {QStringLiteral("logical"), QVariantMap {{QStringLiteral("width"), width}, {QStringLiteral("height"), height}}}};
}

QVariantMap window(const QString &appId, int workspaceId)
{
    return {{QStringLiteral("app_id"), appId}, {QStringLiteral("workspace_id"), workspaceId}};
}

Config::Config load(const QString &text)
{
    const auto loaded = Config::loadString(text, QStringLiteral("config.kdl"));
    return loaded ? loaded->config : Config::Config();
}

}

class TestSettingsLiveMatching : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rulesMatchWindowsOnTheOutputOfTheirWorkspace()
    {
        const Config::Config config = load(QStringLiteral("monitor-profile \"wide\" { match width-above=2000; }\n"
                                                          "window-rule {\n    match app-id=\"^term$\" monitor-profile=\"wide\"\n}\n"));
        QVERIFY(!config.windowRules.isEmpty());
        const QVariantList outputs {output(QStringLiteral("DP-1"), 3440, 1440), output(QStringLiteral("DP-2"), 1280, 720)};
        const QVariantList workspaces {QVariantMap {{QStringLiteral("id"), 1}, {QStringLiteral("output"), QStringLiteral("DP-1")}},
            QVariantMap {{QStringLiteral("id"), 2}, {QStringLiteral("output"), QStringLiteral("DP-2")}}};
        const QVariantList windows {
            window(QStringLiteral("term"), 1), window(QStringLiteral("term"), 2), window(QStringLiteral("term"), 3)};
        const QVariantList found = windowsMatchingRule(config, config.windowRules.first(), windows, workspaces, outputs);
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().toMap().value(QStringLiteral("workspace_id")).toInt(), 1);
    }

    void aScopeFollowsTheProfileOfItsOutput()
    {
        const Config::Config config
            = load(QStringLiteral("monitor-profile \"wide\" {\n    match width-above=2000\n    layout {\n        gaps 20\n    }\n}\n"
                                  "output \"DP-1\" {\n    layout {\n        gaps 4\n    }\n}\n"
                                  "output \"DP-3\" {\n    layout {\n        gaps 6\n    }\n}\n"));
        const QVariantList outputs {output(QStringLiteral("DP-1"), 3440, 1440), output(QStringLiteral("DP-2"), 1280, 720)};
        QCOMPARE(scopedLayout(config, QStringLiteral("output"), QStringLiteral("DP-2"), outputs, {}).gaps, config.layout.gaps);
        QCOMPARE(scopedLayout(config, QStringLiteral("output"), QStringLiteral("DP-3"), outputs, {}).gaps, 6.0);
        QCOMPARE(scopedLayout(config, QStringLiteral("output"), QStringLiteral("DP-1"), outputs, {}).gaps, 4.0);
        QCOMPARE(scopedLayout(config, QStringLiteral("monitor-profile"), QStringLiteral("wide"), outputs, {}).gaps, 20.0);
        QCOMPARE(scopedLayout(config, QStringLiteral("monitor-profile"), QStringLiteral("none"), outputs, {}).gaps, config.layout.gaps);
        QCOMPARE(scopedLayout(config, QStringLiteral("window-rule"), QString(), outputs, {}).gaps, config.layout.gaps);
        QCOMPARE(profileNameFor(config, output(QStringLiteral("DP-1"), 3440, 1440)), QStringLiteral("wide"));
    }
};

QTEST_GUILESS_MAIN(TestSettingsLiveMatching)

#include "test_settings_live_matching.moc"
