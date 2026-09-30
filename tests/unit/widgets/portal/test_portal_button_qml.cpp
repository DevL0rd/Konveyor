#include "portalharness.h"

#include <QTest>

using Konveyor::Test::FakePlasmoidAttached;
using Konveyor::Test::PortalHarness;
using Konveyor::Test::portalWarnings;

namespace
{

const QString busctl = QStringLiteral("busctl --user call org.devl0rd.KontrolPanel /KontrolPanel org.devl0rd.KontrolPanel ");

}

class TestPortalButtonQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_harness = std::make_unique<PortalHarness>();
        QVERIFY(m_harness->load(QStringLiteral("org.devl0rd.portal.launcher"), PortalHarness::Horizontal));
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

    void cleanup() { m_harness.reset(); }

    void showsTheKontrolPanelIcon()
    {
        QCOMPARE(FakePlasmoidAttached::instance().icon, QStringLiteral("start-here-kde-plasma-symbolic"));
        QCOMPARE(FakePlasmoidAttached::instance().title, QStringLiteral("Kontrol Panel"));
        QVERIFY(!root()->property("activationTogglesExpanded").toBool());
        QCOMPARE(
            root()->property("preferredRepresentation").value<QObject *>(), root()->property("compactRepresentation").value<QObject *>());
        m_harness->config()->insert(QStringLiteral("icon"), QStringLiteral("go-home"));
        QCOMPARE(FakePlasmoidAttached::instance().icon, QStringLiteral("go-home"));
    }

    void neverOpensAPopup()
    {
        root()->setProperty("expanded", true);
        QVERIFY(!root()->property("expanded").toBool());
    }

    void clickingOrActivatingTogglesThePanel()
    {
        m_harness->call("toggle");
        Q_EMIT FakePlasmoidAttached::instance().activated();
        const QStringList requested = m_harness->requested();
        QCOMPARE(requested.size(), 2);
        for (const QString &command : requested) {
            QVERIFY2(command.startsWith(busctl + QStringLiteral("Toggle # ")), qPrintable(command));
        }
    }

    void droppedDesktopFilesArePinned()
    {
        const QVariantList urls {QStringLiteral("file:///usr/share/applications/org.kde.konsole.desktop"),
            QStringLiteral("file:///home/u/My%20App's.desktop"), QStringLiteral("file:///home/u/notes.txt")};
        QVERIFY(m_harness->call("pinFiles", urls).toBool());
        const QString command = m_harness->requested().value(0);
        QVERIFY2(command.startsWith(busctl
                     + QStringLiteral("Pin as 2 '/usr/share/applications/org.kde.konsole.desktop' '/home/u/My App'\\''s.desktop' # ")),
            qPrintable(command));
    }

    void droppingNoDesktopFileDoesNothing()
    {
        QVERIFY(!m_harness->call("pinFiles", QVariantList {QStringLiteral("file:///tmp/a.txt")}).toBool());
        QVERIFY(m_harness->requested().isEmpty());
    }

    void aFailedCallShowsAnErrorUntilOneWorks()
    {
        m_harness->call("toggle");
        QObject *panel = m_harness->dataSources().first();
        const QString command = m_harness->requested().first();
        const QVariantMap failed {{QStringLiteral("exit code"), 1}, {QStringLiteral("stderr"), QStringLiteral("Unit not found.\n")}};
        QVERIFY(QMetaObject::invokeMethod(panel, "newData", Q_ARG(QString, command), Q_ARG(QVariant, QVariant(failed))));
        QCOMPARE(FakePlasmoidAttached::instance().icon, QStringLiteral("dialog-error"));
        QCOMPARE(root()->property("toolTipSubText").toString(), QStringLiteral("Could not open the Kontrol Panel: Unit not found."));
        QVERIFY(panel->property("connectedSources").toStringList().isEmpty());

        const QVariantMap silent {{QStringLiteral("exit code"), 1}};
        QVERIFY(QMetaObject::invokeMethod(panel, "newData", Q_ARG(QString, command), Q_ARG(QVariant, QVariant(silent))));
        QCOMPARE(root()->property("failure").toString(), QStringLiteral("The Kontrol Panel service did not answer"));

        QVERIFY(PortalHarness::answer(panel, command, QString()));
        QCOMPARE(root()->property("failure").toString(), QString());
        QCOMPARE(FakePlasmoidAttached::instance().icon, QStringLiteral("start-here-kde-plasma-symbolic"));
    }

    void theLabelShowsOnlyWhenSetAndHorizontal_data()
    {
        QTest::addColumn<int>("formFactor");
        QTest::addColumn<bool>("showLabel");
        QTest::addColumn<QString>("label");
        QTest::addColumn<bool>("shown");
        QTest::newRow("shown") << int(PortalHarness::Horizontal) << true << "Start" << true;
        QTest::newRow("switched off") << int(PortalHarness::Horizontal) << false << "Start" << false;
        QTest::newRow("empty") << int(PortalHarness::Horizontal) << true << "" << false;
        QTest::newRow("vertical panel") << int(PortalHarness::Vertical) << true << "Start" << false;
    }

    void theLabelShowsOnlyWhenSetAndHorizontal()
    {
        QFETCH(int, formFactor);
        QFETCH(bool, showLabel);
        QFETCH(QString, label);
        QFETCH(bool, shown);
        m_harness = std::make_unique<PortalHarness>();
        QVERIFY(m_harness->load(QStringLiteral("org.devl0rd.portal.launcher"), PortalHarness::FormFactor(formFactor),
            {{QStringLiteral("showLabel"), showLabel}, {QStringLiteral("label"), label}}));
        auto *component = root()->property("compactRepresentation").value<QQmlComponent *>();
        std::unique_ptr<QObject> button(component->create(qmlContext(root())));
        QVERIFY2(button, qPrintable(component->errorString()));
        QCOMPARE(button->property("showLabel").toBool(), shown);
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

private:
    QObject *root() const { return m_harness->root(); }

    std::unique_ptr<PortalHarness> m_harness;
};

QTEST_MAIN(TestPortalButtonQml)
#include "test_portal_button_qml.moc"
