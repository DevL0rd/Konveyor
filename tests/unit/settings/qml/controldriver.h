#pragma once

#include "settingsqmlharness.h"

#include <QQuickWindow>

namespace Konveyor::Settings::Testing
{

constexpr QByteArrayView ControlDriver = R"(
import QtQuick.Window
Window {
    width: 1000
    height: 900
    visible: true
    property Item page: null
    function open(type, properties) {
        const component = Qt.createComponent("org.kde.konveyor.settings", type);
        page = component.createObject(contentItem, Object.assign({ width: width, height: height }, properties));
        return page !== null;
    }
    function items(root) {
        const found = [];
        const seen = new Set();
        const visit = item => {
            if (!item || seen.has(item)) {
                return;
            }
            seen.add(item);
            found.push(item);
            for (const child of (item.children || [])) {
                visit(child);
            }
            visit(item.contentItem);
            for (const entry of (item.data || [])) {
                visit(entry);
            }
        };
        visit(root);
        return found;
    }
    function drive(label, nth, signal, args) {
        const all = items(page);
        const byTitle = label.startsWith("^");
        const text = byTitle ? label.slice(1) : label;
        let rows = byTitle ? [] : all.filter(item => item.label === text);
        if (!rows.length) {
            rows = all.filter(item => item.title === text || item.text === text || item.heading === text);
        }
        const row = rows[nth];
        if (!row) {
            return "no control labelled " + label + " #" + nth;
        }
        const calls = signal.split(",");
        const [name, which] = calls[0].split("@");
        const slots = Array.from(row.editor || []).concat(Array.from(row.control || []));
        const inSlots = [].concat(...slots.map(items));
        const target = inSlots.concat(items(row)).filter(item => typeof item[name] === "function")[Number(which || 0)];
        if (!target) {
            return "no " + calls[0] + " under " + label;
        }
        for (const call of [name].concat(calls.slice(1))) {
            target[call](...(call === name ? args : []));
        }
        return "";
    }
}
)";

inline QVariant parsedJson(const QByteArray &json)
{
    return QJsonDocument::fromJson("[" + json + "]").array().first().toVariant();
}

inline QByteArray jsonText(const QVariant &value)
{
    return QJsonDocument(QJsonArray {QJsonValue::fromVariant(value)}).toJson(QJsonDocument::Compact);
}

struct Control
{
    const char *page;
    const char *label;
    const char *signal;
    const char *arguments;
    const char *path;
    const char *expected;
    const char *config = nullptr;
    int nth = 0;
    const char *properties = "{}";
};

inline void addControlColumns()
{
    QTest::addColumn<QString>("page");
    QTest::addColumn<QByteArray>("properties");
    QTest::addColumn<std::optional<QString>>("config");
    QTest::addColumn<QString>("label");
    QTest::addColumn<int>("nth");
    QTest::addColumn<QString>("signal");
    QTest::addColumn<QByteArray>("arguments");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QByteArray>("expected");
}

inline void addControl(const char *name, const Control &control)
{
    QTest::newRow(name) << QString::fromLatin1(control.page) << QByteArray(control.properties)
                        << (control.config ? std::optional(QString::fromUtf8(control.config)) : std::nullopt)
                        << QString::fromUtf8(control.label) << control.nth << QString::fromLatin1(control.signal)
                        << QByteArray(control.arguments) << QString::fromUtf8(control.path) << QByteArray(control.expected);
}

inline QVariantMap expectedPart(const QVariantMap &node, const QVariantMap &expected)
{
    QVariantMap actual;
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        if (it.key() == QLatin1String("children")) {
            QVariantList names;
            for (const QVariant &child : node.value(QStringLiteral("children")).toList()) {
                names.append(child.toMap().value(QStringLiteral("name")));
            }
            actual.insert(it.key(), names);
        } else {
            actual.insert(it.key(), node.value(it.key()));
        }
    }
    return actual;
}

inline void runControl(const SettingsHome &home)
{
    QFETCH(QString, page);
    QFETCH(QByteArray, properties);
    QFETCH(std::optional<QString>, config);
    QFETCH(QString, label);
    QFETCH(int, nth);
    QFETCH(QString, signal);
    QFETCH(QByteArray, arguments);
    QFETCH(QString, path);
    QFETCH(QByteArray, expected);
    home.resetConfig(config);
    WarningLog log;
    QQmlEngine engine;
    QString error;
    const std::unique_ptr<QObject> driver = home.create(engine, ControlDriver.toByteArray(), &error);
    QVERIFY2(driver, qPrintable(error));
    QVERIFY(QTest::qWaitForWindowExposed(qobject_cast<QQuickWindow *>(driver.get())));
    QObject *store = SettingsHome::store(engine);
    QVariant opened;
    QVERIFY(QMetaObject::invokeMethod(
        driver.get(), "open", Q_RETURN_ARG(QVariant, opened), Q_ARG(QVariant, page), Q_ARG(QVariant, parsedJson(properties))));
    QVERIFY(opened.toBool());
    QSignalSpy failed(store, SIGNAL(editFailed(QString)));
    QVariant problem;
    QVERIFY(QMetaObject::invokeMethod(driver.get(), "drive", Q_RETURN_ARG(QVariant, problem), Q_ARG(QVariant, label), Q_ARG(QVariant, nth),
        Q_ARG(QVariant, signal), Q_ARG(QVariant, parsedJson(arguments))));
    QCOMPARE(problem.toString(), QString());
    QCOMPARE(failed.count(), 0);
    QCOMPARE(store->property("configError").toString(), QString());
    const QVariantMap node = call<QVariantMap>(store, "node", path);
    const QVariant wanted = parsedJson(expected);
    if (wanted.isNull()) {
        QCOMPARE(jsonText(node), QByteArray("[{}]"));
    } else {
        QVERIFY2(!node.isEmpty(), qPrintable(path));
        QCOMPARE(jsonText(expectedPart(node, wanted.toMap())), jsonText(wanted));
    }
    QCOMPARE(log.take(), QStringList {});
}

}
