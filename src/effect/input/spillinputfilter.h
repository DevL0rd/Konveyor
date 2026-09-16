#pragma once

#include <input.h>

#include <QPointer>
#include <QSet>

#include <functional>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class SpillInputFilter : public KWin::InputEventFilter
{
public:
    using OwnsPoint = std::function<bool(KWin::Window *, const QPointF &)>;

    explicit SpillInputFilter(OwnsPoint ownsPoint);
    ~SpillInputFilter() override;

    bool pointerMotion(KWin::PointerMotionEvent *event) override;
    bool pointerButton(KWin::PointerButtonEvent *event) override;
    bool pointerAxis(KWin::PointerAxisEvent *event) override;

private:
    bool isStolen(const QPointF &position) const;
    KWin::Window *ownerAt(const QPointF &position) const;
    void redirectTo(KWin::Window *window, const QPointF &position);
    void restore(const QPointF &position);

    OwnsPoint m_ownsPoint;
    QPointer<KWin::Window> m_target;
    bool m_redirecting = false;
    QSet<quint32> m_redirectedButtons;
};

}
