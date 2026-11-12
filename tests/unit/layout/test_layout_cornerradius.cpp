#include "helpers.h"

using namespace LayoutTest;

class TestLayoutCornerRadius : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cornerRadiusAndClippingReachWindowState()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.geometryCornerRadius = Config::CornerRadius {8, 8, 8, 8};
        rule.clipToGeometry = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("round"));
        QVERIFY(fixture.state(id).cornerRadius == Config::CornerRadius({8, 8, 8, 8}));
        QVERIFY(fixture.state(id).clipToGeometry);
        VERIFY_INVARIANTS(fixture);
    }

    void zeroCornerRadiusStaysSquareInsteadOfUnset()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.geometryCornerRadius = Config::CornerRadius {};
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("square"));
        QVERIFY(fixture.state(id).cornerRadius == Config::CornerRadius());
        VERIFY_INVARIANTS(fixture);
    }

    void cornerRadiusWithoutRuleIsUnset()
    {
        Fixture fixture(instantConfig());
        const auto id = fixture.add(QStringLiteral("plain"));
        QVERIFY(!fixture.state(id).cornerRadius.has_value());
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutCornerRadius)
#include "test_layout_cornerradius.moc"
