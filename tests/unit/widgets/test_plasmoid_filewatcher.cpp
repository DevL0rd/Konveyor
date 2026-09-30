#include "plasmoidharness.h"

#include <QDir>
#include <QQmlComponent>
#include <QSignalSpy>
#include <QTest>

#include <cstdio>

namespace
{

void replaceFile(const QString &path, const QByteArray &content)
{
    QFile file(path + QStringLiteral(".tmp"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
    file.close();
    QCOMPARE(std::rename(QFile::encodeName(file.fileName()).constData(), QFile::encodeName(path).constData()), 0);
}

}

class TestPlasmoidFileWatcher : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        m_engine = std::make_unique<QQmlEngine>();
        QQmlComponent component(
            m_engine.get(), QUrl::fromLocalFile(PlasmoidHarness::widgetsDir() + QStringLiteral("/shared/common/FileWatcher.qml")));
        m_watcher.reset(component.createWithInitialProperties({{QStringLiteral("root"), m_dir->path()}}));
        QVERIFY2(m_watcher, qPrintable(component.errorString()));
        m_changed = std::make_unique<QSignalSpy>(m_watcher.get(), SIGNAL(changed()));
    }

    void cleanup()
    {
        m_changed.reset();
        m_watcher.reset();
        m_engine.reset();
    }

    void firesForEveryReplacement()
    {
        const QString path = m_dir->filePath(QStringLiteral("data.json"));
        replaceFile(path, "1");
        m_watcher->setProperty("path", path);
        QTRY_COMPARE(m_changed->count(), 1);
        replaceFile(path, "22");
        QTRY_COMPARE(m_changed->count(), 2);
        replaceFile(path, "333");
        QTRY_COMPARE(m_changed->count(), 3);
    }

    void firesWhenTheFileAppears()
    {
        const QString path = m_dir->filePath(QStringLiteral("data.json"));
        m_watcher->setProperty("path", path);
        QTRY_VERIFY(watching(m_watcher.get(), m_dir->path()));
        replaceFile(path, "1");
        QTRY_COMPARE(m_changed->count(), 1);
    }

    void waitsForAMissingDirectory()
    {
        const QString path = m_dir->filePath(QStringLiteral("a/b/data.json"));
        m_watcher->setProperty("path", path);
        QTRY_VERIFY(watching(m_watcher.get(), m_dir->path()));
        QVERIFY(QDir().mkpath(m_dir->filePath(QStringLiteral("a/b"))));
        QTRY_VERIFY(watching(m_watcher.get(), m_dir->filePath(QStringLiteral("a/b"))));
        replaceFile(path, "1");
        QTRY_COMPARE(m_changed->count(), 1);
        replaceFile(path, "22");
        QTRY_COMPARE(m_changed->count(), 2);
        QVERIFY(m_watcher->property("exists").toBool());
    }

    void followsADirectoryThatIsRemovedAndMadeAgain()
    {
        const QString path = m_dir->filePath(QStringLiteral("a/data.json"));
        QVERIFY(QDir().mkpath(m_dir->filePath(QStringLiteral("a"))));
        replaceFile(path, "1");
        m_watcher->setProperty("path", path);
        QTRY_COMPARE(m_changed->count(), 1);
        QVERIFY(QDir(m_dir->filePath(QStringLiteral("a"))).removeRecursively());
        QTRY_VERIFY(!m_watcher->property("exists").toBool());
        QVERIFY(QDir().mkpath(m_dir->filePath(QStringLiteral("a"))));
        QTRY_VERIFY(watching(m_watcher.get(), m_dir->filePath(QStringLiteral("a"))));
        replaceFile(path, "22");
        QTRY_COMPARE(m_changed->count(), 2);
    }

    void ignoresOtherFiles()
    {
        const QString path = m_dir->filePath(QStringLiteral("data.json"));
        replaceFile(path, "1");
        m_watcher->setProperty("path", path);
        QTRY_COMPARE(m_changed->count(), 1);
        replaceFile(m_dir->filePath(QStringLiteral("other.json")), "1");
        replaceFile(path, "22");
        QTRY_COMPARE(m_changed->count(), 2);
    }

    void stopsWhenThePathIsCleared()
    {
        const QString path = m_dir->filePath(QStringLiteral("data.json"));
        replaceFile(path, "1");
        m_watcher->setProperty("path", path);
        QTRY_COMPARE(m_changed->count(), 1);
        m_watcher->setProperty("path", QString());
        replaceFile(path, "22");
        m_watcher->setProperty("path", m_dir->filePath(QStringLiteral("sentinel.json")));
        QTRY_VERIFY(watching(m_watcher.get(), m_dir->path()));
        replaceFile(m_dir->filePath(QStringLiteral("sentinel.json")), "1");
        QTRY_COMPARE(m_changed->count(), 2);
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_watcher;
    std::unique_ptr<QSignalSpy> m_changed;
};

QTEST_MAIN(TestPlasmoidFileWatcher)

#include "test_plasmoid_filewatcher.moc"
