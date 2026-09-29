#include "kontrolpanelservice.h"

#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KGlobalAccel>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

namespace
{

void registerToggleShortcut(QAction &action, Konveyor::KontrolPanelService &service)
{
    action.setObjectName(QStringLiteral("toggle"));
    action.setText(QStringLiteral("Open or close the Kontrol Panel"));
    action.setProperty("componentName", QStringLiteral("konveyor-kontrol-panel"));
    action.setProperty("componentDisplayName", QStringLiteral("Kontrol Panel"));
    QObject::connect(&action, &QAction::triggered, &service, &Konveyor::KontrolPanelService::Toggle);
    KGlobalAccel::setGlobalShortcut(&action, QList<QKeySequence> {QKeySequence(Qt::Key_Meta), QKeySequence(Qt::ALT | Qt::Key_F1)});
}

}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("konveyor-kontrol-panel"));
    application.setDesktopFileName(QStringLiteral("org.devl0rd.KontrolPanel"));
    application.setQuitOnLastWindowClosed(false);
    application.setQuitLockEnabled(false);
    const QStringList arguments = QApplication::arguments();
    if (arguments.size() != 2) {
        qCritical("usage: konveyor-kontrol-panel DIRECTORY");
        return 2;
    }
    const QDir directory(arguments.at(1));
    QFile schema(directory.filePath(QStringLiteral("config/main.xml")));
    if (!schema.exists()) {
        qCritical("konveyor-kontrol-panel: cannot read %s", qPrintable(schema.fileName()));
        return 1;
    }
    KLocalizedString::setApplicationDomain("plasma_applet_org.devl0rd.portal.launcher");

    Konveyor::KontrolPanelService service;
    if (!service.registerOn(QDBusConnection::sessionBus())) {
        qCritical("konveyor-kontrol-panel: could not register %s on the session bus; is it already running?",
            Konveyor::KontrolPanelService::serviceName);
        return 1;
    }

    QAction toggle;
    registerToggleShortcut(toggle, service);

    KConfigLoader loader(KSharedConfig::openConfig(QStringLiteral("konveyor/kontrolpanelrc")), &schema);
    KConfigPropertyMap config(&loader);
    config.setNotify(true);
    QObject::connect(&config, &QQmlPropertyMap::valueChanged, &config, [&config] { config.writeConfig(); });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedQmlContext(&engine));
    engine.setInitialProperties(
        {{QStringLiteral("service"), QVariant::fromValue(&service)}, {QStringLiteral("config"), QVariant::fromValue(&config)}});
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl::fromLocalFile(directory.filePath(QStringLiteral("Main.qml"))));
    return QApplication::exec();
}
