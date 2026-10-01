#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (argc != 5) {
        return 1;
    }
    app.setDesktopFileName(QString::fromLocal8Bit(argv[1]));
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/tests/nested/clients/client.qml")));
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}
