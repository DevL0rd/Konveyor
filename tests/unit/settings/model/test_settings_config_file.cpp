#include "store/configfile.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <sys/resource.h>

#include <csignal>

namespace
{

class FileSizeLimit
{
public:
    explicit FileSizeLimit(rlim_t bytes)
    {
        getrlimit(RLIMIT_FSIZE, &m_previous);
        const rlimit limit {bytes, m_previous.rlim_max};
        setrlimit(RLIMIT_FSIZE, &limit);
        m_handler = std::signal(SIGXFSZ, SIG_IGN);
    }

    ~FileSizeLimit()
    {
        std::signal(SIGXFSZ, m_handler);
        setrlimit(RLIMIT_FSIZE, &m_previous);
    }

    FileSizeLimit(const FileSizeLimit &) = delete;
    FileSizeLimit &operator=(const FileSizeLimit &) = delete;

private:
    rlimit m_previous {};
    void (*m_handler)(int) = SIG_DFL;
};

}

using namespace Konveyor::Settings;

class TestSettingsConfigFile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writesAndReadsBackAndCreatesFolders()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("a/b/config.kdl"));
        QVERIFY(writeConfigText(path, QStringLiteral("layout {\n}\n")).has_value());
        QCOMPARE(readConfigText(path).value_or(QString()), QStringLiteral("layout {\n}\n"));
        QVERIFY(!readConfigText(dir.filePath(QStringLiteral("missing.kdl"))));
    }

    void reportsAFolderItCannotWriteTo()
    {
        QTemporaryDir dir;
        QVERIFY(QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
        const QString path = dir.filePath(QStringLiteral("config.kdl"));
        const auto written = writeConfigText(path, QStringLiteral("x"));
        QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        QVERIFY(!written);
        QVERIFY(written.error().startsWith(QStringLiteral("Could not write ") + path));
    }

    void keepsTheOldFileWhenTheNewTextDoesNotFit()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("config.kdl"));
        QVERIFY(writeConfigText(path, QStringLiteral("old")).has_value());
        std::expected<void, QString> written;
        {
            const FileSizeLimit limit(8);
            written = writeConfigText(path, QString(200, QLatin1Char('x')));
        }
        QVERIFY(!written);
        QVERIFY(written.error().startsWith(QStringLiteral("Could not write ") + path));
        QCOMPARE(readConfigText(path).value_or(QString()), QStringLiteral("old"));
    }
};

QTEST_GUILESS_MAIN(TestSettingsConfigFile)

#include "test_settings_config_file.moc"
