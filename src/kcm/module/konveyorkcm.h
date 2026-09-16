#pragma once

#include "document/configdocument.h"
#include "system/livesession.h"

#include <KQuickConfigModule>

#include <QKeySequence>
#include <QVariantList>
#include <QVariantMap>

namespace Konveyor::Settings
{

class KonveyorKcm : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY documentChanged)
    Q_PROPERTY(QString configError READ configError NOTIFY documentChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString initialPage READ initialPage CONSTANT)
    Q_PROPERTY(QVariantMap values READ values NOTIFY documentChanged)
    Q_PROPERTY(LiveSession *live READ live CONSTANT)

public:
    KonveyorKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);

    int revision() const;
    QString configError() const;
    QString configPath() const;
    QString initialPage() const;
    QVariantMap values() const;
    LiveSession *live() const;

    void load() override;
    void save() override;
    void defaults() override;

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
    void editFailed(const QString &message);

private:
    bool apply(const EditResult &result);
    void setDocumentText(const QString &text);
    void refresh();

    ConfigDocument m_document;
    ConfigDocument m_defaults;
    QString m_savedText;
    QString m_defaultText;
    QString m_configError;
    QString m_initialPage;
    QVariantMap m_values;
    LiveSession *m_live = nullptr;
    int m_revision = 0;
};

}
