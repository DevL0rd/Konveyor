#include "store/includeoverrides.h"

#include "documenttesthelpers.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Konveyor::Settings;

namespace
{

bool write(const QDir &dir, const QString &name, const QString &text)
{
    dir.mkpath(QFileInfo(name).path());
    QFile file(dir.filePath(name));
    return file.open(QIODevice::WriteOnly) && file.write(text.toUtf8()) == text.toUtf8().size();
}

}

class TestSettingsIncludeOverrides : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init() { QVERIFY(m_dir.isValid()); }

    void nothingOverridesAnUnreadableQuestion()
    {
        const QString config = m_dir.filePath(QStringLiteral("config.kdl"));
        QVERIFY(write(m_dir.path(), QStringLiteral("later.kdl"), QStringLiteral("layout {\n    gaps 4\n}\n")));
        const QString main = QStringLiteral("layout {\n    gaps 8\n}\ninclude \"later.kdl\"\n");
        QCOMPARE(includesOverriding(main, config, QStringLiteral("layout/gaps")), QStringList {QStringLiteral("later.kdl")});
        QCOMPARE(includesOverriding(main, config, QStringLiteral("layout/gaps#x")), QStringList());
        QCOMPARE(includesOverriding(main, config, QString()), QStringList());
        QCOMPARE(includesOverriding(QStringLiteral("layout {"), config, QStringLiteral("layout/gaps")), QStringList());
    }

    void includesThatCannotBeFollowedAreSkipped()
    {
        const QString config = m_dir.filePath(QStringLiteral("config.kdl"));
        QVERIFY(write(m_dir.path(), QStringLiteral("broken.kdl"), QStringLiteral("layout {")));
        QVERIFY(write(m_dir.path(), QStringLiteral("good.kdl"), QStringLiteral("layout {\n    gaps 4\n}\n")));
        const QString main = QStringLiteral("include\ninclude 5\ninclude \"missing.kdl\"\ninclude \"broken.kdl\"\ninclude \"good.kdl\"\n"
                                            "include \"good.kdl\"\n");
        QCOMPARE(includesOverriding(main, config, QStringLiteral("layout/gaps")), QStringList {QStringLiteral("good.kdl")});
    }

    void absoluteIncludesAreFollowedAsTheyAre()
    {
        QVERIFY(write(m_dir.path(), QStringLiteral("elsewhere/abs.kdl"), QStringLiteral("layout {\n    gaps 4\n}\n")));
        const QString config = m_dir.filePath(QStringLiteral("config.kdl"));
        const QString main = QStringLiteral("include \"%1\"\n").arg(m_dir.filePath(QStringLiteral("elsewhere/abs.kdl")));
        QCOMPARE(includesOverriding(main, config, QStringLiteral("layout/gaps")), QStringList {QStringLiteral("elsewhere/abs.kdl")});
    }

    void tildeMeansTheHomeFolder()
    {
        const QTemporaryDir home;
        QVERIFY(home.isValid());
        qputenv("HOME", home.path().toUtf8());
        QVERIFY(write(home.path(), QStringLiteral("shared.kdl"), QStringLiteral("layout {\n    gaps 4\n}\n")));
        const QString config = m_dir.filePath(QStringLiteral("config.kdl"));
        const QString main = QStringLiteral("include \"~\"\ninclude \"~/shared.kdl\"\n");
        QCOMPARE(includesOverriding(main, config, QStringLiteral("layout/gaps")),
            QStringList {QDir(m_dir.path()).relativeFilePath(home.filePath(QStringLiteral("shared.kdl")))});
    }

    void windowRulesAreComparedByTheirMatchesAndExcludes()
    {
        const QString config = m_dir.filePath(QStringLiteral("config.kdl"));
        const QString rule = QStringLiteral("window-rule {\n    match app-id=\"^a$\"\n    exclude title=\"x\"\n    %1\n}\n");
        QVERIFY(write(m_dir.path(), QStringLiteral("same.kdl"),
            QStringLiteral("layout {\n}\n") + rule.arg(QStringLiteral("open-maximized true"))
                + rule.arg(QStringLiteral("open-floating true"))));
        QVERIFY(write(m_dir.path(), QStringLiteral("other.kdl"),
            QStringLiteral("window-rule {\n    match app-id=\"^b$\"\n    open-floating true\n}\nwindow-rule {\n    match app-id=\"^a$\"\n  "
                           "  open-maximized true\n}\n")));
        const QString main
            = rule.arg(QStringLiteral("open-floating false")) + QStringLiteral("include \"same.kdl\"\ninclude \"other.kdl\"\n");
        QCOMPARE(includesOverriding(main, config, QStringLiteral("window-rule/open-floating")), QStringList {QStringLiteral("same.kdl")});
        QCOMPARE(includesOverriding(main, config, QStringLiteral("window-rule")), QStringList());
        QCOMPARE(includesOverriding(main, config, QStringLiteral("output/mode")), QStringList());
    }

private:
    QTemporaryDir m_dir;
};

QTEST_GUILESS_MAIN(TestSettingsIncludeOverrides)

#include "test_settings_include_overrides.moc"
