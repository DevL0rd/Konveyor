#include "kwin/minimizerule.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <rules.h>
#include <window.h>
#include <workspace.h>

namespace Konveyor
{

namespace
{

constexpr QLatin1StringView ruleId("konveyor-no-minimize");
constexpr int forceRule = 2;

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
    QStringList rules = general.readEntry("rules", QStringList());
    if (rules.contains(ruleId) == blockMinimize) {
        return;
    }
    if (blockMinimize) {
        rules.append(ruleId);
        KConfigGroup rule = config->group(ruleId);
        rule.writeEntry("Description", QStringLiteral("Konveyor: windows cannot be minimized"));
        rule.writeEntry("minimize", false);
        rule.writeEntry("minimizerule", forceRule);
    } else {
        rules.removeAll(ruleId);
        config->deleteGroup(ruleId);
    }
    general.writeEntry("rules", rules);
    general.writeEntry("count", rules.size());
    config->sync();
    reloadKWinRules();
}

}
