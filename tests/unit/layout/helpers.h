#pragma once

#include "anim/clock.h"
#include "layout/engine/engine.h"

#include <QHash>
#include <QTest>

#include <chrono>
#include <optional>

namespace LayoutTest
{

using Konveyor::Anim::Clock;
using Konveyor::Anim::Duration;
namespace Config = Konveyor::Config;
namespace Layout = Konveyor::Layout;

inline Layout::OutputInfo makeOutput(const QString &name, QRectF geometry, double scale = 1.0)
{
    Layout::OutputInfo info;
    info.name = name;
    info.geometry = geometry;
    info.workArea = geometry;
    info.scale = scale;
    return info;
}

inline Layout::WindowProperties makeWindow(const QString &appId = {}, const QString &title = {}, QSizeF size = QSizeF(100, 100))
{
    Layout::WindowProperties properties;
    properties.appId = appId;
    properties.title = title;
    properties.frameSize = size;
    return properties;
}

inline Config::Config instantConfig()
{
    Config::Config config;
    config.animations.enabled = false;
    config.layout.defaultColumnWidth = Config::Proportion {0.5};
    config.layout.presetColumnWidths = {Config::Proportion {1.0 / 3.0}, Config::Proportion {0.5}, Config::Proportion {2.0 / 3.0}};
    config.layout.rememberWindowSizes = false;
    return config;
}

inline Config::WindowRule ruleFor(const QString &appId)
{
    Config::WindowRule rule;
    Config::Match match;
    match.appId = QRegularExpression(QStringLiteral("^") + appId + QStringLiteral("$"));
    rule.matches.append(match);
    return rule;
}

inline Config::Action action(
    const QString &name, const QStringList &arguments = {}, const QList<std::pair<QString, QString>> &properties = {})
{
    return Config::Action {name, arguments, properties};
}

class Fixture
{
public:
    explicit Fixture(const Config::Config &config = instantConfig(), QRectF geometry = QRectF(0, 0, 1920, 1080), Layout::Hooks hooks = {})
        : m_clock(Clock::frozenAt(Duration::zero()))
        , m_engine(m_clock, std::move(hooks))
    {
        m_engine.setConfig(config);
        m_engine.addOutput(makeOutput(QStringLiteral("DP-1"), geometry));
    }

    Layout::Engine &engine() { return m_engine; }
    Clock &clock() { return m_clock; }

    void setConfig(const Config::Config &config) { m_engine.setConfig(config); }

    Layout::WindowId add(const QString &appId = {}, QSizeF size = QSizeF(100, 100))
    {
        const Layout::WindowId id = ++m_nextId;
        m_engine.addWindow(id, makeWindow(appId, appId, size), QString(), Layout::ActivationPolicy::Focus);
        settle();
        return id;
    }

    Layout::WindowId addWith(const Layout::WindowProperties &properties, Layout::ActivationPolicy policy = Layout::ActivationPolicy::Focus)
    {
        const Layout::WindowId id = ++m_nextId;
        m_engine.addWindow(id, properties, QString(), policy);
        settle();
        return id;
    }

    Layout::ActionResult perform(
        const QString &name, const QStringList &arguments = {}, const QList<std::pair<QString, QString>> &properties = {})
    {
        const Layout::ActionResult result = m_engine.perform(action(name, arguments, properties));
        settle();
        return result;
    }

    void settle()
    {
        for (int round = 0; round < 8; ++round) {
            bool changed = false;
            for (const Layout::WindowState &state : m_engine.windowStates()) {
                const QSizeF size = state.targetFrame.size();
                if (m_committed.value(state.id) == size) {
                    continue;
                }
                m_committed.insert(state.id, size);
                m_engine.windowSizeCommitted(state.id, size);
                changed = true;
            }
            if (!changed) {
                return;
            }
        }
    }

    Layout::WindowState state(Layout::WindowId id)
    {
        const auto found = m_engine.windowState(id);
        return found.value_or(Layout::WindowState());
    }

    QRectF frame(Layout::WindowId id) { return state(id).targetFrame; }

    std::optional<Layout::WindowId> focused() const { return m_engine.focusedWindow(); }

    void advance(qint64 milliseconds)
    {
        m_elapsed += milliseconds;
        m_clock.setRawNow(std::chrono::duration_cast<Duration>(std::chrono::milliseconds(m_elapsed)));
        m_engine.tickAnimations();
        settle();
    }

    void remove(Layout::WindowId id)
    {
        m_engine.removeWindow(id);
        m_committed.remove(id);
        settle();
    }

    QString invariants() { return m_engine.checkConsistency(); }

private:
    Clock m_clock;
    Layout::Engine m_engine;
    QHash<Layout::WindowId, QSizeF> m_committed;
    Layout::WindowId m_nextId = 0;
    qint64 m_elapsed = 0;
};

inline Config::Config nativeWidthCycleConfig(bool floating)
{
    Config::Config config = instantConfig();
    config.layout.presetColumnWidths
        = {Config::PresetSize(Config::Fixed {300}), Config::PresetSize(Config::Fixed {600}), Config::PresetSize(Config::Fixed {900})};
    Config::WindowRule rule = ruleFor(QStringLiteral("game"));
    rule.forceResizable = true;
    rule.openFloating = floating;
    config.windowRules.append(rule);
    return config;
}

inline void verifyNativeWidthCycle(Fixture &fixture, Layout::WindowId id)
{
    fixture.perform(QStringLiteral("switch-preset-column-width"), {}, {{QStringLiteral("from-native"), QStringLiteral("true")}});
    QCOMPARE(fixture.frame(id).width(), 600.0);
    QCOMPARE(fixture.state(id).widthPresetIndex, std::optional(1));
    QCOMPARE(fixture.state(id).nativeWidthSuccessorIndex, std::optional(1));

    fixture.perform(QStringLiteral("switch-preset-column-width-back"), {}, {{QStringLiteral("from-native"), QStringLiteral("true")}});
    QCOMPARE(fixture.frame(id).width(), 300.0);
    QCOMPARE(fixture.state(id).widthPresetIndex, std::optional(0));
    QCOMPARE(fixture.state(id).widthPresetCount, 3);
}

}

#define VERIFY_INVARIANTS(fixture) QVERIFY2((fixture).invariants().isEmpty(), qPrintable((fixture).invariants()))
