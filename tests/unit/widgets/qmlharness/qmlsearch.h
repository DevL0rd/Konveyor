#pragma once

#include <QQuickItem>
#include <QStringList>

#include <functional>

QStringList visibleTexts(QQuickItem *item);
QStringList plainTexts(QQuickItem *item);
QList<QObject *> findByType(QObject *root, const char *type);
QList<QQuickItem *> visibleItems(QQuickItem *item, const char *type);
bool watching(QObject *root, const QString &path);
int delegateIndex(QQuickItem *view, QQuickItem *delegate);
QList<QQuickItem *> shownDelegates(QQuickItem *view);
using RowCheck = std::function<QString(QQuickItem *row, int index)>;
QString rowMismatch(QQuickItem *view, const RowCheck &check);
bool rowsMatch(QQuickItem *view, const RowCheck &check);
QQuickItem *delegateWith(QQuickItem *view, const char *property, const QVariant &value);
QObject *withText(const QList<QObject *> &objects, const QString &text);
void clickAt(QQuickItem *item, Qt::MouseButton button, QPointF local = {-1, -1});
