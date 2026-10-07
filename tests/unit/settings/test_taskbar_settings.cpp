#include "store/taskbarsettings.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using Konveyor::Settings::TaskbarSettings;

class TestTaskbarSettings : public QObject
{
    Q_OBJECT

    QTemporaryDir m_home;

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        qputenv("XDG_CONFIG_HOME", m_home.filePath(QStringLiteral("config")).toUtf8());
        QVERIFY(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).startsWith(m_home.path()));
    }

    void init() { QFile::remove(m_home.filePath(QStringLiteral("config/konveyor/taskbarrc"))); }

    void startsFromTheDefaults()
    {
        TaskbarSettings settings;
        QCOMPARE(settings.path(), m_home.filePath(QStringLiteral("config/konveyor/taskbarrc")));
        QCOMPARE(settings.values()->value(QStringLiteral("iconSpacing")).toInt(), 2);
        QCOMPARE(settings.values()->value(QStringLiteral("showApps")).toBool(), true);
        QCOMPARE(settings.values()->value(QStringLiteral("launchers")).toStringList(), QStringList());
        QVERIFY(settings.isDefault(QStringLiteral("iconSpacing")));
        QCOMPARE(settings.defaultValue(QStringLiteral("pillContent")).toInt(), 0);
        QVERIFY(!settings.defaultValue(QStringLiteral("nothing")).isValid());
    }

    void aChangeReachesEveryReader()
    {
        TaskbarSettings settingsApp;
        TaskbarSettings widget;
        QVERIFY(settingsApp.values()->setProperty("iconSpacing", 7));
        QVERIFY(settingsApp.values()->setProperty("launchers", QStringList {QStringLiteral("applications:firefox.desktop")}));
        QVERIFY(QFile::exists(settingsApp.path()));
        QTRY_COMPARE(widget.values()->value(QStringLiteral("iconSpacing")).toInt(), 7);
        QTRY_COMPARE(widget.values()->value(QStringLiteral("launchers")).toStringList(),
            QStringList {QStringLiteral("applications:firefox.desktop")});
        QVERIFY(!widget.isDefault(QStringLiteral("iconSpacing")));
        widget.reset(QStringLiteral("iconSpacing"));
        QCOMPARE(widget.values()->value(QStringLiteral("iconSpacing")).toInt(), 2);
        QTRY_COMPARE(settingsApp.values()->value(QStringLiteral("iconSpacing")).toInt(), 2);
    }

    void outsideEditsAreFollowed()
    {
        TaskbarSettings widget;
        QDir().mkpath(m_home.filePath(QStringLiteral("config/konveyor")));
        QFile file(widget.path());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[General]\nshowWorkspaces=false\n");
        file.close();
        QTRY_COMPARE(widget.values()->value(QStringLiteral("showWorkspaces")).toBool(), false);
    }
};

QTEST_GUILESS_MAIN(TestTaskbarSettings)

#include "test_taskbar_settings.moc"
