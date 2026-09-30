#pragma once

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QStringList>

namespace Konveyor::Test
{

class FakeAction : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE void trigger() { triggered = true; }

    bool triggered = false;
};

class FakePlasmoidAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap *configuration READ configuration CONSTANT)
    Q_PROPERTY(int formFactor MEMBER formFactor NOTIFY formFactorChanged)
    Q_PROPERTY(int id MEMBER id CONSTANT)
    Q_PROPERTY(QString icon MEMBER icon NOTIFY iconChanged)
    Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)

public:
    static FakePlasmoidAttached &instance()
    {
        static FakePlasmoidAttached attached;
        return attached;
    }

    QQmlPropertyMap *configuration() const { return config; }

    Q_INVOKABLE QObject *internalAction(const QString &name)
    {
        requestedActions.append(name);
        QQmlEngine::setObjectOwnership(&action, QQmlEngine::CppOwnership);
        return &action;
    }

    QQmlPropertyMap *config = nullptr;
    int formFactor = 0;
    int id = 7;
    QString icon;
    QString title;
    QStringList requestedActions;
    FakeAction action;

Q_SIGNALS:
    void formFactorChanged();
    void iconChanged();
    void titleChanged();
};

class FakePlasmoid : public QObject
{
    Q_OBJECT

public:
    static FakePlasmoidAttached *qmlAttachedProperties(QObject *) { return &FakePlasmoidAttached::instance(); }
};

class FakePlasmoidItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool expanded MEMBER m_expanded NOTIFY expandedChanged)
    Q_PROPERTY(QQmlComponent *compactRepresentation MEMBER m_compact NOTIFY compactRepresentationChanged)
    Q_PROPERTY(QQmlComponent *fullRepresentation MEMBER m_full NOTIFY fullRepresentationChanged)
    Q_PROPERTY(QQmlComponent *preferredRepresentation MEMBER m_preferred NOTIFY preferredRepresentationChanged)
    Q_PROPERTY(QString toolTipMainText MEMBER m_toolTipMainText NOTIFY toolTipChanged)
    Q_PROPERTY(QString toolTipSubText MEMBER m_toolTipSubText NOTIFY toolTipChanged)

public:
    using QQuickItem::QQuickItem;

Q_SIGNALS:
    void expandedChanged();
    void compactRepresentationChanged();
    void fullRepresentationChanged();
    void preferredRepresentationChanged();
    void toolTipChanged();

private:
    bool m_expanded = false;
    QQmlComponent *m_compact = nullptr;
    QQmlComponent *m_full = nullptr;
    QQmlComponent *m_preferred = nullptr;
    QString m_toolTipMainText;
    QString m_toolTipSubText;
};

inline void registerFakePlasmoid()
{
    qmlRegisterType<FakePlasmoidItem>("org.kde.plasma.plasmoid", 2, 0, "PlasmoidItem");
    qmlRegisterUncreatableType<FakePlasmoid>("org.kde.plasma.plasmoid", 2, 0, "Plasmoid", QStringLiteral("attached only"));
}

}

QML_DECLARE_TYPEINFO(Konveyor::Test::FakePlasmoid, QML_HAS_ATTACHED_PROPERTIES)
