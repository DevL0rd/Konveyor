#include "config/keypositions.h"

#include <QTest>

#include <xkbcommon/xkbcommon-keysyms.h>

using namespace Konveyor::Config;

class TestConfigKeyPositions : public QObject
{
    Q_OBJECT

private:
    xkb_context *m_context = nullptr;

    xkb_keymap *keymap(const char *layout)
    {
        const xkb_rule_names names {"evdev", "pc105", layout, "", ""};
        return xkb_keymap_new_from_names(m_context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    }

private Q_SLOTS:
    void initTestCase()
    {
        m_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        QVERIFY(m_context);
    }

    void cleanupTestCase() { xkb_context_unref(m_context); }

    void findsTheUsKeyForASymbol()
    {
        QCOMPARE(usKeycode(XKB_KEY_grave), std::optional<quint32>(49));
        QCOMPARE(usKeycode(XKB_KEY_bracketleft), std::optional<quint32>(34));
        QCOMPARE(usKeycode(XKB_KEY_q), std::optional<quint32>(24));
        QCOMPARE(usKeycode(XKB_KEY_Q), std::optional<quint32>(24));
        QCOMPARE(usKeycode(XKB_KEY_Page_Up), std::optional<quint32>(112));
        QCOMPARE(usKeycode(XKB_KEY_plus), std::nullopt);
    }

    void knowsWhichSymbolsALayoutTypes_data()
    {
        QTest::addColumn<QByteArray>("layout");
        QTest::addColumn<quint32>("keysym");
        QTest::addColumn<bool>("types");
        QTest::newRow("us grave") << QByteArrayLiteral("us") << quint32(XKB_KEY_grave) << true;
        QTest::newRow("de grave is only a dead key") << QByteArrayLiteral("de") << quint32(XKB_KEY_grave) << false;
        QTest::newRow("es grave is only a dead key") << QByteArrayLiteral("es") << quint32(XKB_KEY_grave) << false;
        QTest::newRow("se grave is only a dead key") << QByteArrayLiteral("se") << quint32(XKB_KEY_grave) << false;
        QTest::newRow("ch grave is only a dead key") << QByteArrayLiteral("ch") << quint32(XKB_KEY_grave) << false;
        QTest::newRow("fr grave through AltGr") << QByteArrayLiteral("fr") << quint32(XKB_KEY_grave) << true;
        QTest::newRow("de bracket through AltGr") << QByteArrayLiteral("de") << quint32(XKB_KEY_bracketleft) << true;
        QTest::newRow("de minus") << QByteArrayLiteral("de") << quint32(XKB_KEY_minus) << true;
        QTest::newRow("ru has no latin q") << QByteArrayLiteral("ru") << quint32(XKB_KEY_q) << false;
    }

    void knowsWhichSymbolsALayoutTypes()
    {
        QFETCH(QByteArray, layout);
        QFETCH(quint32, keysym);
        QFETCH(bool, types);
        xkb_keymap *compiled = keymap(layout.constData());
        QVERIFY(compiled);
        QCOMPARE(layoutTypesKeysym(compiled, 0, keysym), types);
        xkb_keymap_unref(compiled);
    }

    void findsTheKeyThatTypesASymbolOnALayout_data()
    {
        QTest::addColumn<QByteArray>("layout");
        QTest::addColumn<quint32>("index");
        QTest::addColumn<quint32>("keysym");
        QTest::addColumn<std::optional<quint32>>("keycode");
        QTest::newRow("us h") << QByteArrayLiteral("us") << quint32(0) << quint32(XKB_KEY_h) << std::optional<quint32>(43);
        QTest::newRow("us upper case H") << QByteArrayLiteral("us") << quint32(0) << quint32(XKB_KEY_H) << std::optional<quint32>(43);
        QTest::newRow("ru has no h") << QByteArrayLiteral("ru") << quint32(0) << quint32(XKB_KEY_h) << std::optional<quint32>();
        QTest::newRow("second layout") << QByteArrayLiteral("ru,us") << quint32(1) << quint32(XKB_KEY_h) << std::optional<quint32>(43);
        QTest::newRow("de y and z swap") << QByteArrayLiteral("de") << quint32(0) << quint32(XKB_KEY_y) << std::optional<quint32>(52);
        QTest::newRow("only at a higher level") << QByteArrayLiteral("de") << quint32(0) << quint32(XKB_KEY_at) << std::optional<quint32>();
    }

    void findsTheKeyThatTypesASymbolOnALayout()
    {
        QFETCH(QByteArray, layout);
        QFETCH(quint32, index);
        QFETCH(quint32, keysym);
        QFETCH(std::optional<quint32>, keycode);
        xkb_keymap *compiled = keymap(layout.constData());
        QVERIFY(compiled);
        QCOMPARE(keycodeOnLayout(compiled, index, keysym), keycode);
        xkb_keymap_unref(compiled);
        QCOMPARE(keycodeOnLayout(nullptr, 0, keysym), std::nullopt);
    }

    void readsTheActiveLayoutOfAMultiLayoutKeymap()
    {
        xkb_keymap *compiled = keymap("us,de");
        QVERIFY(compiled);
        QVERIFY(layoutTypesKeysym(compiled, 0, XKB_KEY_grave));
        QVERIFY(!layoutTypesKeysym(compiled, 1, XKB_KEY_grave));
        xkb_keymap_unref(compiled);
    }
};

QTEST_GUILESS_MAIN(TestConfigKeyPositions)
#include "test_config_keypositions.moc"
