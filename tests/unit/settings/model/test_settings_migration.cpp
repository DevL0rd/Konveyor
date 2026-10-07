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

    void isIdempotent()
    {
        const QString once = migrated(QStringLiteral("binds {\n}\n"));
        QCOMPARE(migrated(once), once);
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
            const auto result = addDefaultBinds(shipped, fileName, shipped, migration.defaultBinds);
            QVERIFY2(result, qPrintable(result.error()));
            QCOMPARE(*result, shipped);
            for (const QString &key : migration.defaultBinds) {
                QVERIFY2(shipped.contains(QStringLiteral("    ") + key + QStringLiteral(" {")), qPrintable(key));
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestSettingsMigration)

#include "test_settings_migration.moc"
