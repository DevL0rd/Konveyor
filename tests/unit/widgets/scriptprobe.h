#pragma once

#include <QQmlComponent>
#include <QQmlEngine>

#include <memory>

class ScriptProbe
{
public:
    QString load(const QByteArray &qml)
    {
        m_engine = std::make_unique<QQmlEngine>();
        QQmlComponent component(m_engine.get());
        component.setData(qml, QUrl(QStringLiteral("file:///probe.qml")));
        m_probe.reset(component.create());
        return m_probe ? QString() : component.errorString();
    }

    QObject *get() const { return m_probe.get(); }

private:
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_probe;
};
