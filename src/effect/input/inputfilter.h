#pragma once

#include "config/types.h"

#include <input.h>

#include <functional>

namespace Konveyor
{

struct InputHandlers
{
    std::function<bool(Config::BindTrigger, Qt::KeyboardModifiers, Config::MouseButton, Config::ScrollDirection)> pointerBind;
    std::function<void(const QPointF &, qint64)> pointerMoved;
    std::function<void()> pointerReleased;
};

class InputFilter : public KWin::InputEventFilter
{
public:
    explicit InputFilter(InputHandlers handlers);
    ~InputFilter() override;

    bool pointerAxis(KWin::PointerAxisEvent *event) override;
    bool pointerButton(KWin::PointerButtonEvent *event) override;
    bool pointerMotion(KWin::PointerMotionEvent *event) override;

private:
    InputHandlers m_handlers;
};

}
