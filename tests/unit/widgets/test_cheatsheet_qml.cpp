#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace
{

const char *const sectionsJson = R"([
    {"name": "Focus", "entries": [{"action": "Focus column left", "keys": ["Mod+H", "Mod+Left"]}]},
    {"name": "KDE Windows & Desktops", "entries": [{"action": "Zoom In", "keys": ["Meta++"]}, {"action": "Peek at Desktop", "keys": ["Meta+D"]}]}
])";

QStringList messages;

void collectMessages(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (type != QtDebugMsg && type != QtInfoMsg
        && !message.endsWith(QLatin1String("is not a wayland window. Not creating zwlr_layer_surface"))) {
        messages.append(message);
    }
}

void collectTexts(QQuickItem *item, QStringList &texts)
{
    if (!item->isVisible()) {
        return;
    }
    if (item->inherits("QQuickText")) {
        texts.append(item->property("text").toString());
    }
    const QList<QQuickItem *> children = item->childItems();
    for (QQuickItem *child : children) {
        collectTexts(child, texts);
    }
}

QQuickItem *findTextInput(QQuickItem *item)
{
    if (item->inherits("QQuickTextInput")) {
        return item;
    }
    const QList<QQuickItem *> children = item->childItems();
    for (QQuickItem *child : children) {
        if (QQuickItem *found = findTextInput(child)) {
            return found;
        }
    }
    return nullptr;
}

}

class TestCheatsheetQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        messages.clear();
        qInstallMessageHandler(collectMessages);
        m_engine = std::make_unique<QQmlEngine>();
        QQmlComponent component(m_engine.get(), QUrl::fromLocalFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/src/cheatsheet/Cheatsheet.qml")));
        m_window.reset(qobject_cast<QQuickWindow *>(component.create()));
        QVERIFY2(m_window, qPrintable(component.errorString()));
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
    }

    void cleanup()
    {
        m_window.reset();
        m_engine.reset();
        qInstallMessageHandler(nullptr);
        QVERIFY2(messages.isEmpty(), qPrintable(messages.join(QLatin1Char('\n'))));
    }

    void showsEverySectionEntryAndKey()
    {
        const QStringList shown = texts();
        for (const QString &text : {QStringLiteral("Keyboard Shortcuts"), QStringLiteral("Focus"), QStringLiteral("KDE Windows & Desktops"),
                 QStringLiteral("Focus column left"), QStringLiteral("Zoom In"), QStringLiteral("Peek at Desktop"), QStringLiteral("Mod"),
                 QStringLiteral("H"), QStringLiteral("Left"), QStringLiteral("D"), QStringLiteral("Esc or Super+K to close")}) {
            QVERIFY2(shown.contains(text), qPrintable(text));
        }
        QVERIFY(!shown.contains(QStringLiteral("Meta")));
        QCOMPARE(shown.count(QStringLiteral("Super")), 2);
    }

    void keepsThePlusKey() { QVERIFY(texts().contains(QStringLiteral("+"))); }

    void searchFiltersEntriesAndSections()
    {
        QQuickItem *search = findTextInput(m_window->contentItem());
        QVERIFY(search);
        search->setProperty("text", QStringLiteral("  PEEK "));
        QStringList shown = texts();
        QVERIFY(shown.contains(QStringLiteral("Peek at Desktop")));
        QVERIFY(!shown.contains(QStringLiteral("Zoom In")));
        QVERIFY(!shown.contains(QStringLiteral("Focus")));
        search->setProperty("text", QStringLiteral("mod+left"));
        shown = texts();
        QVERIFY(shown.contains(QStringLiteral("Focus column left")));
        QVERIFY(!shown.contains(QStringLiteral("Peek at Desktop")));
        search->setProperty("text", QStringLiteral("kde windows"));
        shown = texts();
        QVERIFY(shown.contains(QStringLiteral("Zoom In")));
        QVERIFY(shown.contains(QStringLiteral("Peek at Desktop")));
        QVERIFY(!shown.contains(QStringLiteral("Focus column left")));
    }

    void escapeQuits()
    {
        QSignalSpy quit(m_engine.get(), &QQmlEngine::quit);
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
        QTest::keyClick(m_window.get(), Qt::Key_Escape);
        QCOMPARE(quit.count(), 1);
    }

    void viewerNeedsAFile() { QCOMPARE(runViewer({}).first, 2); }

    void viewerFailsOnABrokenFile()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("Broken.qml"));
        QVERIFY(write(path, "import QtQuick\nItem {\n"));
        QCOMPARE(runViewer({path}).first, 1);
        QCOMPARE(runViewer({directory.filePath(QStringLiteral("Missing.qml"))}).first, 1);
    }

    void viewerPassesItsArgumentsOn()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("Echo.qml"));
        QVERIFY(write(path,
            "import QtQuick\nQtObject { Component.onCompleted: { console.warn(\"args:\" + Qt.application.arguments.slice(2).join(\"|\")); "
            "Qt.quit() } }\n"));
        const auto [code, output] = runViewer({path, QStringLiteral("[1, 2]"), QStringLiteral("two words")});
        QCOMPARE(code, 0);
        QVERIFY2(output.contains("args:[1, 2]|two words"), output.constData());
    }

private:
    QStringList texts() const
    {
        QStringList result;
        collectTexts(m_window->contentItem(), result);
        return result;
    }

    static bool write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static std::pair<int, QByteArray> runViewer(const QStringList &arguments)
    {
        QProcess viewer;
        viewer.setProcessChannelMode(QProcess::MergedChannels);
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("QT_FORCE_STDERR_LOGGING"), QStringLiteral("1"));
        viewer.setProcessEnvironment(environment);
        viewer.start(QStringLiteral(KONVEYOR_CHEATSHEET_VIEWER), arguments);
        if (!viewer.waitForFinished(30000)) {
            viewer.kill();
            viewer.waitForFinished();
            return {-1, viewer.readAll()};
        }
        return {viewer.exitCode(), viewer.readAll()};
    }

    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QQuickWindow> m_window;
};

int main(int argc, char *argv[])
{
    QList<QByteArray> storage;
    for (int index = 0; index < argc; ++index) {
        storage.append(QByteArray(argv[index]));
    }
    storage.append(QByteArray(sectionsJson));
    QList<char *> arguments;
    for (QByteArray &argument : storage) {
        arguments.append(argument.data());
    }
    int count = int(arguments.size());
    QGuiApplication application(count, arguments.data());
    TestCheatsheetQml test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_cheatsheet_qml.moc"
