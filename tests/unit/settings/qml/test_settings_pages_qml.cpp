#include "settingsqmlharness.h"

#include <QQuickWindow>

using namespace Konveyor::Settings::Testing;

namespace
{

const QString RichConfig = QStringLiteral(R"(input { focus-follows-mouse; warp-mouse-to-focus mode="center-xy"; mod-key "Alt"; }
layout {
    gaps 8
    tab-indicator { place-within-column; active-gradient from="#000" to="#fff" angle=90 in="oklch longer hue"; }
    insert-hint { color "#ff000080"; }
    border { on; active-color "hover"; }
}
gestures { hot-corners { top-right; bottom-left; }; touchscreen { off; }; }
animations { window-open { spring damping-ratio=0.8 stiffness=500 epsilon=0.001; }; window-resize { off; }; }
output "DP-1" { hot-corners { off; }; layout { gaps 2; focus-ring { off; }; }; }
monitor-profile "wide" { match aspect-ratio-above=2.0 name="^DP"; layout { max-rows-per-column 2; }; }
workspace "mail" { open-on-output "DP-1"; layout { gaps 3; }; }
window-rule {
    match app-id=r#"^org\.kde\.dolphin$"# title="x" is-floating=true
    exclude title="^Copy"
    open-floating true
    default-column-width { fixed 800; }
    default-floating-position x=10 y=20 relative-to="bottom-right"
    opacity 0.8
    geometry-corner-radius 1 2 3 4
    focus-ring { off; width 3; active-color "#123456"; }
    border { active-gradient from="#000" to="#fff"; }
    min-width 100
    max-height 900
    column-position "start" { max-stack 2; }
}
binds {
    Mod+T { spawn "konsole"; }
    Mod+Shift+1 { move-window-to-workspace "mail" focus=false; }
    Mod+WheelScrollDown cooldown-ms=150 { focus-workspace-down; }
}
)");

}

class TestSettingsPagesQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void loadsWithoutWarnings_data();
    void loadsWithoutWarnings();

private:
    SettingsHome m_home;
};

void TestSettingsPagesQml::loadsWithoutWarnings_data()
{
    QTest::addColumn<QString>("config");
    QTest::addColumn<bool>("hasConfig");
    QTest::newRow("shipped") << QString() << false;
    QTest::newRow("empty") << QString() << true;
    QTest::newRow("rich") << RichConfig << true;
    QTest::newRow("error") << QStringLiteral("layout {\n    gaps \"wide\"\n}\nwindow-rule {\n}\n") << true;
}

void TestSettingsPagesQml::loadsWithoutWarnings()
{
    QFETCH(QString, config);
    QFETCH(bool, hasConfig);
    m_home.resetConfig(hasConfig ? std::optional(config) : std::nullopt);
    WarningLog log;
    QQmlEngine engine;
    QString error;
    const std::unique_ptr<QObject> root = m_home.create(engine,
        "import QtQuick.Window\n"
        "Window {\n"
        "    width: 1100; height: 800; visible: true\n"
        "    property alias view: settings\n"
        "    SettingsView { id: settings; anchors.fill: parent }\n"
        "}\n",
        &error);
    QVERIFY2(root, qPrintable(error));
    auto *window = qobject_cast<QQuickWindow *>(root.get());
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject *view = root->property("view").value<QObject *>();
    QObject *navigation
        = engine.singletonInstance<QObject *>(QStringLiteral("org.kde.konveyor.settings"), QStringLiteral("SettingsNavigation"));
    QVERIFY(view && navigation);
    const auto settle = [&]() {
        window->grabWindow();
        QCoreApplication::processEvents();
        window->grabWindow();
    };
    const QStringList ids {QStringLiteral("layout"), QStringLiteral("look"), QStringLiteral("motion"), QStringLiteral("mouse"),
        QStringLiteral("touch"), QStringLiteral("shortcuts"), QStringLiteral("rules"), QStringLiteral("monitors"),
        QStringLiteral("workspaces"), QStringLiteral("taskbar"), QStringLiteral("plasma"), QStringLiteral("experiments")};
    const QString pagesSource = SettingsHome::read(m_home.sourcePath(QStringLiteral("catalog/Pages.js")));
    QCOMPARE(pagesSource.count(QStringLiteral("{ id: \"")), ids.size());
    for (const QString &id : ids) {
        QVERIFY2(pagesSource.contains(QStringLiteral("{ id: \"") + id + QLatin1Char('"')), qPrintable(id));
        view->setProperty("pageId", id);
        settle();
        QCOMPARE(log.take(), QStringList {});
        QCOMPARE(navigation->property("depth").toInt(), 1);
    }
    const QList<std::pair<QString, QVariantMap>> subpages {
        {QStringLiteral("pages/RuleEditorPage.qml"), {{QStringLiteral("rulePath"), QStringLiteral("window-rule")}}},
        {QStringLiteral("pages/RuleEditorPage.qml"), {{QStringLiteral("rulePath"), QStringLiteral("window-rule#40")}}},
        {QStringLiteral("pages/MonitorProfilePage.qml"), {{QStringLiteral("profilePath"), QStringLiteral("monitor-profile")}}},
        {QStringLiteral("pages/MonitorProfilePage.qml"), {{QStringLiteral("profilePath"), QStringLiteral("monitor-profile#40")}}},
        {QStringLiteral("pages/OutputOverridePage.qml"), {{QStringLiteral("outputPath"), QStringLiteral("output")}}},
        {QStringLiteral("pages/OutputOverridePage.qml"), {{QStringLiteral("outputPath"), QStringLiteral("output#40")}}},
    };
    for (const auto &[file, properties] : subpages) {
        QVERIFY(QMetaObject::invokeMethod(navigation, "push", Q_ARG(QString, file), Q_ARG(QVariantMap, properties)));
        settle();
        QCOMPARE(navigation->property("depth").toInt(), 2);
        QCOMPARE(log.take(), QStringList {});
        QVERIFY(QMetaObject::invokeMethod(navigation, "pop"));
        settle();
        QCOMPARE(navigation->property("depth").toInt(), 1);
    }
    QCOMPARE(log.take(), QStringList {});
}

QTEST_MAIN(TestSettingsPagesQml)
#include "test_settings_pages_qml.moc"
