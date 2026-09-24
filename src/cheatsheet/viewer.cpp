#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    const QStringList arguments = QGuiApplication::arguments();
    if (arguments.size() < 2) {
        qCritical("usage: konveyor-cheatsheet-viewer FILE.qml [ARGUMENT...]");
        return 2;
    }
    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl::fromLocalFile(arguments.at(1)));
    return QGuiApplication::exec();
}
