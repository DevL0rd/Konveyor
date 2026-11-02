#include "document/borderdefaults.h"
#include "document/configmigration.h"

#include "config/loader.h"

#include <QFile>
#include <QTest>

using namespace Konveyor::Settings;

namespace
{

const QString defaults = QStringLiteral("binds {\n    Mod+1 { focus-workspace 1; }\n    Super+Alt+1 { focus-column 1; }\n"
                                        "    Super+Alt+2 { focus-column 2; }\n}\n");
const QStringList keys {QStringLiteral("Super+Alt+1"), QStringLiteral("Super+Alt+2"), QStringLiteral("Super+Alt+3")};
const QString fileName = QStringLiteral("/tmp/konveyor-migration/config.kdl");

QString migrated(const QString &text)
{
    const auto result = addDefaultBinds(text, fileName, defaults, keys);
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

QString shippedDefaults()
{
    QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

QString ring(const QString &lines)
{
    return QStringLiteral("layout {\n    gaps 8\n    // my ring\n    focus-ring {\n%1    }\n}\n// my styles\n").arg(lines);
}

const QString oldRing = QStringLiteral("        width 4\n        // follows the accent\n        active-color \"accent\"\n"
                                       "        inactive-color \"#505050\"\n");

QString focusEffects(const QString &text)
{
    const auto result = migrateFocusEffects(text, fileName, shippedDefaults());
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

QString borders(const QString &text)
{
    const auto result = migrateBorderDefaults(text, fileName, shippedDefaults());
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

}

class TestSettingsMigration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void addsTheNewDefaultBinds()
    {
        const QString text = QStringLiteral("// my binds\nbinds {\n    Mod+T { spawn \"kitty\"; } // terminal\n}\n");
        QCOMPARE(migrated(text),
            QStringLiteral("// my binds\nbinds {\n    Mod+T { spawn \"kitty\"; } // terminal\n    Super+Alt+1 { focus-column 1; }\n"
                           "    Super+Alt+2 { focus-column 2; }\n}\n"));
    }

    void keepsKeysTheUserAlreadyBound()
    {
        const QString text = QStringLiteral("binds {\n    Alt+Mod+1 { spawn \"mine\"; }\n}\n");
        QCOMPARE(migrated(text), QStringLiteral("binds {\n    Alt+Mod+1 { spawn \"mine\"; }\n    Super+Alt+2 { focus-column 2; }\n}\n"));
    }

    void addsBindsThatStayDistinctUnderAnotherModKey()
    {
        const QString text = QStringLiteral("input {\n    mod-key \"Alt\";\n}\nbinds {\n    Mod+1 { spawn \"mine\"; }\n}\n");
        QCOMPARE(migrated(text),
            QStringLiteral("input {\n    mod-key \"Alt\";\n}\nbinds {\n    Mod+1 { spawn \"mine\"; }\n    Super+Alt+1 { focus-column 1; }\n"
                           "    Super+Alt+2 { focus-column 2; }\n}\n"));
    }

    void knowsWhetherTheDefaultsShipTheBinds()
    {
        const QStringList paths {QStringLiteral("binds/Super+Alt+1"), QStringLiteral("binds/Super+Alt+3")};
        QVERIFY(shipsAny(defaults, paths));
        QVERIFY(!shipsAny(QStringLiteral("binds {\n    Mod+1 { focus-workspace 1; }\n}\n"), paths));
        QVERIFY(!shipsAny(QString(), paths));
    }

    void replacesOnlyTheOldColumnBindsWithTaskbarItemBinds()
    {
        const QString shipped
            = QStringLiteral("binds {\n    Super+Alt+1 { focus-taskbar-item 1; }\n    Super+Alt+2 { focus-taskbar-item 2; }\n"
                             "    Super+Alt+3 { focus-taskbar-item 3; }\n}\n");
        const QString mine = QStringLiteral("binds {\n    Super+Alt+1 { focus-column 1; }\n    Super+Alt+2 { focus-column 5; }\n"
                                            "    Super+Alt+3 { spawn \"mine\"; }\n}\n");
        const auto result = replaceBindAction(mine, fileName, shipped, keys, QStringLiteral("focus-column"));
        QVERIFY2(result, result ? "" : qPrintable(result.error()));
        QCOMPARE(*result,
            QStringLiteral("binds {\n    Super+Alt+1 { focus-taskbar-item 1; }\n    Super+Alt+2 { focus-column 5; }\n"
                           "    Super+Alt+3 { spawn \"mine\"; }\n}\n"));
        const auto again = replaceBindAction(*result, fileName, shipped, keys, QStringLiteral("focus-column"));
        QVERIFY(again);
        QCOMPARE(*again, *result);
    }

    void isIdempotent()
    {
        const QString once = migrated(QStringLiteral("binds {\n}\n"));
        QCOMPARE(migrated(once), once);
    }

    void migratesTheFullscreenShortcut_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("expected");
        QTest::newRow("old-default") << QStringLiteral(
            "binds {\n    Mod+F { cycle-window-expansion; }\n    Mod+Shift+F { fullscreen-window; }\n}\n")
                                     << QStringLiteral(
                                            "binds {\n    Mod+F { fullscreen-window; }\n    Mod+Shift+F { fullscreen-window; }\n}\n");
        QTest::newRow("custom-mod-key") << QStringLiteral("input { mod-key \"Alt\"; }\nbinds {\n    Mod+F { cycle-window-expansion; }\n}\n")
                                        << QStringLiteral("input { mod-key \"Alt\"; }\nbinds {\n    Mod+F { fullscreen-window; }\n}\n");
        for (const auto &[name, text] : QList<QPair<const char *, QString>> {
                 {"custom-action", QStringLiteral("binds {\n    Mod+F { spawn \"mine\"; }\n}\n")},
                 {"custom-repeat", QStringLiteral("binds {\n    Mod+F repeat=false { cycle-window-expansion; }\n}\n")},
                 {"custom-cooldown", QStringLiteral("binds {\n    Mod+F cooldown-ms=150 { cycle-window-expansion; }\n}\n")},
                 {"custom-title", QStringLiteral("binds {\n    Mod+F hotkey-overlay-title=\"Expand\" { cycle-window-expansion; }\n}\n")},
                 {"already-fullscreen", QStringLiteral("binds {\n    Mod+F { fullscreen-window; }\n}\n")},
                 {"unbound", QStringLiteral("binds {\n    Mod+Shift+F { fullscreen-window; }\n}\n")},
                 {"no-binds", QStringLiteral("layout {\n    gaps 8;\n}\n")},
             }) {
            QTest::newRow(name) << text << text;
        }
    }

    void migratesTheFullscreenShortcut()
    {
        QFETCH(QString, text);
        QFETCH(QString, expected);
        QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString shipped = QString::fromUtf8(file.readAll());
        ConfigMigration fullscreenMigration;
        for (const ConfigMigration &migration : configMigrations()) {
            if (migration.id == QStringLiteral("fullscreen-window-bind")) {
                fullscreenMigration = migration;
                break;
            }
        }
        QCOMPARE(fullscreenMigration.shippedPaths, QStringList {QStringLiteral("binds/Mod+F")});
        const auto result = applyMigration(fullscreenMigration, text, fileName, shipped);
        QVERIFY2(result, result ? "" : qPrintable(result.error()));
        QCOMPARE(*result, expected);
        const auto again = applyMigration(fullscreenMigration, *result, fileName, shipped);
        QVERIFY2(again, again ? "" : qPrintable(again.error()));
        QCOMPARE(*again, expected);
    }

    void animatedBorderDefaultsReachAnUntouchedDefaultRing()
    {
        const QString result = borders(ring(oldRing));
        QVERIFY(result.contains(QStringLiteral("// my ring")));
        QVERIFY(result.contains(QStringLiteral("// follows the accent")));
        QVERIFY(result.contains(QStringLiteral("// my styles")));
        const auto loaded = Konveyor::Config::loadString(result, fileName);
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->config.layout.focusRing.width, 2.0);
        QCOMPARE(loaded->config.layout.focusRing.activeStyle, QStringLiteral("Default-Style"));
        QCOMPARE(loaded->config.layout.focusRing.urgentStyle, QStringLiteral("Default-Attention-Style"));
        QCOMPARE(loaded->config.layout.focusRing.inactiveStyle, QStringLiteral("Default-Style-Inactive"));
        QCOMPARE(loaded->config.borderStyles.size(), 3);
        QCOMPARE(result.count(QStringLiteral("border-style \"Default-Style\"")), 1);
        QCOMPARE(result.count(QStringLiteral("border-style \"Default-Attention-Style\"")), 1);
    }

    void animatedBorderDefaultsKeepTheUsersOwnChoices_data()
    {
        QTest::addColumn<QString>("lines");
        QTest::newRow("width") << QStringLiteral("        width 6\n        active-color \"accent\"\n        active-style \"mine\"\n"
                                                 "        urgent-style \"mine\"\n");
        QTest::newRow("colors") << QStringLiteral("        width 2\n        active-color \"#112233\"\n        urgent-color \"#00ff00\"\n");
        QTest::newRow("gradient") << QStringLiteral("        width 2\n        active-gradient from=\"#000000\" to=\"#ffffff\" angle=45\n"
                                                    "        urgent-gradient from=\"#000000\" to=\"#ffffff\" angle=45\n");
        QTest::newRow("styles") << QStringLiteral("        width 2\n        active-style \"mine\"\n        urgent-style \"other\"\n");
        QTest::newRow("no-ring") << QString();
    }

    void animatedBorderDefaultsKeepTheUsersOwnChoices()
    {
        QFETCH(QString, lines);
        const QString text = lines.isEmpty() ? QStringLiteral("layout {\n    gaps 8\n}\n") : ring(lines);
        QCOMPARE(borders(text), text);
    }

    void animatedBorderDefaultsDropTheOldShippedInactiveColor()
    {
        const QString result = borders(ring(oldRing));
        QVERIFY(!result.contains(QStringLiteral("inactive-color")));
        const auto loaded = Konveyor::Config::loadString(result, fileName);
        QVERIFY(loaded.has_value());
        QVERIFY(!loaded->config.layout.focusRing.inactive.isSet());
    }

    void animatedBorderDefaultsDropTheInactiveColorFromAnyRing()
    {
        const QString lines = QStringLiteral("        width 2\n        active-style \"mine\"\n        urgent-style \"mine\"\n");
        QCOMPARE(borders(ring(lines + QStringLiteral("        inactive-color \"#505050\"\n"))), ring(lines));
    }

    void animatedBorderDefaultsSurviveARingWithOnlyTheOldInactiveColor()
    {
        const QString result = borders(ring(QStringLiteral("        inactive-color \"#505050\"\n")));
        QVERIFY(!result.contains(QStringLiteral("inactive-color")));
        QVERIFY(Konveyor::Config::loadString(result, fileName).has_value());
    }

    void animatedBorderDefaultsKeepAnInactiveColorTheUserChose()
    {
        const QString own = QStringLiteral("        inactive-color \"#505051\"\n");
        const QString result = borders(ring(QStringLiteral("        width 2\n") + own));
        QVERIFY(result.contains(own));
        const QString borderOnly = QStringLiteral("layout {\n    border {\n        inactive-color \"#505050\"\n    }\n}\n");
        QCOMPARE(borders(borderOnly), borderOnly);
    }

    void animatedBorderDefaultsAddOnlyTheMissingState()
    {
        const QString result = borders(ring(oldRing + QStringLiteral("        active-style \"mine\"\n")));
        QVERIFY(result.contains(QStringLiteral("active-style \"mine\"")));
        QVERIFY(!result.contains(QStringLiteral("active-style \"Default-Style\"")));
        QVERIFY(result.contains(QStringLiteral("urgent-style \"Default-Attention-Style\"")));
        QVERIFY(!result.contains(QStringLiteral("border-style \"Default-Style\"")));
        QVERIFY(result.contains(QStringLiteral("border-style \"Default-Attention-Style\"")));
        QVERIFY(result.contains(QStringLiteral("width 2")));
    }

    void animatedBorderDefaultsReplaceTheOldShippedUrgentColor()
    {
        const QString result = borders(ring(oldRing + QStringLiteral("        urgent-color \"#9B0000\"\n")));
        QVERIFY(result.contains(QStringLiteral("urgent-style \"Default-Attention-Style\"")));
        QVERIFY(result.contains(QStringLiteral("urgent-color \"#9B0000\"")));
    }

    void animatedBorderDefaultsKeepAStyleThatAlreadyHasTheName()
    {
        const QString own
            = QStringLiteral("border-style \"Default-Style\" {\n    layer \"pulse\" {\n        colors \"#123456\"\n    }\n}\n");
        const QString result = borders(ring(oldRing) + own);
        QCOMPARE(result.count(QStringLiteral("border-style \"Default-Style\"")), 1);
        QVERIFY(result.contains(own));
        QVERIFY(!result.contains(QStringLiteral("active-style \"Default-Style\"")));
        QVERIFY(result.contains(QStringLiteral("urgent-style \"Default-Attention-Style\"")));
    }

    void animatedBorderDefaultsLeaveTheShippedConfigAlone()
    {
        const QString shipped = shippedDefaults();
        QCOMPARE(borders(shipped), shipped);
    }

    void animatedBorderDefaultsAreIdempotent()
    {
        const QString once = borders(ring(oldRing));
        QVERIFY(once != ring(oldRing));
        QCOMPARE(borders(once), once);
    }

    void animatedBorderDefaultsKeepAFocusRingThatIsOff()
    {
        const QString result = borders(ring(QStringLiteral("        off\n") + oldRing));
        QVERIFY(result.contains(QStringLiteral("        off\n")));
        QVERIFY(result.contains(QStringLiteral("width 2")));
        QVERIFY(result.contains(QStringLiteral("active-style \"Default-Style\"")));
        const auto loaded = Konveyor::Config::loadString(result, fileName);
        QVERIFY(loaded.has_value());
        QVERIFY(!loaded->config.layout.focusRing.enabled);
    }

    void animatedBorderDefaultsRefuseABrokenConfig()
    {
        QVERIFY(borders(QStringLiteral("layout {\n")).startsWith(QStringLiteral("<error: ")));
    }

    void focusEffectsFireARippleOnFocusInPlace()
    {
        const QString text = QStringLiteral(
            "layout {\n    focus-ring {\n        active-style \"Mine\"\n    }\n}\nborder-style \"Mine\" {\n    layer \"glow\" {\n        "
            "speed 1\n    }\n"
            "    layer \"ripple\" {\n        colors \"accent\"\n        opacity 0.8\n        blend \"add\"\n        speed 1\n    }\n}\n");
        const QString result = focusEffects(text);
        const auto loaded = Konveyor::Config::loadString(result, fileName);
        QVERIFY2(loaded.has_value(), qPrintable(result));
        const auto &styles = loaded->config.borderStyles;
        QCOMPARE(styles.size(), 1);
        QCOMPARE(styles[0].layers.size(), 2);
        QVERIFY(styles[0].layers[0].fireOn.isEmpty());
        QCOMPARE(styles[0].layers[1].kind, Konveyor::Config::BorderLayerKind::Ripple);
        QCOMPARE(styles[0].layers[1].opacity, 0.8);
        QCOMPARE(styles[0].layers[1].fireOn, QList<Konveyor::Config::Signal> {Konveyor::Config::Signal::Focus});
        QVERIFY(!loaded->config.layout.eventEffects.of(Konveyor::Config::EventTrigger::Focus).has_value());
        QCOMPARE(focusEffects(result), result);
    }

    void focusEffectsKeepAFiringRippleAndARippleElsewhere()
    {
        const QString firing = QStringLiteral("layout {\n    focus-ring {\n        active-style \"Mine\"\n    }\n}\n"
                                              "border-style \"Mine\" {\n    layer \"ripple\" {\n        fire-on \"click\"\n    }\n}\n");
        QCOMPARE(focusEffects(firing), firing);
        const QString elsewhere = QStringLiteral("layout {\n    focus-ring {\n        active-style \"Mine\"\n    }\n}\nborder-style "
                                                 "\"Mine\" {\n    layer \"glow\" {\n        speed 1\n    }\n}\n"
                                                 "border-style \"Other\" {\n    layer \"ripple\" {\n        speed 1\n    }\n}\n");
        QCOMPARE(focusEffects(elsewhere), elsewhere);
    }

    void focusEffectsLeaveTheShippedConfigAlone() { QCOMPARE(focusEffects(shippedDefaults()), shippedDefaults()); }

    void leavesAConfigWithoutBindsAlone()
    {
        const QString text = QStringLiteral("layout {\n    gaps 8;\n}\n");
        QCOMPARE(migrated(text), text);
    }

    void refusesABrokenConfig() { QVERIFY(migrated(QStringLiteral("binds {\n")).startsWith(QStringLiteral("<error: "))); }

    void theShippedDefaultsAlreadyHaveEveryMigratedBind()
    {
        QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString shipped = QString::fromUtf8(file.readAll());
        for (const ConfigMigration &migration : configMigrations()) {
            const auto result = applyMigration(migration, shipped, fileName, shipped);
            QVERIFY2(result, result ? "" : qPrintable(result.error()));
            QCOMPARE(*result, shipped);
            QVERIFY(shipsAny(shipped, migration.shippedPaths));
        }
    }
};

QTEST_GUILESS_MAIN(TestSettingsMigration)

#include "test_settings_migration.moc"
