#include "plugin/windowmemorystore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace Konveyor
{

WindowMemoryStore::WindowMemoryStore()
    : m_path(QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + QStringLiteral("/konveyor/window-memory.json"))
{ }

Layout::WindowMemory WindowMemoryStore::load() const
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "konveyor: could not read" << m_path << error.errorString();
        return {};
    }
    return Layout::windowMemoryFromJson(document.object());
}

void WindowMemoryStore::save(const Layout::WindowMemory &memory) const
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "konveyor: could not write" << m_path << file.errorString();
        return;
    }
    file.write(QJsonDocument(Layout::windowMemoryToJson(memory)).toJson(QJsonDocument::Compact));
    if (!file.commit()) {
        qWarning() << "konveyor: could not write" << m_path << file.errorString();
    }
}

}
