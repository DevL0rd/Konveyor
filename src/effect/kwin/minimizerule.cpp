#include "kwin/minimizerule.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <config-kwin.h>
#include <rules.h>
#include <window.h>
#include <workspace.h>

namespace Konveyor
{

namespace
{

constexpr QLatin1StringView ruleId("konveyor-no-minimize");
constexpr int forceRule = 2;
constexpr bool orderedRules = PROJECT_VERSION_MAJOR > 6 || PROJECT_VERSION_MINOR >= 8;
constexpr QLatin1StringView legacyListKey("rules");
constexpr QLatin1StringView listKey(orderedRules ? "Order" : "rules");

void reloadKWinRules()
{
    KWin::workspace()->rulebook()->load();
    const QList<KWin::Window *> windows = KWin::workspace()->windows();
    for (KWin::Window *window : windows) {
        if (window->supportsWindowRules()) {
            window->evaluateWindowRules();
            window->invalidateDecoration();
        }
    }
}
}

void MinimizeRule::apply(bool blockMinimize)
{
    const KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("kwinrulesrc"), KConfig::NoGlobals);
    config->reparseConfiguration();
    KConfigGroup general = config->group(QStringLiteral("General"));
    QStringList rules = general.readEntry(listKey, QStringList());
    if (rules.isEmpty()) {
        rules = general.readEntry(legacyListKey, QStringList());
    }
    const KConfigGroup existing = config->group(ruleId);
    const bool blocking
        = rules.contains(ruleId) && existing.readEntry("minimizerule", 0) == forceRule && !existing.readEntry("minimize", true);
    const bool absent = !rules.contains(ruleId) && !config->hasGroup(ruleId);
    if (blockMinimize ? blocking : absent) {
        return;
    }
    rules.removeAll(ruleId);
    config->deleteGroup(ruleId);
    if (blockMinimize) {
        rules.append(ruleId);
        KConfigGroup rule = config->group(ruleId);
        rule.writeEntry("Description", QStringLiteral("Konveyor: windows cannot be minimized"));
        rule.writeEntry("minimize", false);
        rule.writeEntry("minimizerule", forceRule);
    }
    if constexpr (orderedRules) {
        general.deleteEntry(legacyListKey);
        general.deleteEntry("count");
    } else {
        general.writeEntry("count", rules.size());
    }
    general.writeEntry(listKey, rules);
    config->sync();
    reloadKWinRules();
}

}
