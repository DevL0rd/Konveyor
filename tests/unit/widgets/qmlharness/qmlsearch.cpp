#include "plasmoidharness.h"

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTest>
#include <QtQuickTest/quicktest.h>

QStringList visibleTexts(QQuickItem *item)
{
    QStringList texts;
    if (!item->isVisible()) {
        return texts;
    }
    if (item->inherits("QQuickText")) {
        texts.append(item->property("text").toString());
    }
    const QList<QQuickItem *> children = item->childItems();
    for (QQuickItem *child : children) {
        texts += visibleTexts(child);
    }
    return texts;
}

QStringList plainTexts(QQuickItem *item)
{
    static const QRegularExpression markup(QStringLiteral("<[^>]*>"));
    return visibleTexts(item).replaceInStrings(markup, QString());
}

QList<QObject *> findByType(QObject *root, const char *type)
{
    QList<QObject *> found;
    const QString prefix = QLatin1String(type) + QLatin1Char('_');
    const QList<QObject *> children = root->findChildren<QObject *>();
    for (QObject *child : children) {
        const QString name = QLatin1String(child->metaObject()->className());
        if (name == QLatin1String(type) || name.startsWith(prefix)) {
            found.append(child);
        }
    }
    return found;
}

bool watching(QObject *root, const QString &path)
{
    const QFileInfo file(path);
    const QUrl folder = QUrl::fromLocalFile(file.absolutePath());
    const QList<QAbstractItemModel *> models = root->findChildren<QAbstractItemModel *>();
    return std::any_of(models.cbegin(), models.cend(), [&](QAbstractItemModel *model) {
        return model->inherits("QQuickFolderListModel") && model->property("folder").toUrl() == folder
            && model->property("status").toInt() == 1 && model->property("nameFilters").toStringList().contains(file.fileName());
    });
}

QList<QQuickItem *> visibleItems(QQuickItem *item, const char *type)
{
    QList<QQuickItem *> found;
    if (!item->isVisible()) {
        return found;
    }
    const QString name = QLatin1String(item->metaObject()->className());
    if (name == QLatin1String(type) || name.startsWith(QLatin1String(type) + QLatin1Char('_'))) {
        found.append(item);
    }
    const QList<QQuickItem *> children = item->childItems();
    for (QQuickItem *child : children) {
        found += visibleItems(child, type);
    }
    return found;
}

QList<QQuickItem *> shownDelegates(QQuickItem *view)
{
    QList<QQuickItem *> shown;
    const QRectF viewport = view->mapRectToScene(view->boundingRect());
    const QList<QQuickItem *> children = view->property("contentItem").value<QQuickItem *>()->childItems();
    for (QQuickItem *child : children) {
        QVariant index = child->property("index");
        if (auto *model = child->property("model").value<QObject *>(); !index.isValid() && model) {
            index = model->property("index");
        }
        QQuickItem *placed = nullptr;
        if (index.isValid()) {
            QMetaObject::invokeMethod(view, "itemAtIndex", Q_RETURN_ARG(QQuickItem *, placed), Q_ARG(int, index.toInt()));
        }
        if (placed == child && child->isVisible() && child->mapRectToScene(child->boundingRect()).intersects(viewport)) {
            shown.append(child);
        }
    }
    return shown;
}

QObject *withText(const QList<QObject *> &objects, const QString &text)
{
    const auto found
        = std::find_if(objects.cbegin(), objects.cend(), [&](QObject *object) { return object->property("text").toString() == text; });
    return found == objects.cend() ? nullptr : *found;
}

void clickAt(QQuickItem *item, Qt::MouseButton button, QPointF local)
{
    if (local.x() < 0) {
        local = QPointF(item->width() / 2, item->height() / 2);
    }
    QQuickTest::qWaitForPolish(item->window());
    QTest::mouseClick(item->window(), button, {}, item->mapToScene(local).toPoint());
}
