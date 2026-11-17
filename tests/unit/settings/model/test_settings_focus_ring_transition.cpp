#include "config/loader.h"
#include "values/configvalues.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;

namespace
{

Config::Config load(const QString &text)
{
    const auto loaded = Config::loadString(text, QStringLiteral("config.kdl"));
    return loaded ? loaded->config : Config::Config();
}

}

class TestSettingsFocusRingTransition : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsToRide()
    {
        const QVariantMap values = layoutValues(Config::Layout());
        QCOMPARE(values.value(QStringLiteral("focus-ring-transition")).toString(), QStringLiteral("ride"));
    }

    void readsCrossfade()
    {
        const QVariantMap values = layoutValues(load(QStringLiteral(R"(
layout { focus-ring-transition "crossfade"; }
)"))
                .layout);
        QCOMPARE(values.value(QStringLiteral("focus-ring-transition")).toString(), QStringLiteral("crossfade"));
    }
};

QTEST_GUILESS_MAIN(TestSettingsFocusRingTransition)
#include "test_settings_focus_ring_transition.moc"
