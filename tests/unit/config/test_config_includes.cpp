#include "configtesthelpers.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigIncludes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void mergesIncludedSections();
    void appliesPositionalOrder();
    void appendsListsAtIncludePosition();
    void overridesBindsByKey();
    void doesNotMergeStrutsOrPresets();
    void resolvesRelativeToIncludingFile();
    void reportsRecursiveInclude();
    void reportsRecursionLimit();
    void warnsAboutMissingOptionalInclude();
    void failsOnMissingRequiredInclude();
    void reportsErrorsInsideIncludes();
    void rejectsMalformedIncludeNode();
    void emptyBorderQuirkOnlyAppliesToMainFile();

private:
    QTemporaryDir m_dir;
    void write(const QString &name, const QString &text);
    QString path(const QString &name) const;
    LoadResult loadMain();
};

void TestConfigIncludes::init()
{
    QVERIFY(m_dir.isValid());
    const QDir dir(m_dir.path());
    for (const QString &entry : dir.entryList(QDir::Files)) {
        QVERIFY(QFile::remove(dir.filePath(entry)));
    }
    QDir(m_dir.filePath(QStringLiteral("nested"))).removeRecursively();
}

void TestConfigIncludes::write(const QString &name, const QString &text)
{
    const QString full = path(name);
    QVERIFY(QDir().mkpath(QFileInfo(full).path()));
    QFile file(full);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(file.write(text.toUtf8()) >= 0);
}

QString TestConfigIncludes::path(const QString &name) const
{
    return QDir(m_dir.path()).filePath(name);
}

LoadResult TestConfigIncludes::loadMain()
{
    const auto result = loadFile(path(QStringLiteral("config.kdl")));
    if (!result) {
        qWarning("unexpected config error: %s", qPrintable(result.error().toString()));
        return LoadResult {};
    }
    return *result;
}

void TestConfigIncludes::mergesIncludedSections()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("layout {\n    gaps 4\n}\ninclude \"extra.kdl\"\n"));
    write(QStringLiteral("extra.kdl"), QStringLiteral("layout {\n    center-focused-column \"always\"\n}\n"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.layout.gaps, 4.0);
    QCOMPARE(result.config.layout.centerFocusedColumn, CenterFocusedColumn::Always);
    QCOMPARE(result.files.size(), 2);
    QVERIFY(result.files.at(1).endsWith(QStringLiteral("extra.kdl")));
}

void TestConfigIncludes::appliesPositionalOrder()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"first.kdl\"\nlayout {\n    gaps 9\n}\ninclude \"second.kdl\"\n"));
    write(QStringLiteral("first.kdl"), QStringLiteral("layout {\n    gaps 1\n}\n"));
    write(QStringLiteral("second.kdl"), QStringLiteral("layout {\n    always-center-single-column\n}\n"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.layout.gaps, 9.0);
    QCOMPARE(result.config.layout.alwaysCenterSingleColumn, true);

    write(QStringLiteral("config.kdl"), QStringLiteral("layout {\n    gaps 9\n}\ninclude \"first.kdl\"\n"));
    QCOMPARE(loadMain().config.layout.gaps, 1.0);
}

void TestConfigIncludes::appendsListsAtIncludePosition()
{
    write(QStringLiteral("config.kdl"), QStringLiteral(R"(
window-rule {
    match app-id="^a$"
}
include "more.kdl"
window-rule {
    match app-id="^c$"
}
workspace "main"
)"));
    write(QStringLiteral("more.kdl"), QStringLiteral(R"(
window-rule {
    match app-id="^b$"
}
workspace "chat"
output "eDP-1" {
    layout {
        gaps 3
    }
}
)"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.windowRules.size(), 3);
    QCOMPARE(result.config.windowRules.at(0).matches.first().appId->pattern(), QStringLiteral("^a$"));
    QCOMPARE(result.config.windowRules.at(1).matches.first().appId->pattern(), QStringLiteral("^b$"));
    QCOMPARE(result.config.windowRules.at(2).matches.first().appId->pattern(), QStringLiteral("^c$"));
    QCOMPARE(result.config.workspaces.size(), 2);
    QCOMPARE(result.config.workspaces.first().name, QStringLiteral("chat"));
    QCOMPARE(result.config.outputs.size(), 1);
    QCOMPARE(result.config.outputs.first().layout->gaps, 3.0);
}

void TestConfigIncludes::overridesBindsByKey()
{
    write(QStringLiteral("config.kdl"), QStringLiteral(R"(
binds {
    Mod+A { close-window; }
    Mod+B { toggle-overview; }
}
include "override.kdl"
)"));
    write(QStringLiteral("override.kdl"), QStringLiteral(R"(
binds {
    Mod+A { maximize-column; }
    Mod+C { center-column; }
}
)"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.binds.size(), 3);
    const auto action = [&](const QString &text) {
        for (const Bind &bind : result.config.binds) {
            if (bind.keyText == text) {
                return bind.action.name;
            }
        }
        return QString();
    };
    QCOMPARE(action(QStringLiteral("A")), QStringLiteral("maximize-column"));
    QCOMPARE(action(QStringLiteral("B")), QStringLiteral("toggle-overview"));
    QCOMPARE(action(QStringLiteral("C")), QStringLiteral("center-column"));
}

void TestConfigIncludes::doesNotMergeStrutsOrPresets()
{
    write(QStringLiteral("config.kdl"), QStringLiteral(R"(
layout {
    struts {
        left 10
        right 10
    }
    preset-column-widths {
        proportion 0.25
    }
}
include "extra.kdl"
)"));
    write(QStringLiteral("extra.kdl"), QStringLiteral(R"(
layout {
    struts {
        top 5
    }
    preset-column-widths {
        proportion 0.9
        fixed 100
    }
}
)"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.layout.struts, (Struts {0, 0, 5, 0}));
    QCOMPARE(result.config.layout.presetColumnWidths.size(), 2);
    QCOMPARE(proportionOf(result.config.layout.presetColumnWidths.first()), 0.9);
}

void TestConfigIncludes::resolvesRelativeToIncludingFile()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"nested/a.kdl\"\n"));
    write(QStringLiteral("nested/a.kdl"), QStringLiteral("include \"b.kdl\"\n"));
    write(QStringLiteral("nested/b.kdl"), QStringLiteral("layout {\n    gaps 7\n}\n"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.layout.gaps, 7.0);
    QCOMPARE(result.files.size(), 3);
    QVERIFY(result.files.at(2).endsWith(QStringLiteral("nested/b.kdl")));
}

void TestConfigIncludes::reportsRecursiveInclude()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"loop.kdl\"\n"));
    write(QStringLiteral("loop.kdl"), QStringLiteral("include \"loop.kdl\"\n"));

    const auto result = loadFile(path(QStringLiteral("config.kdl")));
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().message, QStringLiteral("recursive include (file includes itself)"));
    QCOMPARE(result.error().location.file, QStringLiteral("loop.kdl"));
}

void TestConfigIncludes::reportsRecursionLimit()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"level1.kdl\"\n"));
    for (int level = 1; level <= 12; ++level) {
        write(QStringLiteral("level%1.kdl").arg(level), QStringLiteral("include \"level%1.kdl\"\n").arg(level + 1));
    }

    const auto result = loadFile(path(QStringLiteral("config.kdl")));
    QVERIFY(!result.has_value());
    QVERIFY2(result.error().message.contains(QStringLiteral("recursion limit")), qPrintable(result.error().message));
    QVERIFY(result.error().message.contains(QStringLiteral("10 levels deep")));
}

void TestConfigIncludes::warnsAboutMissingOptionalInclude()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include optional=true \"absent.kdl\"\nlayout {\n gaps 3\n}\n"));

    const LoadResult result = loadMain();
    QCOMPARE(result.config.layout.gaps, 3.0);
    QCOMPARE(result.warnings.size(), 1);
    QVERIFY(result.warnings.first().contains(QStringLiteral("optional include not found")));
    QCOMPARE(result.files.size(), 2);
    QVERIFY(result.files.at(1).endsWith(QStringLiteral("absent.kdl")));
}

void TestConfigIncludes::failsOnMissingRequiredInclude()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"absent.kdl\"\n"));

    const auto result = loadFile(path(QStringLiteral("config.kdl")));
    QVERIFY(!result.has_value());
    QVERIFY(result.error().message.startsWith(QStringLiteral("failed to read included config from ")));
    QVERIFY(result.error().files.size() == 2);
}

void TestConfigIncludes::reportsErrorsInsideIncludes()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"bad.kdl\"\n"));
    write(QStringLiteral("bad.kdl"), QStringLiteral("layout {\n    bogus 1\n}\n"));

    const auto result = loadFile(path(QStringLiteral("config.kdl")));
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().message, QStringLiteral("unexpected node `bogus`"));
    QCOMPARE(result.error().location.file, QStringLiteral("bad.kdl"));
    QCOMPARE(result.error().location.line, 2);
    QCOMPARE(result.error().sourceLine, QStringLiteral("    bogus 1"));
}

void TestConfigIncludes::rejectsMalformedIncludeNode()
{
    QVERIFY(mustFail(QStringLiteral("include\n")).message.contains(QStringLiteral("is required")));
    QVERIFY(mustFail(QStringLiteral("include \"a.kdl\" \"b.kdl\"\n")).message.contains(QStringLiteral("unexpected argument")));
    QVERIFY(mustFail(QStringLiteral("include bogus=true \"a.kdl\"\n")).message.contains(QStringLiteral("unexpected property")));
    QVERIFY(mustFail(QStringLiteral("include \"a.kdl\" {\n  layout {\n  }\n}\n")).message.contains(QStringLiteral("unexpected node")));
}

void TestConfigIncludes::emptyBorderQuirkOnlyAppliesToMainFile()
{
    write(QStringLiteral("config.kdl"), QStringLiteral("include \"extra.kdl\"\n"));
    write(QStringLiteral("extra.kdl"), QStringLiteral("layout {\n    border {\n    }\n}\n"));
    QCOMPARE(loadMain().config.layout.border.enabled, false);

    write(QStringLiteral("config.kdl"), QStringLiteral("layout {\n    border {\n    }\n}\n"));
    QCOMPARE(loadMain().config.layout.border.enabled, true);

    write(QStringLiteral("config.kdl"), QStringLiteral("output \"eDP-1\" {\n    layout {\n        border {\n        }\n    }\n}\n"));
    QCOMPARE(loadMain().config.outputs.first().layout->border.enabled, false);
}

QTEST_MAIN(TestConfigIncludes)
#include "test_config_includes.moc"
