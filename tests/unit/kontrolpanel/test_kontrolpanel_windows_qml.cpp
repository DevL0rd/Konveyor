#include "kontrolpanelservice.h"

#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>

#include <memory>

using Konveyor::KontrolPanelService;

namespace
{

QStringList warnings;

void collectWarnings(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (type != QtDebugMsg && type != QtInfoMsg && !message.contains(QLatin1String("is not a wayland window"))
        && message != QLatin1String("Could not find any platform plugin")) {
        warnings.append(message);
    }
}

bool write(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

const QByteArray rootQml = R"(import QtQuick
Item {
    id: root
    required property var config
    required property QtObject service
    property bool open: true
    property string currentPage: ""
    property int hides: 0
    signal pageRequested(string page)
    function hide() { hides++; open = false }
    property Overlay overlay: Overlay {}
    property Component settings: Component { ConfigWindow {} }
}
)";

const QByteArray launcherViewQml = R"(import QtQuick
Item {
    objectName: "launcherView"
    signal activateRequested()
    signal closeFinished()
    property string page: "home"
    property real progress: 1
    property bool hadFocus: false
    property bool shown: true
    property bool menuOpen: false
    function goToPage(name) { page = name }
}
)";

const QByteArray configGeneralQml = R"(import QtQuick
Item {
    objectName: "configGeneral"
    property bool portal: true
    property int cfg_tileSize
    property string cfg_defaultPage
    property bool cfg_searchWeb
}
)";

QWindow *windowTitled(const QString &title)
{
    const QWindowList windows = QGuiApplication::topLevelWindows();
    for (QWindow *window : windows) {
        if (window->title() == title) {
            return window;
        }
    }
    return nullptr;
}

}

class TestKontrolPanelWindowsQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { KLocalizedString::setApplicationDomain("plasma_applet_org.devl0rd.portal.launcher"); }

    void init()
    {
        warnings.clear();
        m_previousHandler = qInstallMessageHandler(collectWarnings);
        m_directory = std::make_unique<QTemporaryDir>();
        const QDir directory(m_directory->path());
        for (const QString &name : {QStringLiteral("Overlay.qml"), QStringLiteral("ConfigWindow.qml")}) {
            QVERIFY(QFile::copy(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/kontrol-panel/") + name, directory.filePath(name)));
        }
        QVERIFY(write(directory.filePath(QStringLiteral("Root.qml")), rootQml));
        QVERIFY(write(directory.filePath(QStringLiteral("LauncherView.qml")), launcherViewQml));
        QVERIFY(write(directory.filePath(QStringLiteral("configGeneral.qml")), configGeneralQml));
        m_config = std::make_unique<QQmlPropertyMap>();
        m_config->insert(QStringLiteral("cardWidth"), 90);
        m_config->insert(QStringLiteral("cardHeight"), 56);
        m_config->insert(QStringLiteral("dimStrength"), 0.55);
        m_config->insert(QStringLiteral("tileSize"), 52);
        m_config->insert(QStringLiteral("defaultPage"), QStringLiteral("home"));
        m_config->insert(QStringLiteral("searchWeb"), false);
        m_config->insert(QStringLiteral("notInTheForm"), QStringLiteral("kept"));
        m_service = std::make_unique<KontrolPanelService>();
        m_engine = std::make_unique<QQmlEngine>();
        m_engine->rootContext()->setContextObject(new KLocalizedQmlContext(m_engine.get()));
        QQmlComponent component(m_engine.get(), QUrl::fromLocalFile(directory.filePath(QStringLiteral("Root.qml"))));
        m_root.reset(component.createWithInitialProperties({{QStringLiteral("config"), QVariant::fromValue<QObject *>(m_config.get())},
            {QStringLiteral("service"), QVariant::fromValue<QObject *>(m_service.get())}}));
        QVERIFY2(m_root, qPrintable(component.errorString()));
        m_view = findView();
        QVERIFY(m_view);
    }

    void cleanup()
    {
        m_settings.reset();
        m_root.reset();
        m_engine.reset();
        m_service.reset();
        m_config.reset();
        m_directory.reset();
        qInstallMessageHandler(m_previousHandler);
    }

    void overlayStartsHidden()
    {
        QVERIFY(!windowTitled(QStringLiteral("Kontrol Panel backdrop"))->isVisible());
        QVERIFY(!windowTitled(QStringLiteral("Kontrol Panel"))->isVisible());
        QCOMPARE(warnings.join(QLatin1Char('\n')), QString());
    }

    void activatingShowsTheBackdropThenTheCard()
    {
        QVERIFY(QMetaObject::invokeMethod(m_view, "activateRequested"));
        QVERIFY(windowTitled(QStringLiteral("Kontrol Panel backdrop"))->isVisible());
        QVERIFY(!windowTitled(QStringLiteral("Kontrol Panel"))->isVisible());
        QCoreApplication::processEvents();
        QVERIFY(windowTitled(QStringLiteral("Kontrol Panel"))->isVisible());
        QVERIFY(QMetaObject::invokeMethod(m_view, "closeFinished"));
        QVERIFY(!windowTitled(QStringLiteral("Kontrol Panel backdrop"))->isVisible());
        QVERIFY(!windowTitled(QStringLiteral("Kontrol Panel"))->isVisible());
        QCOMPARE(warnings.join(QLatin1Char('\n')), QString());
    }

    void clickingTheBackdropCloses()
    {
        QVERIFY(QMetaObject::invokeMethod(m_view, "activateRequested"));
        QWindow *backdrop = windowTitled(QStringLiteral("Kontrol Panel backdrop"));
        backdrop->resize(200, 200);
        QVERIFY(QTest::qWaitForWindowExposed(backdrop));
        QTest::mouseClick(backdrop, Qt::RightButton, {}, QPoint(5, 5));
        QCOMPARE(m_root->property("hides").toInt(), 1);
    }

    void pageRequestsReachTheViewAndPageChangesReachTheRoot()
    {
        QCOMPARE(m_root->property("currentPage").toString(), QStringLiteral("home"));
        QVERIFY(QMetaObject::invokeMethod(m_root.get(), "pageRequested", Q_ARG(QString, QStringLiteral("games"))));
        QCOMPARE(m_view->property("page").toString(), QStringLiteral("games"));
        QCOMPARE(m_root->property("currentPage").toString(), QStringLiteral("games"));
    }

    void cardSizeFollowsTheConfigAndFitsTheScreen()
    {
        QObject *overlay = m_root->property("overlay").value<QObject *>();
        m_config->insert(QStringLiteral("cardWidth"), 10);
        m_config->insert(QStringLiteral("cardHeight"), 5);
        const int gridUnit = overlay->property("cardWidth").toInt() / 10;
        QVERIFY(gridUnit > 0);
        QCOMPARE(overlay->property("cardHeight").toInt(), gridUnit * 5);
        const QSize screen = overlay->property("screenSize").toSize();
        m_config->insert(QStringLiteral("cardWidth"), 10000);
        m_config->insert(QStringLiteral("cardHeight"), 10000);
        QCOMPARE(overlay->property("cardWidth").toInt(), screen.width() - gridUnit * 6);
        QCOMPARE(overlay->property("cardHeight").toInt(), screen.height() - gridUnit * 5);
    }

    void losingFocusAfterHavingItCloses()
    {
        QVERIFY(QMetaObject::invokeMethod(m_view, "activateRequested"));
        QWindow *card = windowTitled(QStringLiteral("Kontrol Panel"));
        QVERIFY(QTest::qWaitForWindowActive(card));
        QVERIFY(m_view->property("hadFocus").toBool());
        QWindow other;
        m_view->setProperty("menuOpen", true);
        other.show();
        other.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&other));
        QCOMPARE(m_root->property("hides").toInt(), 0);
        m_view->setProperty("menuOpen", false);
        card->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(card));
        other.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&other));
        QCOMPARE(m_root->property("hides").toInt(), 1);
    }

    void settingsFormStartsFromTheConfig()
    {
        QObject *form = openSettings();
        QCOMPARE(form->property("portal").toBool(), false);
        QCOMPARE(form->property("cfg_tileSize").toInt(), 52);
        QCOMPARE(form->property("cfg_defaultPage").toString(), QStringLiteral("home"));
        QCOMPARE(warnings.join(QLatin1Char('\n')), QString());
    }

    void applyWritesOnlyChangedFields()
    {
        QObject *form = openSettings();
        form->setProperty("cfg_tileSize", 64);
        form->setProperty("cfg_searchWeb", true);
        QVERIFY(QMetaObject::invokeMethod(m_settings.get(), "apply"));
        QCOMPARE(m_config->value(QStringLiteral("tileSize")).toInt(), 64);
        QCOMPARE(m_config->value(QStringLiteral("searchWeb")).toBool(), true);
        QCOMPARE(m_config->value(QStringLiteral("defaultPage")).toString(), QStringLiteral("home"));
        QCOMPARE(m_config->value(QStringLiteral("notInTheForm")).toString(), QStringLiteral("kept"));
    }

    void theButtonsApplyOrDiscard()
    {
        QObject *form = openSettings();
        QObject *buttons = dialogButtons();
        QVERIFY(buttons);
        form->setProperty("cfg_tileSize", 60);
        QVERIFY(QMetaObject::invokeMethod(buttons, "applied"));
        QCOMPARE(m_config->value(QStringLiteral("tileSize")).toInt(), 60);
        form->setProperty("cfg_tileSize", 99);
        QVERIFY(QMetaObject::invokeMethod(buttons, "rejected"));
        QCOMPARE(m_config->value(QStringLiteral("tileSize")).toInt(), 60);
        QVERIFY(!m_settings->property("visible").toBool());

        form = openSettings();
        form->setProperty("cfg_defaultPage", QStringLiteral("games"));
        QVERIFY(QMetaObject::invokeMethod(dialogButtons(), "accepted"));
        QCOMPARE(m_config->value(QStringLiteral("defaultPage")).toString(), QStringLiteral("games"));
        QVERIFY(!m_settings->property("visible").toBool());
        QCOMPARE(warnings.join(QLatin1Char('\n')), QString());
    }

private:
    QObject *findView() const
    {
        const QWindowList windows = QGuiApplication::topLevelWindows();
        for (QWindow *window : windows) {
            if (QObject *view = window->findChild<QObject *>(QStringLiteral("launcherView"))) {
                return view;
            }
        }
        return nullptr;
    }

    QObject *dialogButtons() const
    {
        const QList<QObject *> children = m_settings->findChildren<QObject *>();
        for (QObject *child : children) {
            if (child->metaObject()->indexOfSignal("applied()") >= 0) {
                return child;
            }
        }
        return nullptr;
    }

    QObject *openSettings()
    {
        auto *component = m_root->property("settings").value<QQmlComponent *>();
        m_settings.reset(component->create(qmlContext(m_root.get())));
        return m_settings ? m_settings->findChild<QObject *>(QStringLiteral("configGeneral")) : nullptr;
    }

    QtMessageHandler m_previousHandler = nullptr;
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<QQmlPropertyMap> m_config;
    std::unique_ptr<KontrolPanelService> m_service;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_root;
    std::unique_ptr<QObject> m_settings;
    QObject *m_view = nullptr;
};

QTEST_MAIN(TestKontrolPanelWindowsQml)
#include "test_kontrolpanel_windows_qml.moc"
