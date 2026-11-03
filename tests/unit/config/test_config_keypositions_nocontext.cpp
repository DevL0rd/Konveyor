#include "config/keypositions.h"

#include <QTest>

#include <xkbcommon/xkbcommon-keysyms.h>

using namespace Konveyor::Config;

extern "C" xkb_context *xkb_context_new(enum xkb_context_flags)
{
    return nullptr;
}

class TestConfigKeyPositionsNoContext : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void withoutAKeymapContextNoKeyHasAPosition()
    {
        QVERIFY(!usKeycode(XKB_KEY_a).has_value());
        QVERIFY(!usKeycode(XKB_KEY_space).has_value());
    }
};

QTEST_GUILESS_MAIN(TestConfigKeyPositionsNoContext)
#include "test_config_keypositions_nocontext.moc"
