#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

const QString kText = QStringLiteral(R"(
    binds {
        Mod+Q hotkey-overlay-title="Quit" { close-window; }
    }
    border-style "Pulse" {
        layer "glow" {
            drive "width" "audio.bass" amount=18
        }
    }
    monitor-profile "docked" {
        match name="^DP-1$"
        layout { gaps 8; }
    }
    window-rule {
        border { active-gradient from="#000000" to="#ffffff"; }
    }
    output "eDP-1" {
        layout {
            gaps 4
            preset-column-widths { proportion 0.5; fixed 960; }
            tab-indicator { width 5; }
            insert-hint { color "#00ff00"; }
        }
    }
    workspace "chat" {
        open-on-output "eDP-1"
        layout { gaps 6; }
    }
    layout {
        event-effects {
            on "click" "none" reach="neighbors" duration=1.5
        }
    }
)");

}

class TestConfigEquality : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identicalTextLoadsToEqualSections();
    void differentTextLoadsToDifferentSections();
};

void TestConfigEquality::identicalTextLoadsToEqualSections()
{
    const Config first = parsed(kText);
    const Config second = parsed(kText);
    QVERIFY(first.outputs == second.outputs);
    QVERIFY(first.workspaces == second.workspaces);
    QVERIFY(first.binds == second.binds);
    QVERIFY(first.borderStyles == second.borderStyles);
    QVERIFY(first.monitorProfiles == second.monitorProfiles);
    QVERIFY(first.windowRules == second.windowRules);
    QVERIFY(first.layout.eventEffects == second.layout.eventEffects);
    QVERIFY(first.layout.eventEffects.of(EventTrigger::Click) == second.layout.eventEffects.of(EventTrigger::Click));
}

void TestConfigEquality::differentTextLoadsToDifferentSections()
{
    QString changed = kText;
    changed.replace(QStringLiteral("gaps 4"), QStringLiteral("gaps 5")).replace(QStringLiteral("gaps 6"), QStringLiteral("gaps 7"));
    changed.replace(QStringLiteral("duration=1.5"), QStringLiteral("duration=2"));
    changed.replace(QStringLiteral("amount=18"), QStringLiteral("amount=19"));
    changed.replace(QStringLiteral("close-window"), QStringLiteral("fullscreen-window"));
    changed.replace(QStringLiteral("gaps 8"), QStringLiteral("gaps 9"));
    changed.replace(QStringLiteral("#ffffff"), QStringLiteral("#fffffe"));
    const Config first = parsed(kText);
    const Config second = parsed(changed);
    QVERIFY(first.outputs != second.outputs);
    QVERIFY(first.workspaces != second.workspaces);
    QVERIFY(first.layout.eventEffects != second.layout.eventEffects);
    QVERIFY(first.binds != second.binds);
    QVERIFY(first.borderStyles != second.borderStyles);
    QVERIFY(first.monitorProfiles != second.monitorProfiles);
    QVERIFY(first.windowRules != second.windowRules);
}

QTEST_GUILESS_MAIN(TestConfigEquality)
#include "test_config_equality.moc"
