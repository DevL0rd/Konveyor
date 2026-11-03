#include "configfiles.h"
#include "filefaults.h"
#include "plugin/windowmemorystore.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using Konveyor::WindowMemoryStore;
using Konveyor::Layout::RememberedWindow;
using Konveyor::Layout::WindowMemory;
using Konveyor::Test::isolate;
using Konveyor::Test::Permissions;
using Konveyor::Test::writeInPlace;
using Konveyor::Test::WriteLimit;

class TestWindowMemoryStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void startsEmptyWithoutAFile();
    void remembersWhatWasSaved();
    void ignoresAFileThatIsNotJson();
    void keepsTheOldFileWhenTheFolderCannotBeWritten();
    void keepsTheOldFileWhenTheWriteIsCutShort();

private:
    QString storePath() const { return m_dir.filePath(QStringLiteral("state/konveyor/window-memory.json")); }
    static WindowMemory sample();

    QTemporaryDir m_dir;
};

WindowMemory TestWindowMemoryStore::sample()
{
    RememberedWindow window;
    window.floatingSize = QSize(640, 480);
    window.floatingPosition = QPointF(10, 20);
    return {{QStringLiteral("org.example.app"), window}};
}

void TestWindowMemoryStore::init()
{
    QVERIFY(m_dir.isValid());
    isolate(m_dir.path());
    QDir(m_dir.path()).removeRecursively();
    QVERIFY(QDir().mkpath(m_dir.path()));
}

void TestWindowMemoryStore::startsEmptyWithoutAFile()
{
    QVERIFY(WindowMemoryStore().load().isEmpty());
}

void TestWindowMemoryStore::remembersWhatWasSaved()
{
    WindowMemoryStore().save(sample());
    QCOMPARE(WindowMemoryStore().load(), sample());
}

void TestWindowMemoryStore::ignoresAFileThatIsNotJson()
{
    QVERIFY(QDir().mkpath(QFileInfo(storePath()).absolutePath()));
    QVERIFY(writeInPlace(storePath(), QStringLiteral("{ not json")));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not read .*window-memory.json")));
    QVERIFY(WindowMemoryStore().load().isEmpty());
}

void TestWindowMemoryStore::keepsTheOldFileWhenTheFolderCannotBeWritten()
{
    const WindowMemoryStore store;
    store.save(sample());
    {
        const Permissions locked(QFileInfo(storePath()).absolutePath(), QFileDevice::ReadOwner | QFileDevice::ExeOwner);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not write .*window-memory.json")));
        store.save({});
    }
    QCOMPARE(store.load(), sample());
}

void TestWindowMemoryStore::keepsTheOldFileWhenTheWriteIsCutShort()
{
    const WindowMemoryStore store;
    store.save(sample());
    {
        const WriteLimit limit(0);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not write .*window-memory.json")));
        store.save({});
    }
    QCOMPARE(store.load(), sample());
}

QTEST_GUILESS_MAIN(TestWindowMemoryStore)
#include "test_window_memory_store.moc"
