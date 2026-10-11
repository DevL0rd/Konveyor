#include "document/configmigration.h"

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
        QVERIFY(shipsAnyBind(defaults, keys));
        QVERIFY(!shipsAnyBind(QStringLiteral("binds {\n    Mod+1 { focus-workspace 1; }\n}\n"), keys));
        QVERIFY(!shipsAnyBind(QString(), keys));
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
        QCOMPARE(fullscreenMigration.defaultBinds, QStringList {QStringLiteral("Mod+F")});
        QCOMPARE(fullscreenMigration.replacesAction, QStringLiteral("cycle-window-expansion"));
        const auto result = applyMigration(fullscreenMigration, text, fileName, shipped);
        QVERIFY2(result, result ? "" : qPrintable(result.error()));
        QCOMPARE(*result, expected);
        const auto again = applyMigration(fullscreenMigration, *result, fileName, shipped);
        QVERIFY2(again, again ? "" : qPrintable(again.error()));
        QCOMPARE(*again, expected);
    }

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
            for (const QString &key : migration.defaultBinds) {
                QVERIFY2(shipped.contains(QStringLiteral("    ") + key + QStringLiteral(" {")), qPrintable(key));
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestSettingsMigration)

#include "test_settings_migration.moc"
