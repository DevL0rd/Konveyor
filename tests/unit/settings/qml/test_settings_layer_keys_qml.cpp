#include "settingsqmlharness.h"

using namespace Konveyor::Settings::Testing;

class TestSettingsLayerKeysQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void layerKeys_data();
    void layerKeys();
    void reduceMotionSwitch();
    void eventEffectsWrite();

private:
    struct Session
    {
        std::unique_ptr<QQmlEngine> engine = std::make_unique<QQmlEngine>();
        std::unique_ptr<QObject> host;
        QObject *store = nullptr;
    };

    Session open(const QString &text)
    {
        m_home.resetConfig(text);
        Session session;
        QString error;
        session.host = m_home.create(*session.engine, ScriptHost.toByteArray(), &error);
        session.store = SettingsHome::store(*session.engine);
        return session;
    }

    QString saved(QObject *store) { return saveAndWait(store) ? SettingsHome::read(m_home.configPath()) : QStringLiteral("<not saved>"); }

    SettingsHome m_home;
};

void TestSettingsLayerKeysQml::layerKeys_data()
{
    QTest::addColumn<QString>("layer");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("expected");

    QTest::newRow("baseline") << "bass-thump" << "baseline" << QVariant(0.25) << "baseline 0.25";
    QTest::newRow("burst") << "particles" << "burst" << QVariant(40.0) << "burst 40";
    QTest::newRow("dash") << "dashes" << "dash" << QVariant(24.0) << "dash 24";
    QTest::newRow("direction") << "particles" << "direction" << QVariant(QStringLiteral("up")) << "direction \"up\"";
    QTest::newRow("lifetime") << "particles" << "lifetime" << QVariant(2.5) << "lifetime 2.5";
    QTest::newRow("rate") << "particles" << "rate" << QVariant(90.0) << "rate 90";
    QTest::newRow("sensitivity") << "elastic" << "sensitivity" << QVariant(3.5) << "sensitivity 3.5";
    QTest::newRow("smoothing") << "elastic" << "smoothing" << QVariant(0.4) << "smoothing 0.4";
    QTest::newRow("tail") << "comet" << "tail" << QVariant(0.6) << "tail 0.6";
    QTest::newRow("thickness") << "elastic" << "thickness" << QVariant(7.5) << "thickness 7.5";
}

void TestSettingsLayerKeysQml::layerKeys()
{
    QFETCH(QString, layer);
    QFETCH(QString, key);
    QFETCH(QVariant, value);
    QFETCH(QString, expected);
    Session session
        = open(QStringLiteral("border-style \"Mine\" {\n    layer \"%1\" {\n        colors \"#ffffff\"\n    }\n}\n").arg(layer));
    QVERIFY(call<bool>(session.store, "setValue", QStringLiteral("border-style#0/layer#0/") + key, QVariantList {value}, QVariantMap {}));
    const QString text = saved(session.store);
    QVERIFY2(text.contains(expected), qPrintable(text));
    QVERIFY(text.contains(QStringLiteral("layer \"%1\"").arg(layer)));
}

void TestSettingsLayerKeysQml::reduceMotionSwitch()
{
    Session session = open(QStringLiteral("animations {\n}\n"));
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("animations/reduce-motion"), true));
    QVERIFY2(saved(session.store).contains(QStringLiteral("reduce-motion")), "switch on");
    QVERIFY(call<bool>(session.store, "setFlag", QStringLiteral("animations/reduce-motion"), false));
    QVERIFY(!saved(session.store).contains(QStringLiteral("reduce-motion")));
}

void TestSettingsLayerKeysQml::eventEffectsWrite()
{
    Session session = open(QStringLiteral("layout {\n}\nborder-style \"Pulse\" {\n    layer \"glow\"\n}\n"));
    const QVariantMap value {{QStringLiteral("style"), QStringLiteral("Pulse")}, {QStringLiteral("reach"), QStringLiteral("all")},
        {QStringLiteral("mode"), QStringLiteral("once")}, {QStringLiteral("duration"), 0}};
    QVERIFY(script(
        session.host.get(), "EventEffects", "write", {QStringLiteral("@store"), QStringLiteral("layout"), QStringLiteral("scroll"), value})
            .toBool());
    const QString text = saved(session.store);
    QVERIFY2(text.contains(QStringLiteral("event-effects")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("on \"scroll\" \"Pulse\"")), qPrintable(text));
    QVERIFY(text.contains(QStringLiteral("reach=\"all\"")));
}

QTEST_MAIN(TestSettingsLayerKeysQml)
#include "test_settings_layer_keys_qml.moc"
