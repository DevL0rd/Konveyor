#pragma once

#include "config/types.h"

#include <input.h>

#include <QSet>

#include <functional>

namespace Konveyor
{

struct InputHandlers
{
    std::function<bool(Config::BindTrigger, Qt::KeyboardModifiers, Config::MouseButton, Config::ScrollDirection)> pointerBind;
    std::function<void(const QPointF &, qint64)> pointerMoved;
    std::function<void()> pointerReleased;
    std::function<bool(const QPointF &)> tabClicked;
    std::function<bool(quint32, Qt::KeyboardModifiers, bool)> keyPositionBind;
    std::function<void()> escapePressed;
};

class InputFilter : public KWin::InputEventFilter
{
public:
    explicit InputFilter(InputHandlers handlers);
    ~InputFilter() override;

    bool pointerAxis(KWin::PointerAxisEvent *event) override;
    bool pointerButton(KWin::PointerButtonEvent *event) override;
    bool pointerMotion(KWin::PointerMotionEvent *event) override;
    bool keyboardKey(KWin::KeyboardKeyEvent *event) override;

private:
    bool triggersBind(const KWin::KeyboardKeyEvent *event);

    InputHandlers m_handlers;
    QSet<quint32> m_swallowedKeys;
    QSet<Qt::MouseButton> m_swallowedButtons;
};

class DragMotionFilter : public KWin::InputEventFilter
{
public:
    explicit DragMotionFilter(std::function<void(const QPointF &, qint64)> moved);

    bool pointerMotion(KWin::PointerMotionEvent *event) override;

private:
    std::function<void(const QPointF &, qint64)> m_moved;
};

}
