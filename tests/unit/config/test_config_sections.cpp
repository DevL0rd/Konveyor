#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigSections : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hidesDesktopWidgetsFlag();
    void rejectsUnsupportedSections_data();
    void rejectsUnsupportedSections();
};

void TestConfigSections::hidesDesktopWidgetsFlag()
{
    QVERIFY(!parsed(QString()).hideDesktopWidgets);
    QVERIFY(parsed(QStringLiteral("hide-desktop-widgets\n")).hideDesktopWidgets);
    QVERIFY(!parsed(QString()).fillPanelsOnMaximize);
    QVERIFY(parsed(QStringLiteral("fill-panels-on-maximize\n")).fillPanelsOnMaximize);
    QVERIFY(!parsed(QString()).disableMinimize);
    QVERIFY(parsed(QStringLiteral("disable-minimize\n")).disableMinimize);
}

void TestConfigSections::rejectsUnsupportedSections_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("prefer-no-csd") << QStringLiteral("prefer-no-csd\n");
    QTest::newRow("overview") << QStringLiteral("overview {\n zoom 0.25\n}\n");
    QTest::newRow("hotkey-overlay") << QStringLiteral("hotkey-overlay {\n skip-at-startup\n}\n");

    QTest::newRow("layer-rule") << QStringLiteral("layer-rule {\n match namespace=\"^bar$\"\n}\n");
    QTest::newRow("recent-windows") << QStringLiteral("recent-windows {\n off\n}\n");
    QTest::newRow("environment") << QStringLiteral("environment {\n QT_QPA_PLATFORM \"wayland\"\n}\n");
    QTest::newRow("spawn-at-startup") << QStringLiteral("spawn-at-startup \"waybar\"\n");
    QTest::newRow("spawn-sh-at-startup") << QStringLiteral("spawn-sh-at-startup \"true\"\n");
    QTest::newRow("cursor") << QStringLiteral("cursor {\n xcursor-size 16\n}\n");
    QTest::newRow("screenshot-path") << QStringLiteral("screenshot-path \"~/shot.png\"\n");
    QTest::newRow("clipboard") << QStringLiteral("clipboard {\n disable-primary\n}\n");
    QTest::newRow("xwayland-satellite") << QStringLiteral("xwayland-satellite {\n off\n}\n");
    QTest::newRow("switch-events") << QStringLiteral("switch-events {\n lid-close { spawn \"true\"; }\n}\n");
    QTest::newRow("debug") << QStringLiteral("debug {\n render-drm-device \"/dev/dri/renderD129\"\n}\n");
    QTest::newRow("blur") << QStringLiteral("blur {\n passes 3\n}\n");
}

void TestConfigSections::rejectsUnsupportedSections()
{
    QFETCH(QString, text);
    QVERIFY(mustFail(text).message.startsWith(QStringLiteral("unexpected node")));
}

QTEST_MAIN(TestConfigSections)
#include "test_config_sections.moc"
