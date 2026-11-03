#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigIncludePaths : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void aBareTildeIncludesTheHomeDirectory();
    void absoluteIncludesKeepTheirOwnPath();
    void errorsInIncludesOutsideTheConfigDirectoryNameTheFullPath();
    void errorsInIncludesUnderARootConfigDirectoryDropTheLeadingSlash();

private:
    ConfigDir m_dir;
    ConfigDir m_outside;
    QByteArray m_home;
};

void TestConfigIncludePaths::init()
{
    m_dir.clear();
    m_outside.clear();
    m_home = qgetenv("HOME");
}

void TestConfigIncludePaths::cleanup()
{
    qputenv("HOME", m_home);
}

void TestConfigIncludePaths::aBareTildeIncludesTheHomeDirectory()
{
    qputenv("HOME", QFile::encodeName(m_dir.path(QStringLiteral("home"))));
    m_dir.write(QStringLiteral("config.kdl"), QStringLiteral("include \"~\"\n"));
    QVERIFY(
        m_dir.failure().message.startsWith(QStringLiteral("failed to read included config from ") + m_dir.path(QStringLiteral("home"))));
}

void TestConfigIncludePaths::absoluteIncludesKeepTheirOwnPath()
{
    m_outside.write(QStringLiteral("extra.kdl"), QStringLiteral("layout { gaps 9; }\n"));
    m_dir.write(QStringLiteral("config.kdl"), QStringLiteral("include \"%1\"\n").arg(m_outside.path(QStringLiteral("extra.kdl"))));
    const LoadResult result = m_dir.load();
    QCOMPARE(result.config.layout.gaps, 9.0);
    QCOMPARE(result.files.at(1), m_outside.path(QStringLiteral("extra.kdl")));
}

void TestConfigIncludePaths::errorsInIncludesOutsideTheConfigDirectoryNameTheFullPath()
{
    m_outside.write(QStringLiteral("extra.kdl"), QStringLiteral("layout {\n"));
    m_dir.write(QStringLiteral("config.kdl"), QStringLiteral("include \"%1\"\n").arg(m_outside.path(QStringLiteral("extra.kdl"))));
    QCOMPARE(m_dir.failure().location.file, m_outside.path(QStringLiteral("extra.kdl")));
}

void TestConfigIncludePaths::errorsInIncludesUnderARootConfigDirectoryDropTheLeadingSlash()
{
    const QString included = m_outside.path(QStringLiteral("extra.kdl"));
    m_outside.write(QStringLiteral("extra.kdl"), QStringLiteral("layout {\n"));
    const auto result = loadString(QStringLiteral("include \"%1\"\n").arg(included), QStringLiteral("/config.kdl"));
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().location.file, included.mid(1));
}

QTEST_MAIN(TestConfigIncludePaths)
#include "test_config_includepaths.moc"
