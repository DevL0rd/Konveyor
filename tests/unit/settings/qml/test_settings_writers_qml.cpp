#include "settingsqmlharness.h"

#include <QColor>
#include <QRegularExpression>

using namespace Konveyor::Settings::Testing;

namespace
{

const QString Store = QStringLiteral("@store");

QVariantMap paint(const QString &source, const QString &color = QString())
{
    return {{QStringLiteral("source"), source}, {QStringLiteral("color"), color}};
}

}

class TestSettingsWritersQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void writeFlagEveryFlag_data();
    void writeFlagEveryFlag();
    void flagDefaultsMatchRuntime();
    void writesEveryLayoutKey_data();
    void writesEveryLayoutKey();
    void sameShapeBlocksKeepComments();
    void paintAlphaIsEightBit_data();
    void paintAlphaIsEightBit();

private:
    struct Session
    {
        std::unique_ptr<QQmlEngine> engine = std::make_unique<QQmlEngine>();
        std::unique_ptr<QObject> host;
        QObject *store = nullptr;
    };

    Session open(const std::optional<QString> &text)
    {
        m_home.resetConfig(text);
        Session session;
        QString error;
        session.host = m_home.create(*session.engine, ScriptHost.toByteArray(), &error);
        if (!session.host) {
            qWarning("%s", qPrintable(error));
        }
        session.store = SettingsHome::store(*session.engine);
        return session;
    }

    QString saved(QObject *store) { return saveAndWait(store) ? SettingsHome::read(m_home.configPath()) : QStringLiteral("<not saved>"); }

    SettingsHome m_home;
};

void TestSettingsWritersQml::writeFlagEveryFlag_data()
{
    QTest::addColumn<QString>("block");
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("runtimeDefault");
    QTest::addColumn<bool>("overrideMode");
    QTest::addColumn<bool>("on");
    QTest::addColumn<bool>("shipped");
    const QList<std::tuple<QString, QString, bool>> flags {
        {QString(), QStringLiteral("float-child-windows"), false},
        {QString(), QStringLiteral("always-center-single-column"), false},
        {QString(), QStringLiteral("always-expand-single-column"), true},
        {QString(), QStringLiteral("remember-window-sizes"), false},
        {QString(), QStringLiteral("remember-window-positions"), false},
        {QStringLiteral("tab-indicator"), QStringLiteral("hide-when-single-tab"), false},
        {QStringLiteral("tab-indicator"), QStringLiteral("place-within-column"), false},
    };
    for (const auto &[block, key, runtimeDefault] : flags) {
        for (const bool overrideMode : {false, true}) {
            for (const bool on : {false, true}) {
                for (const bool shipped : {false, true}) {
                    if (overrideMode && (key.startsWith(QLatin1String("remember")) || key == QLatin1String("float-child-windows"))) {
                        continue;
                    }
                    QTest::addRow("%s %s %s %s", qPrintable(key), overrideMode ? "override" : "global", on ? "on" : "off",
                        shipped ? "shipped" : "empty")
                        << block << key << runtimeDefault << overrideMode << on << shipped;
                }
            }
        }
    }
}

void TestSettingsWritersQml::writeFlagEveryFlag()
{
    QFETCH(QString, block);
    QFETCH(QString, key);
    QFETCH(bool, runtimeDefault);
    QFETCH(bool, overrideMode);
    QFETCH(bool, on);
    QFETCH(bool, shipped);
    if (!shipped) {
        QFile::remove(m_home.dataPath());
    }
    Session session = open(QStringLiteral("layout {\n}\noutput \"DP-1\"\n"));
    QVERIFY(session.host);
    const QString layoutPath = overrideMode ? QStringLiteral("output/layout") : QStringLiteral("layout");
    const QString scopePath = block.isEmpty() ? layoutPath : layoutPath + QLatin1Char('/') + block;
    const QString path = scopePath + QLatin1Char('/') + key;
    QVERIFY(script(session.host.get(), QStringLiteral("LayoutKeys"), QStringLiteral("writeFlag"),
        {Store, scopePath, overrideMode, key, on, runtimeDefault})
            .toBool());
    const QVariantMap scoped = call<QVariantMap>(session.store, "scope", layoutPath);
    const QVariantMap values = block.isEmpty() ? scoped : scoped.value(block).toMap();
    QCOMPARE(values.value(key).toBool(), on);
    const QVariantMap node = call<QVariantMap>(session.store, "node", path);
    if (overrideMode) {
        QCOMPARE(node.value(QStringLiteral("args")).toList(), on ? QVariantList {} : QVariantList {false});
    } else {
        const QVariantMap shippedNode = call<QVariantMap>(session.store, "defaultNode", path);
        const bool fresh
            = shippedNode.isEmpty() ? runtimeDefault : shippedNode.value(QStringLiteral("args")).toList().value(0, true).toBool();
        QCOMPARE(call<bool>(session.store, "isDefault", path), on == fresh);
        QCOMPARE(node.isEmpty(), on == fresh && shippedNode.isEmpty());
    }
    QVERIFY(m_home.installDefaults());
}

void TestSettingsWritersQml::flagDefaultsMatchRuntime()
{
    Session session = open(QString());
    const QVariantMap runtime = call<QVariantMap>(session.store, "scope", QStringLiteral("layout"));
    const QRegularExpression call(
        QString::fromLatin1(R"re(LayoutKeys\.writeFlag\(SettingsStore, [^,]+, [^,]+, "([a-z-]+)", on(?:, (true|false))?\))re"));
    int checked = 0;
    for (const QString &file : {QStringLiteral("sections/PlacementSection.qml"), QStringLiteral("sections/TabIndicatorSection.qml")}) {
        const QString source = SettingsHome::read(m_home.sourcePath(file));
        for (const QRegularExpressionMatch &match : call.globalMatch(source)) {
            QVERIFY2(!match.captured(2).isEmpty(), qPrintable(match.captured(0)));
            QVERIFY2(runtime.contains(match.captured(1)), qPrintable(match.captured(1)));
            QCOMPARE(match.captured(2) == QLatin1String("true"), runtime.value(match.captured(1)).toBool());
            ++checked;
        }
    }
    QCOMPARE(checked, 5);
    const QVariantMap tab = runtime.value(QStringLiteral("tab-indicator")).toMap();
    QCOMPARE(tab.value(QStringLiteral("hide-when-single-tab")).toBool(), false);
    QCOMPARE(tab.value(QStringLiteral("place-within-column")).toBool(), false);
    QVERIFY(SettingsHome::read(m_home.sourcePath(QStringLiteral("sections/TabIndicatorSection.qml")))
            .contains(QStringLiteral("LayoutKeys.writeFlag(SettingsStore, blockPath, overrideMode, name, on, false)")));
}

void TestSettingsWritersQml::writesEveryLayoutKey_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("written");
    const auto size
        = [](const QString &kind, double value) { return QVariantMap {{QStringLiteral("kind"), kind}, {QStringLiteral("value"), value}}; };
    QTest::newRow("gaps") << QStringLiteral("gaps") << QVariant(12.0) << QStringLiteral("gaps 12");
    QTest::newRow("center") << QStringLiteral("center-focused-column") << QVariant(QStringLiteral("on-overflow"))
                            << QStringLiteral("center-focused-column \"on-overflow\"");
    QTest::newRow("new column") << QStringLiteral("new-column-position") << QVariant(QStringLiteral("left"))
                                << QStringLiteral("new-column-position \"left\"");
    QTest::newRow("placement") << QStringLiteral("new-window-placement") << QVariant(QStringLiteral("stack"))
                               << QStringLiteral("new-window-placement \"stack\"");
    QTest::newRow("group") << QStringLiteral("group-app-windows") << QVariant(QStringLiteral("off"))
                           << QStringLiteral("group-app-windows \"off\"");
    QTest::newRow("rows") << QStringLiteral("max-rows-per-column") << QVariant(4) << QStringLiteral("max-rows-per-column 4");
    QTest::newRow("display") << QStringLiteral("default-column-display") << QVariant(QStringLiteral("tabbed"))
                             << QStringLiteral("default-column-display \"tabbed\"");
    QTest::newRow("widths") << QStringLiteral("preset-column-widths")
                            << QVariant(QVariantList {size(QStringLiteral("proportion"), 1.0 / 3), size(QStringLiteral("fixed"), 640.5)})
                            << QStringLiteral("preset-column-widths {\n        proportion 0.33333\n        fixed 641\n    }");
    QTest::newRow("no heights") << QStringLiteral("preset-window-heights") << QVariant(QVariantList {})
                                << QStringLiteral("preset-window-heights {}");
    QTest::newRow("app decides") << QStringLiteral("default-column-width") << QVariant() << QStringLiteral("default-column-width {}");
    QTest::newRow("width") << QStringLiteral("default-column-width") << QVariant(size(QStringLiteral("proportion"), 0.5))
                           << QStringLiteral("default-column-width { proportion 0.5; }");
    QTest::newRow("struts") << QStringLiteral("struts")
                            << QVariant(QVariantMap {{QStringLiteral("left"), 10.6}, {QStringLiteral("right"), 0},
                                   {QStringLiteral("top"), -2.4}, {QStringLiteral("bottom"), 3.5}})
                            << QStringLiteral("struts {\n        left 11\n        right 0\n        top -2\n        bottom 4\n    }");
    const QVariantMap gradient {{QStringLiteral("from"), QStringLiteral("#ff000000")}, {QStringLiteral("to"), QStringLiteral("#80ffffff")},
        {QStringLiteral("angle"), 44.6}, {QStringLiteral("relative-to"), QStringLiteral("workspace-view")},
        {QStringLiteral("in"), QStringLiteral("oklch longer hue")}};
    QVariantMap gradientPaint = paint(QStringLiteral("color"));
    gradientPaint.insert(QStringLiteral("gradient"), gradient);
    QTest::newRow("focus ring")
        << QStringLiteral("focus-ring")
        << QVariant(QVariantMap {{QStringLiteral("enabled"), true}, {QStringLiteral("width"), 4},
               {QStringLiteral("active"), paint(QStringLiteral("accent"))},
               {QStringLiteral("inactive"), paint(QStringLiteral("color"), QStringLiteral("#80ff0000"))},
               {QStringLiteral("urgent"), QVariant()}})
        << QStringLiteral(
               "focus-ring {\n        on\n        width 4\n        active-color \"accent\"\n        inactive-color \"#ff000080\"\n    }");
    QTest::newRow("border")
        << QStringLiteral("border")
        << QVariant(
               QVariantMap {{QStringLiteral("enabled"), false}, {QStringLiteral("width"), 2}, {QStringLiteral("active"), gradientPaint}})
        << QStringLiteral(
               "border {\n        off\n        width 2\n        active-gradient angle=45 from=\"#000000\" in=\"oklch longer hue\" "
               "relative-to=\"workspace-view\" to=\"#ffffff80\"\n    }");
    QTest::newRow("tab indicator")
        << QStringLiteral("tab-indicator")
        << QVariant(QVariantMap {{QStringLiteral("enabled"), true}, {QStringLiteral("hide-when-single-tab"), true},
               {QStringLiteral("place-within-column"), false}, {QStringLiteral("position"), QStringLiteral("top")},
               {QStringLiteral("width"), 4}, {QStringLiteral("gap"), 5}, {QStringLiteral("length"), 0.5},
               {QStringLiteral("gaps-between-tabs"), 2}, {QStringLiteral("corner-radius"), 3},
               {QStringLiteral("active"), paint(QStringLiteral("hover"))}})
        << QStringLiteral("tab-indicator {\n        on\n        hide-when-single-tab true\n        place-within-column false\n        "
                          "position \"top\"\n"
                          "        width 4\n        gap 5\n        length total-proportion=0.5\n        gaps-between-tabs 2\n"
                          "        corner-radius 3\n        active-color \"hover\"\n    }");
    QTest::newRow("insert hint") << QStringLiteral("insert-hint")
                                 << QVariant(QVariantMap {{QStringLiteral("enabled"), false},
                                        {QStringLiteral("paint"), paint(QStringLiteral("color"), QStringLiteral("#7f7fc8ff"))}})
                                 << QStringLiteral("insert-hint {\n        off\n        color \"#7fc8ff7f\"\n    }");
    QTest::newRow("background") << QStringLiteral("background-color") << QVariant(QStringLiteral("#00000000"))
                                << QStringLiteral("background-color \"#00000000\"");
}

void TestSettingsWritersQml::writesEveryLayoutKey()
{
    QFETCH(QString, key);
    QFETCH(QVariant, value);
    QFETCH(QString, written);
    Session session = open(QStringLiteral("layout {\n}\n"));
    QVERIFY(script(session.host.get(), QStringLiteral("LayoutKeys"), QStringLiteral("write"), {Store, QStringLiteral("layout"), key, value})
            .toBool());
    QCOMPARE(saved(session.store), QStringLiteral("layout {\n    ") + written + QStringLiteral("\n}\n"));
    QVERIFY(session.store->property("configError").toString().isEmpty());
}

void TestSettingsWritersQml::sameShapeBlocksKeepComments()
{
    Session session = open(QStringLiteral(
        "layout {\n    focus-ring {\n        // mine\n        on\n        width 4 // thin\n        active-color \"accent\"\n    }\n"
        "    struts { left 1; right 2; top 3; bottom 4; }\n}\n"));
    const QVariantMap ring {
        {QStringLiteral("enabled"), true}, {QStringLiteral("width"), 6}, {QStringLiteral("active"), paint(QStringLiteral("focus"))}};
    QVERIFY(script(session.host.get(), QStringLiteral("LayoutKeys"), QStringLiteral("write"),
        {Store, QStringLiteral("layout"), QStringLiteral("focus-ring"), ring})
            .toBool());
    const QVariantMap struts {
        {QStringLiteral("left"), 5}, {QStringLiteral("right"), 2}, {QStringLiteral("top"), 3}, {QStringLiteral("bottom"), 4}};
    QVERIFY(script(session.host.get(), QStringLiteral("LayoutKeys"), QStringLiteral("write"),
        {Store, QStringLiteral("layout"), QStringLiteral("struts"), struts})
            .toBool());
    QCOMPARE(saved(session.store),
        QStringLiteral(
            "layout {\n    focus-ring {\n        // mine\n        on\n        width 6 // thin\n        active-color \"focus\"\n    }\n"
            "    struts { left 5; right 2; top 3; bottom 4; }\n}\n"));
}

void TestSettingsWritersQml::paintAlphaIsEightBit_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QString>("css");
    QTest::newRow("opaque") << QColor(255, 0, 0) << QStringLiteral("#ff0000");
    QTest::newRow("half") << QColor(255, 0, 0, 128) << QStringLiteral("#ff000080");
    QTest::newRow("clear") << QColor(1, 2, 3, 0) << QStringLiteral("#01020300");
    QTest::newRow("almost opaque") << QColor::fromRgbF(0, 0, 1, 0.9985F) << QStringLiteral("#0000ff");
    QTest::newRow("just below") << QColor::fromRgbF(0, 0, 1, 0.998F) << QStringLiteral("#0000fffe");
    QTest::newRow("254") << QColor(0, 0, 255, 254) << QStringLiteral("#0000fffe");
    QTest::newRow("one") << QColor(0, 0, 255, 1) << QStringLiteral("#0000ff01");
}

void TestSettingsWritersQml::paintAlphaIsEightBit()
{
    QFETCH(QColor, color);
    QFETCH(QString, css);
    Session session = open(QString());
    QVariant qmlColor;
    QVERIFY(QMetaObject::invokeMethod(
        session.host.get(), "color", Q_RETURN_ARG(QVariant, qmlColor), Q_ARG(QVariant, color.name(QColor::HexArgb))));
    QCOMPARE(script(session.host.get(), QStringLiteral("Kdl"), QStringLiteral("cssColor"), {qmlColor}).toString(), css);
    const QString argb = script(session.host.get(), QStringLiteral("RulePaint"), QStringLiteral("argbFromCss"), {css}).toString();
    QCOMPARE(QColor(argb).alpha(), css.size() == 9 ? color.alpha() : 255);
    QCOMPARE(QColor(argb).rgb(), color.rgb());
}

QTEST_MAIN(TestSettingsWritersQml)
#include "test_settings_writers_qml.moc"
