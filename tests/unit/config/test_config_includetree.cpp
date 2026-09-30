#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigIncludeTree : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void reportsIndirectCycles();
    void reportsCyclesBackToTheMainFile();
    void expandsHomeDirectory();
    void includeOptionTypes();
    void mergesInputAcrossFiles();
    void mergesAnimationsAcrossFiles();
    void mergesGesturesAcrossFiles();
    void mergesExperimentsAndFlagsAcrossFiles();
    void mergesDecorationsAcrossFiles();
    void rejectsDuplicateSectionsInsideOneInclude();
    void rejectsDuplicateNamesAcrossFiles();
    void scopedLayoutsSeeLaterIncludes();
    void modKeyFromAnIncludeResolvesEveryBind();

private:
    void writeMain(const QString &text) const;
    void writeExtra(const QString &text) const;
    Config loadBoth(const QString &main, const QString &extra) const;

    ConfigDir m_dir;
    QByteArray m_home;
};

void TestConfigIncludeTree::init()
{
    m_dir.clear();
    m_home = qgetenv("HOME");
}

void TestConfigIncludeTree::cleanup()
{
    qputenv("HOME", m_home);
}

void TestConfigIncludeTree::writeMain(const QString &text) const
{
    m_dir.write(QStringLiteral("config.kdl"), text);
}

void TestConfigIncludeTree::writeExtra(const QString &text) const
{
    m_dir.write(QStringLiteral("extra.kdl"), text);
}

Config TestConfigIncludeTree::loadBoth(const QString &main, const QString &extra) const
{
    writeMain(main + QStringLiteral("\ninclude \"extra.kdl\"\n"));
    writeExtra(extra);
    return m_dir.load().config;
}

void TestConfigIncludeTree::reportsIndirectCycles()
{
    writeMain(QStringLiteral("include \"a.kdl\"\n"));
    m_dir.write(QStringLiteral("a.kdl"), QStringLiteral("layout { gaps 1; }\ninclude \"b.kdl\"\n"));
    m_dir.write(QStringLiteral("b.kdl"), QStringLiteral("\n  include \"a.kdl\"\n"));
    const LoadError error = m_dir.failure();
    QCOMPARE(error.message, QStringLiteral("recursive include (file includes itself)"));
    QCOMPARE(error.location.file, QStringLiteral("b.kdl"));
    QCOMPARE(error.location.line, 2);
    QCOMPARE(error.location.column, 3);
    QCOMPARE(error.sourceLine, QStringLiteral("  include \"a.kdl\""));
    QCOMPARE(error.files.size(), 3);
}

void TestConfigIncludeTree::reportsCyclesBackToTheMainFile()
{
    writeMain(QStringLiteral("include \"nested/a.kdl\"\n"));
    m_dir.write(QStringLiteral("nested/a.kdl"), QStringLiteral("include \"../config.kdl\"\n"));
    const LoadError error = m_dir.failure();
    QCOMPARE(error.message, QStringLiteral("recursive include (file includes itself)"));
    QCOMPARE(error.location.file, QStringLiteral("nested/a.kdl"));
}

void TestConfigIncludeTree::expandsHomeDirectory()
{
    qputenv("HOME", QFile::encodeName(m_dir.path(QStringLiteral("home"))));
    m_dir.write(QStringLiteral("home/.config/extra.kdl"), QStringLiteral("layout { gaps 11; }\n"));
    writeMain(QStringLiteral("include \"~/.config/extra.kdl\"\n"));
    const LoadResult result = m_dir.load();
    QCOMPARE(result.config.layout.gaps, 11.0);
    QCOMPARE(result.files.at(1), m_dir.path(QStringLiteral("home/.config/extra.kdl")));

    writeMain(QStringLiteral("include optional=true \"~/absent.kdl\"\n"));
    QCOMPARE(m_dir.load().warnings,
        QStringList {QStringLiteral("optional include not found: ") + m_dir.path(QStringLiteral("home/absent.kdl"))});

    writeMain(QStringLiteral("include \"~other/extra.kdl\"\n"));
    QVERIFY(
        m_dir.failure().message.startsWith(QStringLiteral("failed to read included config from ") + m_dir.path(QStringLiteral("~other"))));
}

void TestConfigIncludeTree::includeOptionTypes()
{
    writeMain(QStringLiteral("include optional=false \"absent.kdl\"\n"));
    QVERIFY(m_dir.failure().message.startsWith(QStringLiteral("failed to read included config from ")));
    writeMain(QStringLiteral("include optional=\"yes\" \"absent.kdl\"\n"));
    QCOMPARE(m_dir.failure().message, QStringLiteral("expected a boolean"));
    writeMain(QStringLiteral("include 5\n"));
    QCOMPARE(m_dir.failure().message, QStringLiteral("expected a string"));
    m_dir.write(QStringLiteral("dir/keep.kdl"), QString());
    writeMain(QStringLiteral("include optional=true \"dir\"\n"));
    QVERIFY(m_dir.failure().message.startsWith(QStringLiteral("failed to read included config from ")));
}

void TestConfigIncludeTree::mergesInputAcrossFiles()
{
    const Config config = loadBoth(QStringLiteral("input { focus-follows-mouse; mod-key \"Alt\"; }"),
        QStringLiteral("input { warp-mouse-to-focus; mod-key \"Ctrl\"; }"));
    QCOMPARE(config.input.focusFollowsMouse, true);
    QCOMPARE(config.input.warpMouseToFocus, true);
    QCOMPARE(config.input.modKey, QStringLiteral("Ctrl"));
}

void TestConfigIncludeTree::mergesAnimationsAcrossFiles()
{
    const Config config = loadBoth(QStringLiteral(R"(
        animations {
            off
            slowdown 3
            window-open { spring damping-ratio=0.5 stiffness=100 epsilon=0.01; }
            window-resize { curve "linear"; duration-ms 70; }
        }
    )"),
        QStringLiteral("animations {\n    window-resize { duration-ms 40; }\n    workspace-switch { off; }\n}\n"));
    QCOMPARE(config.animations.enabled, false);
    QCOMPARE(config.animations.slowdown, 3.0);
    QCOMPARE(std::get<SpringParams>(config.animations.windowOpen.kind), (SpringParams {0.5, 100, 0.01}));
    const auto resize = std::get<EasingParams>(config.animations.windowResize.kind);
    QCOMPARE(resize.durationMs, 40.0);
    QCOMPARE(resize.curve, EasingCurve::EaseOutCubic);
    QCOMPARE(config.animations.workspaceSwitch.enabled, false);
    QCOMPARE(loadBoth(QStringLiteral("animations { off; }"), QStringLiteral("animations { on; }")).animations.enabled, true);
}

void TestConfigIncludeTree::mergesGesturesAcrossFiles()
{
    const Config config = loadBoth(QStringLiteral(R"(
        gestures {
            touchpad { swipe-fingers 4; }
            hot-corners { top-right; bottom-right; }
            dnd-edge-view-scroll { trigger-width 10; }
            titlebar-drag "move-window"
        }
    )"),
        QStringLiteral(R"(
        gestures {
            touchpad { natural-swipe false; }
            hot-corners { bottom-left; }
            dnd-edge-view-scroll { delay-ms 5; }
        }
    )"));
    QCOMPARE(config.gestures.touchpad.swipeFingers, 4);
    QCOMPARE(config.gestures.touchpad.naturalSwipe, false);
    QCOMPARE(config.gestures.hotCorners, (HotCorners {true, false, false, true, false}));
    QCOMPARE(config.gestures.dndEdgeViewScroll, (DndEdgeScroll {10, 5, 1500}));
    QCOMPARE(config.gestures.titlebarDrag, TitlebarDrag::MoveWindow);
}

void TestConfigIncludeTree::mergesExperimentsAndFlagsAcrossFiles()
{
    const Config config = loadBoth(
        QStringLiteral("experiments { prevent-fullscreen-minimize; }\nhide-desktop-widgets\nconfig-notification { disable-failed; }\n"
                       "disable-minimize\n"),
        QStringLiteral(
            "experiments { prevent-fullscreen-exit; }\nhide-desktop-widgets false\nconfig-notification { disable-failed false; }\n"
            "fill-panels-on-maximize\n"));
    QCOMPARE(config.experiments, (Experiments {true, true}));
    QCOMPARE(config.hideDesktopWidgets, false);
    QCOMPARE(config.configNotificationDisableFailed, false);
    QCOMPARE(config.disableMinimize, true);
    QCOMPARE(config.fillPanelsOnMaximize, true);
}

void TestConfigIncludeTree::mergesDecorationsAcrossFiles()
{
    const Config config = loadBoth(QStringLiteral("layout { focus-ring { width 9; }; tab-indicator { width 6; }; insert-hint { off; }; }"),
        QStringLiteral("layout { focus-ring { active-color \"#ff0000\"; }; tab-indicator { position \"right\"; }; insert-hint { color "
                       "\"#00f\"; }; }"));
    QCOMPARE(config.layout.focusRing.width, 9.0);
    QCOMPARE(config.layout.focusRing.active.color, QColor(255, 0, 0));
    QCOMPARE(config.layout.tabIndicator.width, 6.0);
    QCOMPARE(config.layout.tabIndicator.position, TabIndicatorPosition::Right);
    QCOMPARE(config.layout.insertHint.enabled, false);
    QCOMPARE(config.layout.insertHint.paint.color, QColor(0, 0, 255));
}

void TestConfigIncludeTree::rejectsDuplicateSectionsInsideOneInclude()
{
    const QStringList sections {QStringLiteral("layout {}"), QStringLiteral("input {}"), QStringLiteral("animations {}"),
        QStringLiteral("gestures {}"), QStringLiteral("binds {}"), QStringLiteral("experiments {}"),
        QStringLiteral("config-notification {}"), QStringLiteral("hide-desktop-widgets"), QStringLiteral("fill-panels-on-maximize"),
        QStringLiteral("disable-minimize")};
    for (const QString &section : sections) {
        writeMain(section + QStringLiteral("\ninclude \"extra.kdl\"\n"));
        writeExtra(section + QStringLiteral("\n") + section + QStringLiteral("\n"));
        const LoadError error = m_dir.failure();
        QCOMPARE(error.message, QStringLiteral("duplicate node `%1`, single node expected").arg(section.section(QLatin1Char(' '), 0, 0)));
        QCOMPARE(error.location.file, QStringLiteral("extra.kdl"));
        QCOMPARE(error.location.line, 2);
    }
}

void TestConfigIncludeTree::rejectsDuplicateNamesAcrossFiles()
{
    writeMain(QStringLiteral("output \"DP-1\"\ninclude \"extra.kdl\"\n"));
    writeExtra(QStringLiteral("output \"dp-1\" { layout { gaps 1; }; }\n"));
    QCOMPARE(m_dir.failure().location.file, QStringLiteral("extra.kdl"));
    writeExtra(QStringLiteral("monitor-profile \"a\" {}\nmonitor-profile \"a\" {}\n"));
    QCOMPARE(m_dir.failure().message, QStringLiteral("duplicate monitor profile: a"));
    writeExtra(QStringLiteral("workspace \"w\"\n"));
    writeMain(QStringLiteral("workspace \"W\"\ninclude \"extra.kdl\"\n"));
    QCOMPARE(m_dir.failure().message, QStringLiteral("duplicate named workspace: w"));
}

void TestConfigIncludeTree::scopedLayoutsSeeLaterIncludes()
{
    const Config config = loadBoth(QStringLiteral("output \"DP-1\" { layout { gaps 2; }; }\nworkspace \"w\" { layout { gaps 3; }; }"),
        QStringLiteral("layout { center-focused-column \"always\"; gaps 30; }"));
    const Layout output = mergedLayout(config.layout, *config.outputs.first().layout);
    QCOMPARE(output.gaps, 2.0);
    QCOMPARE(output.centerFocusedColumn, CenterFocusedColumn::Always);
    QCOMPARE(mergedLayout(config.layout, *config.workspaces.first().layout).centerFocusedColumn, CenterFocusedColumn::Always);
    QCOMPARE(config.layout.gaps, 30.0);
}

void TestConfigIncludeTree::modKeyFromAnIncludeResolvesEveryBind()
{
    const Config config = loadBoth(QStringLiteral("binds { Mod+A { close-window; }; }"), QStringLiteral("input { mod-key \"Alt\"; }"));
    QCOMPARE(config.binds.first().resolvedModifiers, BindModifiers(BindModifier::Alt));
    writeMain(QStringLiteral("binds { Mod+A { close-window; }; }\ninclude \"extra.kdl\"\n"));
    writeExtra(QStringLiteral("input { mod-key \"Alt\"; }\nbinds {\n    Alt+A { toggle-overview; }\n}\n"));
    const LoadError error = m_dir.failure();
    QCOMPARE(error.message, QStringLiteral("keybind `Alt+A` is the same as `Mod+A` while the Mod key is Alt"));
    QCOMPARE(error.location.file, QStringLiteral("extra.kdl"));
    QCOMPARE(error.location.line, 3);
}

QTEST_MAIN(TestConfigIncludeTree)
#include "test_config_includetree.moc"
