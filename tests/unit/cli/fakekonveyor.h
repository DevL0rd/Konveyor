#pragma once

#include "../dbus/privatebus.h"

#include <QMap>

namespace Konveyor::Test
{

class FakeKonveyor
{
public:
    FakeKonveyor()
        : m_service(QStringLiteral("org.kde.Konveyor"), QStringLiteral("/Konveyor"),
              [this](const QDBusMessage &message) { return answer(message); })
    { }

    bool start(const QString &address) { return m_service.start(address); }

    void reply(const QString &member, const QString &text)
    {
        const QMutexLocker locker(&m_mutex);
        m_replies.insert(member, text);
    }

    QList<QDBusMessage> calls(const QString &member) const { return m_service.calls(member); }

private:
    QDBusMessage answer(const QDBusMessage &message)
    {
        const QMutexLocker locker(&m_mutex);
        if (message.interface() != QLatin1String("org.kde.Konveyor") || !m_replies.contains(message.member())) {
            return message.createErrorReply(QDBusError::UnknownMethod, message.member());
        }
        return message.createReply(m_replies.value(message.member()));
    }

    QMutex m_mutex;
    QMap<QString, QString> m_replies;
    FakeService m_service;
};

}
