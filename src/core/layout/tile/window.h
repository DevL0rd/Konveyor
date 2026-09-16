#pragma once

#include "anim/duration.h"
#include "layout/engine/engine.h"
#include "layout/rules/windowrules.h"

#include <QSize>
#include <QSizeF>

#include <optional>

namespace Konveyor::Layout
{

class LayoutWindow
{
public:
    enum class OneShotResize
    {
        None,
        AwaitingCommit,
        FollowWindow
    };

    LayoutWindow(WindowId id, const WindowProperties &properties);

    WindowId id() const { return m_id; }
    const WindowProperties &properties() const { return m_properties; }
    void setProperties(const WindowProperties &properties) { m_properties = properties; }
    const EffectiveWindowRules &rules() const { return m_rules; }
    bool setRules(const EffectiveWindowRules &rules);

    QSizeF size() const { return m_size; }
    QSize minSize() const;
    QSize maxSize() const;
    std::optional<QSize> requestedSize() const { return m_requestedSize; }
    std::optional<QSize> pendingSize() const;

    WindowMode sizingMode() const { return m_sizingMode; }
    WindowMode requestedMode() const { return m_requestedMode; }
    bool isWindowedFullscreen() const { return m_windowedFullscreen; }
    bool windowedFullscreenRequested() const { return m_windowedFullscreenRequested; }
    void requestWindowedFullscreen(bool value);

    void requestSize(QSize size, WindowMode mode, bool animate);
    void requestSizeUntilCommit(QSize size, bool animate);
    std::optional<QSizeF> takeResizeStartSize();
    bool commit(QSizeF frameSize);

    bool isChildOf(const LayoutWindow &parent) const;

    bool isFocused() const { return m_isFocused; }
    void setFocused(bool focused);
    bool isUrgent() const { return m_isUrgent; }
    void setUrgent(bool urgent);
    bool isActivated() const { return m_isActivated; }
    void setActivated(bool active);
    bool isActiveInColumn() const { return m_isActiveInColumn; }
    void setActiveInColumn(bool active);
    bool isFloating() const { return m_isFloating; }
    void setFloating(bool floating);
    bool needsRuleRecompute() const { return m_needsRuleRecompute; }
    void markRulesDirty() { m_needsRuleRecompute = true; }

    bool isIgnoringOpacityRule() const { return m_ignoreOpacityRule; }
    void toggleIgnoreOpacityRule() { m_ignoreOpacityRule = !m_ignoreOpacityRule; }

    std::optional<Anim::Duration> focusTimestamp() const { return m_focusTimestamp; }
    void setFocusTimestamp(Anim::Duration timestamp) { m_focusTimestamp = timestamp; }

    std::optional<quint8> interactiveResizeEdges() const { return m_resizeEdges; }
    void setInteractiveResize(std::optional<quint8> edges);
    void cancelInteractiveResize();

private:
    WindowId m_id;
    WindowProperties m_properties;
    EffectiveWindowRules m_rules;
    QSizeF m_size;
    std::optional<QSize> m_requestedSize;
    WindowMode m_sizingMode = WindowMode::Normal;
    WindowMode m_requestedMode = WindowMode::Normal;
    bool m_windowedFullscreen = false;
    bool m_windowedFullscreenRequested = false;
    OneShotResize m_oneShotResize = OneShotResize::None;
    bool m_uncommittedRequest = false;
    bool m_resizeEndPending = false;
    bool m_animateNextResize = false;
    std::optional<QSizeF> m_resizeStartSize;
    bool m_isFocused = false;
    bool m_isUrgent = false;
    bool m_isActivated = false;
    bool m_isActiveInColumn = true;
    bool m_isFloating = false;
    bool m_needsRuleRecompute = false;
    bool m_ignoreOpacityRule = false;
    std::optional<Anim::Duration> m_focusTimestamp;
    std::optional<quint8> m_resizeEdges;
};

}
