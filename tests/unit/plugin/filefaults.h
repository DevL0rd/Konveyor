#pragma once

#include <QFile>
#include <QString>

#include <csignal>

#include <sys/resource.h>

namespace Konveyor::Test
{

class Permissions
{
public:
    Permissions(const QString &path, QFileDevice::Permissions locked)
        : m_path(path)
        , m_original(QFile::permissions(path))
    {
        QFile::setPermissions(path, locked);
    }

    Permissions(const Permissions &) = delete;
    Permissions &operator=(const Permissions &) = delete;

    ~Permissions() { QFile::setPermissions(m_path, m_original); }

private:
    QString m_path;
    QFileDevice::Permissions m_original;
};

class WriteLimit
{
public:
    explicit WriteLimit(rlim_t bytes)
    {
        getrlimit(RLIMIT_FSIZE, &m_original);
        m_previous = signal(SIGXFSZ, SIG_IGN);
        const rlimit limit {bytes, m_original.rlim_max};
        setrlimit(RLIMIT_FSIZE, &limit);
    }

    WriteLimit(const WriteLimit &) = delete;
    WriteLimit &operator=(const WriteLimit &) = delete;

    ~WriteLimit()
    {
        setrlimit(RLIMIT_FSIZE, &m_original);
        signal(SIGXFSZ, m_previous);
    }

private:
    rlimit m_original {};
    void (*m_previous)(int) = SIG_DFL;
};

}
