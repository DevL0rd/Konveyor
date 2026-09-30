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
    config.layout.alwaysExpandSingleColumn = false;
    return config;
}

inline Config::NamedWorkspace namedWorkspace(const QString &name, std::optional<QString> output = std::nullopt)
{
    Config::NamedWorkspace named;
    named.name = name;
    named.openOnOutput = std::move(output);
    return named;
}

inline Config::Config wideColumns()
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {900});
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

inline QList<std::pair<QString, QString>> idProperty(quint64 id)
{
    return {{QStringLiteral("id"), QString::number(id)}};
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

    void addOutput(const QString &name, QRectF geometry, double scale = 1.0)
    {
        m_engine.addOutput(makeOutput(name, geometry, scale));
        settle();
    }

    void removeOutput(const QString &name)
    {
        m_engine.removeOutput(name);
        settle();
    }

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

    void holdCommits(bool hold)
    {
        m_holdCommits = hold;
        settle();
    }

    void commitAs(Layout::WindowId id, QSizeF size)
    {
        m_committed.insert(id, size);
        m_engine.windowSizeCommitted(id, size);
    }

    void settle()
    {
        if (m_holdCommits) {
            return;
        }
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

    Layout::WorkspaceState workspaceNamed(const QString &name) const
    {
        for (const Layout::WorkspaceState &workspace : m_engine.workspaceStates()) {
            if (workspace.name == name) {
                return workspace;
            }
        }
        return {};
    }

    std::optional<Layout::WindowId> focused() const { return m_engine.focusedWindow(); }

    void passTime(qint64 milliseconds)
    {
        m_elapsed += milliseconds;
        m_clock.setRawNow(std::chrono::duration_cast<Duration>(std::chrono::milliseconds(m_elapsed)));
    }

    qint64 elapsed() const { return m_elapsed; }

    void advance(qint64 milliseconds)
    {
        passTime(milliseconds);
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
    bool m_holdCommits = false;
};

inline bool startMove(Fixture &fixture, Layout::WindowId id, QPointF to, const QString &output = QStringLiteral("DP-1"))
{
    const QPointF start = fixture.frame(id).center();
    if (!fixture.engine().beginWindowDrag(id, start)) {
        return false;
    }
    fixture.engine().updateWindowDrag(to, output);
    return true;
}

struct WideRow
{
    Fixture fixture {wideColumns()};
    QString output;
    Layout::WindowId first = 0;
    Layout::WindowId second = 0;
    Layout::WindowId third = 0;
    Layout::WindowId last = 0;

    explicit WideRow(const QString &on = QStringLiteral("DP-1"))
        : output(on)
    {
        if (on != QLatin1String("DP-1")) {
            fixture.addOutput(on, QRectF(1920, 0, 1920, 1080));
            fixture.engine().focusOutput(on);
        }
        first = fixture.add(QStringLiteral("a"));
        second = fixture.add(QStringLiteral("b"));
        third = fixture.add(QStringLiteral("c"));
        last = fixture.add(QStringLiteral("d"));
        fixture.advance(1000);
    }

    bool inView(Layout::WindowId id)
    {
        const QRectF frame = fixture.frame(id);
        const double left = output == QLatin1String("DP-1") ? 0.0 : 1920.0;
        return frame.left() >= left && frame.right() <= left + 1920.0;
    }
};

inline std::pair<Layout::WindowId, Layout::WindowId> addStackedPair(Fixture &fixture)
{
    const Layout::WindowId top = fixture.add(QStringLiteral("top"));
    const Layout::WindowId bottom = fixture.add(QStringLiteral("bottom"));
    fixture.perform(QStringLiteral("consume-or-expel-window-left"));
    return {top, bottom};
}

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
