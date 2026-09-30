#include <core/inputdevice.h>
#include <input.h>
#include <plugin.h>

#include <QDBusConnection>
#include <QPointF>

#include <chrono>
#include <memory>

namespace
{

std::chrono::microseconds now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
}

class VirtualTouchpad : public KWin::InputDevice
{
    Q_OBJECT

public:
    explicit VirtualTouchpad(const QString &sysPath)
        : m_sysPath(sysPath)
    { }

    QString sysPath() const override { return m_sysPath; }
    QString name() const override { return QStringLiteral("Konveyor test touchpad"); }
    bool isEnabled() const override { return true; }
    void setEnabled(bool enabled) override { Q_UNUSED(enabled) }
    bool isKeyboard() const override { return false; }
    bool isPointer() const override { return true; }
    bool isTouchpad() const override { return true; }
    bool isTouch() const override { return false; }
    bool isTabletTool() const override { return false; }
    bool isTabletPad() const override { return false; }
    bool isTabletModeSwitch() const override { return false; }
    bool isLidSwitch() const override { return false; }

private:
    QString m_sysPath;
};

class TouchpadPlugin : public KWin::Plugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KonveyorTest.Touchpad")

public:
    TouchpadPlugin()
    {
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/Touchpad"), this, QDBusConnection::ExportAllSlots);
        QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.KonveyorTest"));
    }

    ~TouchpadPlugin() override { Remove(); }

public Q_SLOTS:
    void Add(const QString &sysPath, bool lmrTapButtonMap)
    {
        Remove();
        m_device = std::make_unique<VirtualTouchpad>(sysPath);
        m_device->setProperty("lmrTapButtonMap", lmrTapButtonMap);
        KWin::input()->addInputDevice(m_device.get());
    }

    void Remove()
    {
        if (m_device) {
            KWin::input()->removeInputDevice(m_device.get());
            m_device.reset();
        }
    }

    void Swipe(int fingers, double dx, double dy, int steps, bool cancel)
    {
        Q_EMIT m_device->swipeGestureBegin(fingers, now(), m_device.get());
        for (int step = 0; step < steps; ++step) {
            Q_EMIT m_device->swipeGestureUpdate(QPointF(dx / steps, dy / steps), now(), m_device.get());
        }
        if (cancel) {
            Q_EMIT m_device->swipeGestureCancelled(now(), m_device.get());
        } else {
            Q_EMIT m_device->swipeGestureEnd(now(), m_device.get());
        }
    }

    void Pinch(int fingers, double scale, int steps, bool cancel)
    {
        Q_EMIT m_device->pinchGestureBegin(fingers, now(), m_device.get());
        for (int step = 1; step <= steps; ++step) {
            Q_EMIT m_device->pinchGestureUpdate(1.0 + (scale - 1.0) * step / steps, 0.0, QPointF(), now(), m_device.get());
        }
        if (cancel) {
            Q_EMIT m_device->pinchGestureCancelled(now(), m_device.get());
        } else {
            Q_EMIT m_device->pinchGestureEnd(now(), m_device.get());
        }
    }

    void Scroll(bool horizontal, double delta)
    {
        const KWin::PointerAxis axis = horizontal ? KWin::PointerAxis::Horizontal : KWin::PointerAxis::Vertical;
        Q_EMIT m_device->pointerAxisChanged(axis, delta, 0, KWin::PointerAxisSource::Finger, false, now(), m_device.get());
        Q_EMIT m_device->pointerFrame(m_device.get());
    }

    void Button(uint button, bool pressed)
    {
        const KWin::PointerButtonState state = pressed ? KWin::PointerButtonState::Pressed : KWin::PointerButtonState::Released;
        Q_EMIT m_device->pointerButtonChanged(button, state, now(), m_device.get());
        Q_EMIT m_device->pointerFrame(m_device.get());
    }

private:
    std::unique_ptr<VirtualTouchpad> m_device;
};

class TouchpadPluginFactory : public KWin::PluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginFactory_iid FILE "metadata.json")
    Q_INTERFACES(KWin::PluginFactory)

public:
    std::unique_ptr<KWin::Plugin> create() const override { return std::make_unique<TouchpadPlugin>(); }
};

}

#include "touchpadplugin.moc"
