#include "tools/widgettool.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace Konveyor::Tools;

class TestWidgetTool : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_home = std::make_unique<QTemporaryDir>();
        QVERIFY(m_home->isValid());
        qputenv("HOME", QFile::encodeName(m_home->path()));
        QVERIFY(QDir(m_home->path()).mkpath(QStringLiteral(".local/bin")));
        QVERIFY(QDir(m_home->path()).mkpath(QStringLiteral("usr/bin")));
        m_path = qgetenv("PATH");
        qputenv("PATH", QFile::encodeName(m_home->filePath(QStringLiteral("usr/bin"))));
    }

    void cleanup()
    {
        qputenv("PATH", m_path);
        m_home.reset();
    }

    void findsToolsInTheUsersLocalBin()
    {
        QCOMPARE(widgetToolPath(QStringLiteral("portal-launcher")), m_home->filePath(QStringLiteral(".local/bin/portal-launcher")));
    }

    void fallsBackToPathForSystemPackages()
    {
        writeTool(QStringLiteral("#!/bin/sh\n"), QFileDevice::ReadOwner | QFileDevice::ExeOwner, QStringLiteral("usr/bin"));
        QCOMPARE(widgetToolPath(QStringLiteral("portal-launcher")), m_home->filePath(QStringLiteral("usr/bin/portal-launcher")));
        QVERIFY(widgetToolInstalled(QStringLiteral("portal-launcher")));
    }

    void prefersTheUsersLocalBinOverPath()
    {
        writeTool(QStringLiteral("#!/bin/sh\n"), QFileDevice::ReadOwner | QFileDevice::ExeOwner, QStringLiteral("usr/bin"));
        writeTool(QStringLiteral("#!/bin/sh\n"), QFileDevice::ReadOwner | QFileDevice::ExeOwner);
        QCOMPARE(widgetToolPath(QStringLiteral("portal-launcher")), m_home->filePath(QStringLiteral(".local/bin/portal-launcher")));
    }

    void aMissingToolFailsWithAMessageNamingIt()
    {
        QVERIFY(!widgetToolInstalled(QStringLiteral("portal-launcher")));
        const auto started = startWidgetTool(QStringLiteral("portal-launcher"), {QStringLiteral("toggle")});
        QVERIFY(!started);
        QVERIFY(started.error().contains(m_home->filePath(QStringLiteral(".local/bin/portal-launcher"))));
        QVERIFY(started.error().contains(QStringLiteral("--no-widgets")));
    }

    void aToolThatCannotRunIsNotInstalled()
    {
        writeTool(QStringLiteral("#!/bin/sh\n"), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QVERIFY(!widgetToolInstalled(QStringLiteral("portal-launcher")));
        QVERIFY(!startWidgetTool(QStringLiteral("portal-launcher"), {}));
    }

    void aDirectoryIsNotATool()
    {
        QVERIFY(QDir(m_home->path()).mkpath(QStringLiteral(".local/bin/portal-launcher")));
        QVERIFY(!widgetToolInstalled(QStringLiteral("portal-launcher")));
    }

    void startsAnInstalledToolWithItsArguments()
    {
        qputenv("PATH", m_path); // the tool script needs mv
        const QString record = m_home->filePath(QStringLiteral("arguments"));
        writeTool(QStringLiteral("#!/bin/sh\nprintf '%s\\n' \"$@\" > \"%1.part\"\nmv \"%1.part\" \"%1\"\n").arg(record),
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        QVERIFY(widgetToolInstalled(QStringLiteral("portal-launcher")));
        QVERIFY(startWidgetTool(QStringLiteral("portal-launcher"), {QStringLiteral("toggle"), QStringLiteral("two words")}));
        QTRY_VERIFY(QFile::exists(record));
        QFile file(record);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("toggle\ntwo words\n"));
    }

private:
    void writeTool(const QString &script, QFileDevice::Permissions permissions, const QString &directory = QStringLiteral(".local/bin"))
    {
        QFile tool(m_home->filePath(directory + QStringLiteral("/portal-launcher")));
        QVERIFY(tool.open(QIODevice::WriteOnly));
        tool.write(script.toUtf8());
        tool.close();
        QVERIFY(tool.setPermissions(permissions));
    }

    std::unique_ptr<QTemporaryDir> m_home;
    QByteArray m_path;
};

QTEST_GUILESS_MAIN(TestWidgetTool)
#include "test_widgettool.moc"
