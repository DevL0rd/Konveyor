#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

void checkFraction(const Kdl::Node &node, const QString &name, double value, double min, double max)
{
    if (value < min || value > max) {
        fail(node, QStringLiteral("%1 must be between %2 and %3").arg(name).arg(min).arg(max));
    }
}

}

void decodeOverview(const Kdl::Node &node, Overview &overview)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(QStringLiteral("enabled"), [&overview](const Kdl::Node &child) { overview.enabled = flagArgument(child); });
    table.insert(QStringLiteral("minimum-scale"), [&overview](const Kdl::Node &child) {
        const double value = numberArgument(child, Range {0, 1});
        checkFraction(child, QStringLiteral("minimum-scale"), value, 0.1, 1.0);
        overview.minimumScale = value;
    });
    table.insert(QStringLiteral("card-gap"), [&overview](const Kdl::Node &child) {
        const double value = numberArgument(child, Range {0, 1});
        checkFraction(child, QStringLiteral("card-gap"), value, 0.0, 0.5);
        overview.cardGap = value;
    });
    table.insert(QStringLiteral("offscreen-indicators"), [&overview](const Kdl::Node &child) {
        overview.offscreenIndicators = keywordArgument(child, {QStringLiteral("chevrons"), QStringLiteral("off")}) == 0
            ? OverviewIndicators::Chevrons
            : OverviewIndicators::Off;
    });
    table.insert(QStringLiteral("allow-kontrol-panel"),
        [&overview](const Kdl::Node &child) { overview.allowKontrolPanel = flagArgument(child); });
    table.insert(QStringLiteral("scroll-speed"), [&overview](const Kdl::Node &child) {
        const double value = numberArgument(child, Range {0, 5});
        checkFraction(child, QStringLiteral("scroll-speed"), value, 0.1, 2.0);
        overview.scrollSpeed = value;
    });
    decodeChildren(node, table);
}

}
