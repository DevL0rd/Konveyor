#include "plasmoidharness.h"

#include <QTest>

namespace
{

const PlasmoidSpec taskbar {QStringLiteral("taskbar/plasmoids/org.devl0rd.taskbar"), QStringLiteral("org.devl0rd.taskbar"),
    QStringLiteral("preferences-system-windows-effect"), {}};

struct Control
{
    QString page;
    QString action;
    QString key;
    QVariant value;
};

}

Q_DECLARE_METATYPE(Control)

class TestPlasmoidTaskbarConfig : public QObject
{
    Q_OBJECT

    QObject *open(PlasmoidHarness &harness, const QString &file)
    {
        harness.setUp(Form::Planar);
        return harness.create(QStringLiteral("contents/ui/") + file, configPageProperties(harness, QString()));
    }

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void controlsWriteTheirSettings_data()
    {
        QTest::addColumn<Control>("control");
        const auto row = [](const char *name, const char *page, const char *action, const char *key, const QVariant &value) {
            QTest::newRow(name) << Control {QLatin1String(page), QLatin1String(action), QLatin1String(key), value};
        };
        const char *appearance = "configAppearance.qml";
        row("auto icon size", appearance, "autoIconSize.toggle(); autoIconSize.toggled()", "autoIconSize", false);
        row("icon size", appearance, "iconSize.value = 48; iconSize.valueModified()", "iconSize", 48);
        row("spacing", appearance, "iconSpacing.value = 6; iconSpacing.valueModified()", "iconSpacing", 6);
        row("padding", appearance, "buttonPadding.value = 0; buttonPadding.valueModified()", "buttonPadding", 0);
        row("highlight", appearance, "highlightStyle.currentIndex = 1; highlightStyle.activated(1)", "highlightStyle", 1);
        row("indicators", appearance, "indicatorStyle.currentIndex = 2; indicatorStyle.activated(2)", "indicatorStyle", 2);
        row("indicator edge", appearance, "indicatorEdge.currentIndex = 1; indicatorEdge.activated(1)", "indicatorEdge", 1);
        row("separator", appearance, "showSeparator.toggle(); showSeparator.toggled()", "showSeparator", false);
        row("animations", appearance, "animations.toggle(); animations.toggled()", "animations", false);
        row("attention pulse", appearance, "attentionPulse.toggle(); attentionPulse.toggled()", "attentionPulse", false);
        const char *behavior = "configBehavior.qml";
        row("grouping", behavior, "groupMode.currentIndex = 2; groupMode.activated(2)", "groupMode", 2);
        row("active click", behavior, "activeClick.currentIndex = 2; activeClick.activated(2)", "activeClick", 2);
        row("middle click", behavior, "middleClick.currentIndex = 1; middleClick.activated(1)", "middleClick", 1);
        row("wheel over tasks", behavior, "wheelCyclesTasks.toggle(); wheelCyclesTasks.toggled()", "wheelCyclesTasks", true);
        row("every screen", behavior, "onlyThisScreen.currentIndex = 0; onlyThisScreen.activated(0)", "onlyThisScreen", false);
        row("floating", behavior, "showFloating.toggle(); showFloating.toggled()", "showFloating", false);
        row("tooltips", behavior, "showTooltips.toggle(); showTooltips.toggled()", "showTooltips", false);
        row("badges", behavior, "showShortcutBadges.toggle(); showShortcutBadges.toggled()", "showShortcutBadges", false);
        const char *workspaces = "configWorkspaces.qml";
        row("strip", workspaces, "showWorkspaces.toggle(); showWorkspaces.toggled()", "showWorkspaces", false);
        row("strip after", workspaces, "workspacesAfterTasks.currentIndex = 1; workspacesAfterTasks.activated(1)", "workspacesAfterTasks",
            true);
        row("pill content", workspaces, "pillContent.currentIndex = 2; pillContent.activated(2)", "pillContent", 2);
        row("empty workspaces", workspaces, "showEmptyWorkspaces.toggle(); showEmptyWorkspaces.toggled()", "showEmptyWorkspaces", false);
        row("strip wheel", workspaces, "wheelSwitchesWorkspaces.toggle(); wheelSwitchesWorkspaces.toggled()", "wheelSwitchesWorkspaces",
            false);
        row("pin order", "configPins.qml", "placePinnedLaunches.toggle(); placePinnedLaunches.toggled()", "placePinnedLaunches", false);
    }

    void controlsWriteTheirSettings()
    {
        QFETCH(Control, control);
        PlasmoidHarness harness(taskbar);
        QObject *page = open(harness, control.page);
        QVERIFY2(page, qPrintable(harness.error));
        harness.eval(control.action, page);
        QCOMPARE(page->property(qPrintable(QStringLiteral("cfg_") + control.key)), control.value);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void defaultsReachTheControls()
    {
        PlasmoidHarness harness(taskbar);
        QObject *page = open(harness, QStringLiteral("configAppearance.qml"));
        QVERIFY2(page, qPrintable(harness.error));
        harness.eval(QStringLiteral("animations.toggle(); animations.toggled(); iconSpacing.value = 9; iconSpacing.valueModified()"), page);
        page->setProperty("cfg_animations", true);
        page->setProperty("cfg_iconSpacing", 2);
        QCOMPARE(harness.eval(QStringLiteral("[animations.checked, attentionPulse.enabled, iconSpacing.value]"), page).toList(),
            (QVariantList {true, true, 2}));
        page->setProperty("cfg_autoIconSize", false);
        QCOMPARE(harness.eval(QStringLiteral("iconSize.enabled"), page).toBool(), true);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void pinsCanBeAddedReorderedAndRemoved()
    {
        PlasmoidHarness harness(taskbar);
        QObject *page = open(harness, QStringLiteral("configPins.qml"));
        QVERIFY2(page, qPrintable(harness.error));
        const auto pins = [&] { return page->property("cfg_launchers").toStringList(); };
        harness.eval(QStringLiteral("desktopId.text = ' firefox '; addButton.clicked()"), page);
        harness.eval(QStringLiteral("desktopId.text = 'org.kde.konsole.desktop'; desktopId.accepted()"), page);
        QCOMPARE(
            pins(), (QStringList {QStringLiteral("applications:firefox.desktop"), QStringLiteral("applications:org.kde.konsole.desktop")}));
        QCOMPARE(harness.eval(QStringLiteral("desktopId.text"), page).toString(), QString());
        harness.eval(QStringLiteral("search.item.children[0].text = 'dolph'"), page);
        QTRY_VERIFY(harness.eval(QStringLiteral("search.item.children[1].count"), page).toInt() > 0);
        harness.eval(QStringLiteral("const results = search.item.children[1]; results.width = 300; results.height = 200; "
                                    "results.forceLayout(); results.itemAtIndex(0).clicked()"),
            page);
        QCOMPARE(pins().last(), QStringLiteral("applications:org.kde.dolphin.desktop"));
        const QString button = QStringLiteral("rows.itemAt(%1).children.find(child => child.text === '%2').clicked()");
        harness.eval(button.arg(2).arg(QStringLiteral("Move up")), page);
        QCOMPARE(pins().at(1), QStringLiteral("applications:org.kde.dolphin.desktop"));
        harness.eval(button.arg(0).arg(QStringLiteral("Move down")), page);
        QCOMPARE(pins().first(), QStringLiteral("applications:org.kde.dolphin.desktop"));
        harness.eval(button.arg(0).arg(QStringLiteral("Unpin")), page);
        QCOMPARE(
            pins(), (QStringList {QStringLiteral("applications:firefox.desktop"), QStringLiteral("applications:org.kde.konsole.desktop")}));
        harness.eval(QStringLiteral("desktopId.text = 'firefox'; addButton.clicked()"), page);
        QCOMPARE(pins().size(), 2);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidTaskbarConfig)

#include "test_plasmoid_taskbar_config.moc"
