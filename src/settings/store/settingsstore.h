#pragma once

#include "document/configdocument.h"
#include "store/edithistory.h"
#include "system/livesession.h"

#include <QFileSystemWatcher>
#include <QKeySequence>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace Konveyor::Settings
{

class SettingsStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY documentChanged)
    Q_PROPERTY(QString configError READ configError NOTIFY documentChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QVariantMap values READ values NOTIFY documentChanged)
    Q_PROPERTY(LiveSession *live READ live CONSTANT)
    Q_PROPERTY(bool needsSave READ needsSave NOTIFY documentChanged)
    Q_PROPERTY(bool representsDefaults READ representsDefaults NOTIFY documentChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY documentChanged)
    Q_PROPERTY(bool autoSave READ autoSave WRITE setAutoSave NOTIFY autoSaveChanged)

public:
    explicit SettingsStore(QObject *parent = nullptr);

    int revision() const;
    QString configError() const;
    QString configPath() const;
    QVariantMap values() const;
    LiveSession *live() const;
    bool needsSave() const;
    bool representsDefaults() const;
    bool canUndo() const;
    bool autoSave() const;
    void setAutoSave(bool autoSave);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();
    Q_INVOKABLE void defaults();
    Q_INVOKABLE void undo();

    Q_INVOKABLE bool has(const QString &path) const;
    Q_INVOKABLE bool isDefault(const QString &path) const;
    Q_INVOKABLE bool resetToDefault(const QString &path);
    Q_INVOKABLE QVariantMap node(const QString &path) const;
    Q_INVOKABLE QVariantList children(const QString &parentPath, const QString &name) const;
    Q_INVOKABLE QVariantMap scope(const QString &layoutPath) const;
    Q_INVOKABLE bool setNode(const QString &path, const QVariantMap &node);
    Q_INVOKABLE bool setValue(const QString &path, const QVariantList &arguments, const QVariantMap &properties = {});
    Q_INVOKABLE bool setFlag(const QString &path, bool enabled);
    Q_INVOKABLE bool setToggle(const QString &path, bool enabled);
    Q_INVOKABLE bool remove(const QString &path);
    Q_INVOKABLE QString append(const QString &parentPath, const QVariantMap &node);
    Q_INVOKABLE QString move(const QString &path, int delta);
    Q_INVOKABLE QString keyName(const QKeySequence &sequence) const;
    Q_INVOKABLE QVariantMap checkRule(const QVariantMap &ruleNode) const;
    Q_INVOKABLE QString profileForOutput(const QVariantMap &output) const;
    Q_INVOKABLE void openConfigFile() const;

Q_SIGNALS:
    void documentChanged();
    void autoSaveChanged();
    void editFailed(const QString &message);
    void saved();

private:
    bool apply(const EditResult &result, const QString &before);
    void replaceText(const QString &text);
    void refresh();
    void watch();
    void fileChanged();

    ConfigDocument m_document;
    ConfigDocument m_defaults;
    EditHistory m_history;
    QFileSystemWatcher m_watcher;
    QTimer m_saveTimer;
    QString m_savedText;
    QString m_defaultText;
    QString m_configError;
    QVariantMap m_values;
    LiveSession *m_live = nullptr;
    int m_revision = 0;
    bool m_autoSave = false;
};

}
