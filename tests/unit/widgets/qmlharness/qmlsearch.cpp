#include "plasmoidharness.h"

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QQuickItem>

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
