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
            if (!item || typeof item !== "object" || seen.has(item)) {
                return;
            }
            seen.add(item);
            found.push(item);
            for (const child of (item.children || [])) {
                visit(child);
            }
            visit(item.contentItem);
            const data = item.data;
            for (let index = 0; data && typeof data === "object" && index < (data.length || 0); ++index) {
                visit(data[index]);
            }
        };
        visit(root);
        return found;
    }
    function matchesLabel(item, label) {
        if (label.startsWith("=")) {
            const [key, value] = label.slice(1).includes(":") ? label.slice(1).split(":") : ["path", label.slice(1)];
            return item[key] === value;
        }
        const text = label.startsWith("^") ? label.slice(1) : label;
        return item.title === text || item.text === text || item.heading === text;
    }
    function supports(item, call) {
        const name = call.split("=")[0];
        return item[name] !== undefined && (call.includes("=") || typeof item[name] === "function");
    }
    function targetIn(row, calls, filter) {
        const slots = Array.from(row.editor || []).concat(Array.from(row.control || []));
        const candidates = [].concat(...slots.map(items)).concat(items(row)).filter(item => calls.every(call => supports(item, call)));
        const index = Number(filter || 0);
        return isNaN(index) ? candidates.find(item => item.text === filter) : candidates[index];
    }
    function drive(label, nth, signal, args) {
        const parts = signal.split(";");
        const [first, filter] = parts[0].split("@");
        const calls = [first].concat(parts.slice(1));
        const all = items(page);
        const labelled = label === "*" ? [page] : all.filter(item => !label.startsWith("^") && !label.startsWith("=") && item.label === label);
        const candidates = labelled.length ? labelled : all.filter(item => matchesLabel(item, label));
        const rows = candidates.filter(item => targetIn(item, calls, filter) !== undefined);
        const row = rows[nth];
        if (!row) {
            return "no " + signal + " under " + label + " #" + nth;
        }
        const target = targetIn(row, calls, filter);
        let pending = args;
        for (const call of calls) {
            if (call.includes("=")) {
                const [name, value] = call.split("=");
                target[name] = JSON.parse(value);
            } else {
                target[call](...pending);
                pending = [];
            }
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
    const QStringList labels = label.split(QStringLiteral(" | "));
    const QStringList steps = signal.split(QStringLiteral(" | "));
    const QStringList argumentLists = QString::fromUtf8(arguments).split(QStringLiteral(" | "));
    QCOMPARE(steps.size(), labels.size());
    QCOMPARE(argumentLists.size(), labels.size());
    for (qsizetype step = 0; step < labels.size(); ++step) {
        QVariant problem;
        QVERIFY(QMetaObject::invokeMethod(driver.get(), "drive", Q_RETURN_ARG(QVariant, problem), Q_ARG(QVariant, labels.at(step)),
            Q_ARG(QVariant, step == 0 ? nth : 0), Q_ARG(QVariant, steps.at(step)),
            Q_ARG(QVariant, parsedJson(argumentLists.at(step).toUtf8()))));
        QCOMPARE(problem.toString(), QString());
    }
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
