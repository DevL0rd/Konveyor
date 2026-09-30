#pragma once

#include "plasmadoubles.h"

#include <QQmlEngine>
#include <QQuickWindow>
#include <QTemporaryDir>

#include <memory>

namespace Form
{
constexpr int Planar = 0;
constexpr int Horizontal = 2;
constexpr int Vertical = 3;
}

struct PlasmoidSpec
{
    QString ui;
    QString pluginId;
    QString iconName;
    QStringList libs;
};

Q_DECLARE_METATYPE(PlasmoidSpec)

class PlasmoidHarness
{
public:
    explicit PlasmoidHarness(PlasmoidSpec spec);
    ~PlasmoidHarness();

    static void prepareEnvironment();
    static QString widgetsDir();
    static QByteArray fixture(const QString &name);
    static std::unique_ptr<PlasmoidHarness> started(const PlasmoidSpec &spec, int formFactor, const QVariantMap &config = {});
    bool overlay(const QByteArray &targets, bool visible) const;
    static QStringList &messages();
    static QString report();

    void setUp(int formFactor, const QVariantMap &config = {});
    QObject *create(const QString &relative, const QVariantMap &properties = {});
    QObject *load(int formFactor, const QVariantMap &config = {});
    QQuickItem *show(const char *representation);
    QQuickItem *showItem(QObject *object);
    QString command(const QString &prefix) const;
    bool reply(const QString &prefix, const QString &out, int exitCode = 0, const QString &err = QString());
    void resolveRuntime(const QString &prefix);
    void writeFile(const QString &path, const QByteArray &content) const;
    bool deliver(const QString &path, const QByteArray &content, const std::function<bool()> &arrived) const;
    void unload();
    QString runtimePath(const QString &name) const;
    QString stagedPath(const QString &relative) const;
    QList<QObject *> findAll(const char *type) const;
    QQuickItem *scene() const;
    bool watching(const QString &path) const;
    QVariant eval(const QString &expression, QObject *scope = nullptr) const;
    QVariant config(const QString &key) const;
    void setConfig(const QString &key, const QVariant &value);

    QObject *root() const { return m_root.get(); }
    PlasmoidDouble *plasmoid() const { return m_plasmoid.get(); }
    QQmlEngine *engine() const { return m_engine.get(); }
    QString error;

private:
    void stage();
    void loadConfig();

    PlasmoidSpec m_spec;
    QTemporaryDir m_dir;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<PlasmoidDouble> m_plasmoid;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<QObject> m_root;
    QList<QObject *> m_shown;
};

QStringList visibleTexts(QQuickItem *item);
QStringList plainTexts(QQuickItem *item);
QList<QObject *> findByType(QObject *root, const char *type);
QList<QQuickItem *> visibleItems(QQuickItem *item, const char *type);
bool watching(QObject *root, const QString &path);
QList<QQuickItem *> shownDelegates(QQuickItem *view);
QObject *withText(const QList<QObject *> &objects, const QString &text);
void clickAt(QQuickItem *item, Qt::MouseButton button, QPointF local = {-1, -1});
