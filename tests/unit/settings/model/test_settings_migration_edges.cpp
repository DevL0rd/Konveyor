#include "document/borderdefaults.h"
#include "document/configmigration.h"

#include <QTest>

using namespace Konveyor::Settings;

namespace
{

const QString fileName = QStringLiteral("/tmp/konveyor-migration/config.kdl");

QString text(const std::expected<QString, QString> &result)
{
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

QString replaced(const QString &config, const QString &defaults, const QStringList &keys, const QString &action)
{
    return text(replaceBindAction(config, fileName, defaults, keys, action));
}

QString borders(const QString &config, const QString &defaults)
{
    return text(migrateBorderDefaults(config, fileName, defaults));
}

QString heat(const QString &config)
{
    return text(migrateHeatLayers(config, fileName, QString()));
}

const QString shippedRing = QStringLiteral(R"(layout {
    focus-ring {
        width 2
        active-style "Glow"
        inactive-style "Dim"
    }
}
border-style "Glow" {
    layer "gradient"
}
border-style "Dim" {
    layer "gradient"
}
)");

const QString oldRing = QStringLiteral(R"(layout {
    focus-ring {
        width 4
        inactive-color "#505050"
    }
}
)");

}

class TestSettingsMigrationEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bindReplacementRefusesABrokenConfig();
    void bindReplacementOnlyTouchesBindsItCanSwapSafely();
    void heatMigrationLeavesBrokenConfigsAlone();
    void borderDefaultsKeepAColorGivenAsChannels();
    void borderDefaultsNeedTheShippedPieces();
    void borderDefaultsNeverWriteABrokenConfig();
};

void TestSettingsMigrationEdges::bindReplacementRefusesABrokenConfig()
{
    QVERIFY(replaced(QStringLiteral("binds {"), QString(), {QStringLiteral("Mod+F")}, QStringLiteral("a"))
            .startsWith(QStringLiteral("<error: ")));
}

void TestSettingsMigrationEdges::bindReplacementOnlyTouchesBindsItCanSwapSafely()
{
    const QString defaults
        = QStringLiteral("binds {\n    Mod+C { focus-taskbar-item 1; focus-taskbar-item 1; }\n    Mod+D { focus-taskbar-item 1; }\n"
                         "    Mod+E { focus-taskbar-item 1; }\n    Mod+F { focus-taskbar-item 2; }\n    Mod+G { focus-taskbar-item 1; }\n"
                         "    Mod+H { bogus-action 1; }\n}\n");
    const QString mine = QStringLiteral("binds {\n    Mod+B { focus-column 1; }\n    Mod+C { focus-column 1; }\n    Mod+D repeat=false { "
                                        "focus-column 1; }\n    Mod+E { spawn \"x\"; }\n"
                                        "    Mod+F { focus-column 1; }\n    Mod+G { focus-column 1; }\n    Mod+H { focus-column 1; }\n}\n");
    const QStringList keys {QStringLiteral("Mod+A"), QStringLiteral("Mod+B"), QStringLiteral("Mod+C"), QStringLiteral("Mod+D"),
        QStringLiteral("Mod+E"), QStringLiteral("Mod+F"), QStringLiteral("Mod+G"), QStringLiteral("Mod+H")};
    QCOMPARE(replaced(mine, defaults, keys, QStringLiteral("focus-column")),
        QStringLiteral("binds {\n    Mod+B { focus-column 1; }\n    Mod+C { focus-column 1; }\n    Mod+D repeat=false { focus-column 1; "
                       "}\n    Mod+E { spawn \"x\"; }\n"
                       "    Mod+F { focus-column 1; }\n    Mod+G { focus-taskbar-item 1; }\n    Mod+H { focus-column 1; }\n}\n"));
}

void TestSettingsMigrationEdges::heatMigrationLeavesBrokenConfigsAlone()
{
    const QString oddLayers
        = QStringLiteral("border-style \"X\" {\n    foo\n    layer 5\n    layer \"heat\" \"extra\"\n    layer \"heat\" {\n        drive\n"
                         "        drive \"position\"\n        drive \"position\" \"scroll.distance\" \"third\"\n        drive "
                         "\"color-mix\" \"audio.level\"\n    }\n}\n");
    QCOMPARE(heat(oddLayers), oddLayers);
}

void TestSettingsMigrationEdges::borderDefaultsKeepAColorGivenAsChannels()
{
    const QString channels = QStringLiteral("layout {\n    focus-ring {\n        width 4\n        inactive-color 80 80 80 255\n    }\n}\n");
    const QString result = borders(channels, shippedRing);
    QCOMPARE(result,
        QStringLiteral("layout {\n    focus-ring {\n        width 2\n        inactive-color 80 80 80 255\n        active-style \"Glow\"\n"
                       "    }\n}\n\nborder-style \"Glow\" { layer \"gradient\"; }\n"));
}

void TestSettingsMigrationEdges::borderDefaultsNeedTheShippedPieces()
{
    const QString noWidth
        = QStringLiteral("layout {\n    focus-ring {\n        active-style \"Glow\"\n    }\n}\nborder-style \"Glow\" {\n}\n");
    QCOMPARE(borders(oldRing, noWidth), oldRing);
    const QString noStateStyle = QStringLiteral("layout {\n    focus-ring {\n        width 2\n    }\n}\n");
    QVERIFY(!borders(oldRing, noStateStyle).contains(QStringLiteral("-style")));
    const QString styleMissing = QStringLiteral("layout {\n    focus-ring {\n        width 2\n        active-style \"Ghost\"\n    }\n}\n");
    QVERIFY(!borders(oldRing, styleMissing).contains(QStringLiteral("Ghost")));
}

void TestSettingsMigrationEdges::borderDefaultsNeverWriteABrokenConfig()
{
    const QString broken = QStringLiteral("layout {\n    focus-ring {\n        width 2\n        active-style \"Glow\"\n    "
                                          "}\n}\nborder-style \"Glow\" {\n    layer \"bogus\"\n}\n");
    QCOMPARE(borders(oldRing, broken), oldRing);
}

QTEST_GUILESS_MAIN(TestSettingsMigrationEdges)

#include "test_settings_migration_edges.moc"
