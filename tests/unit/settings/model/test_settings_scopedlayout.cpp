#include "config/loader.h"
#include "values/configvalues.h"
#include "values/livematching.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;

class TestSettingsScopedLayout : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void perScope_data();
    void perScope();
};

void TestSettingsScopedLayout::perScope_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("gaps");
    QTest::addColumn<bool>("centered");
    QTest::newRow("global") << QString() << QString() << 1 << false;
    QTest::newRow("layout") << QStringLiteral("layout") << QString() << 1 << false;
    QTest::newRow("unknown kind") << QStringLiteral("window-rule") << QStringLiteral("DP-1") << 1 << false;
    QTest::newRow("output over its profile") << QStringLiteral("output") << QStringLiteral("DP-1") << 2 << true;
    QTest::newRow("output other case") << QStringLiteral("output") << QStringLiteral("dp-1") << 2 << true;
    QTest::newRow("output without layout uses its profile") << QStringLiteral("output") << QStringLiteral("HDMI-A-1") << 3 << true;
    QTest::newRow("output unplugged") << QStringLiteral("output") << QStringLiteral("DP-9") << 1 << false;
    QTest::newRow("profile") << QStringLiteral("monitor-profile") << QStringLiteral("Wide") << 3 << true;
    QTest::newRow("profile is exact") << QStringLiteral("monitor-profile") << QStringLiteral("wide") << 5 << false;
    QTest::newRow("profile unknown") << QStringLiteral("monitor-profile") << QStringLiteral("tall") << 1 << false;
    QTest::newRow("workspace on its monitor") << QStringLiteral("workspace") << QStringLiteral("Mail") << 4 << true;
    QTest::newRow("workspace other case") << QStringLiteral("workspace") << QStringLiteral("MAIL") << 4 << true;
    QTest::newRow("workspace waiting for its output") << QStringLiteral("workspace") << QStringLiteral("chat") << 3 << true;
    QTest::newRow("workspace on no monitor") << QStringLiteral("workspace") << QStringLiteral("notes") << 1 << false;
    QTest::newRow("workspace unknown") << QStringLiteral("workspace") << QStringLiteral("none") << 1 << false;
}

void TestSettingsScopedLayout::perScope()
{
    QFETCH(QString, kind);
    QFETCH(QString, name);
    QFETCH(int, gaps);
    QFETCH(bool, centered);
    const auto loaded = Config::loadString(QStringLiteral(R"(
layout { gaps 1; center-focused-column "always"; }
output "DP-1" { layout { gaps 2; }; }
output "HDMI-A-1" { hot-corners { off; }; }
monitor-profile "Wide" { match aspect-ratio-above=2.0; layout { gaps 3; always-center-single-column; }; }
monitor-profile "wide" { match aspect-ratio-above=3.0; layout { gaps 5; }; }
workspace "mail" { layout { gaps 4; }; }
workspace "chat" { open-on-output "HDMI-A-1"; }
workspace "notes"
)"),
        QStringLiteral("config.kdl"));
    QVERIFY(loaded);
    const Config::Config config = loaded->config;
    const QVariantMap logical {{QStringLiteral("width"), 2560}, {QStringLiteral("height"), 1080}};
    const QVariantList outputs {QVariantMap {{QStringLiteral("name"), QStringLiteral("DP-1")}, {QStringLiteral("logical"), logical}},
        QVariantMap {{QStringLiteral("name"), QStringLiteral("HDMI-A-1")}, {QStringLiteral("logical"), logical}}};
    const QVariantList workspaces {
        QVariantMap {{QStringLiteral("name"), QStringLiteral("mail")}, {QStringLiteral("output"), QStringLiteral("DP-1")}}};
    const Config::Layout layout = scopedLayout(config, kind, name, outputs, workspaces);
    QCOMPARE(layout.gaps, double(gaps));
    QCOMPARE(layout.alwaysCenterSingleColumn, centered);
    QCOMPARE(layoutValues(layout).value(QStringLiteral("center-focused-column")).toString(), QStringLiteral("always"));
}

QTEST_GUILESS_MAIN(TestSettingsScopedLayout)
#include "test_settings_scopedlayout.moc"
