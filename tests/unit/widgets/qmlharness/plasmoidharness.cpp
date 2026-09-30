#include "plasmoidharness.h"

#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QAbstractItemModel>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlExpression>
#include <QRegularExpression>
#include <QTest>
#include <QXmlStreamReader>

#include <cstdio>

namespace
{

QtMessageHandler previousHandler = nullptr;

void collectMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QLatin1String category(context.category ? context.category : "default");
    const bool fromQml = category == QLatin1String("default") || category == QLatin1String("qml") || category == QLatin1String("js");
    const bool offscreenLimit = message.startsWith(QLatin1String("This plugin does not support "));
    if (type != QtDebugMsg && type != QtInfoMsg && fromQml && !offscreenLimit) {
        PlasmoidHarness::messages().append(message);
    }
    if (previousHandler) {
        previousHandler(type, context, message);
    }
}

void replaceFile(const QString &source, const QString &target)
{
    QDir().mkpath(QFileInfo(target).absolutePath());
    QFile::remove(target);
    QFile::copy(source, target);
}

void copyTree(const QString &from, const QString &to)
{
    QDirIterator it(from, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString source = it.next();
        replaceFile(source, to + source.mid(from.size()));
    }
}

QVariant kcfgValue(const QString &type, const QString &text)
{
    if (type == QLatin1String("Bool")) {
        return text.trimmed() == QLatin1String("true");
    }
    if (type == QLatin1String("Int") || type == QLatin1String("UInt")) {
        return text.trimmed().toInt();
    }
    if (type == QLatin1String("Double")) {
        return text.trimmed().toDouble();
    }
    return text;
}

}

PlasmoidHarness::PlasmoidHarness(PlasmoidSpec spec)
    : m_spec(std::move(spec))
{ }

PlasmoidHarness::~PlasmoidHarness()
{
    unload();
    m_window.reset();
    m_engine.reset();
    m_plasmoid.reset();
    qInstallMessageHandler(previousHandler);
    previousHandler = nullptr;
    DataSourceDouble::commands().clear();
    SessionBusDouble::calls().clear();
}

void PlasmoidHarness::prepareEnvironment()
{
    static QTemporaryDir home;
    const QString root = home.path();
    const QList<QPair<const char *, QString>> paths {
        {"HOME", root},
        {"XDG_CONFIG_HOME", root + QStringLiteral("/.config")},
        {"XDG_DATA_HOME", root + QStringLiteral("/.local/share")},
        {"XDG_CACHE_HOME", root + QStringLiteral("/.cache")},
        {"XDG_STATE_HOME", root + QStringLiteral("/.local/state")},
        {"XDG_RUNTIME_DIR", root + QStringLiteral("/runtime")},
    };
    for (const auto &[name, path] : paths) {
        QDir().mkpath(path);
        qputenv(name, path.toUtf8());
    }
    QFile::setPermissions(root + QStringLiteral("/runtime"), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    const QString doubles = root + QStringLiteral("/doubles");
    const QStringList modules {QStringLiteral("org.kde.plasma.plasmoid"), QStringLiteral("org.kde.plasma.plasma5support"),
        QStringLiteral("org.kde.taskmanager"), QStringLiteral("org.kde.plasma.workspace.dbus")};
    for (const QString &module : modules) {
        const QString directory = doubles + QLatin1Char('/') + QString(module).replace(QLatin1Char('.'), QLatin1Char('/'));
        QDir().mkpath(directory);
        QFile qmldir(directory + QStringLiteral("/qmldir"));
        if (qmldir.open(QIODevice::WriteOnly)) {
            qmldir.write("module " + module.toUtf8() + "\n");
        }
    }
    qputenv("QML_IMPORT_PATH", doubles.toUtf8());
    qputenv("XDG_CONFIG_DIRS", QString(root + QStringLiteral("/etc")).toUtf8());
    qputenv("QML_XHR_ALLOW_FILE_READ", "1");
    qputenv("TZ", "UTC");
    qputenv("QT_QPA_PLATFORM", "offscreen");
    KLocalizedString::setApplicationDomain("konveyor-widget-tests");
}

QString PlasmoidHarness::widgetsDir()
{
    return QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets");
}

QByteArray PlasmoidHarness::fixture(const QString &name)
{
    QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/tests/unit/widgets/fixtures/") + name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

std::unique_ptr<PlasmoidHarness> PlasmoidHarness::started(const PlasmoidSpec &spec, int formFactor, const QVariantMap &config)
{
    auto harness = std::make_unique<PlasmoidHarness>(spec);
    if (!harness->load(formFactor, config) || !harness->show("compactRepresentation") || !harness->show("fullRepresentation")) {
        qWarning("%s", qPrintable(harness->error));
        return {};
    }
    harness->resolveRuntime(QStringLiteral("printf %s"));
    return harness;
}

bool PlasmoidHarness::overlay(const QByteArray &targets, bool visible) const
{
    QObject *root = m_root.get();
    return deliver(runtimePath(QStringLiteral("Konveyor-Monitor-Overlay/state.json")), "{\"targets\": " + targets + "}",
        [root, visible] { return root->property("overlayVisible").toBool() == visible; });
}

QStringList &PlasmoidHarness::messages()
{
    static QStringList list;
    return list;
}

void PlasmoidHarness::stage()
{
    const QString ui = m_dir.path() + QStringLiteral("/contents/ui");
    copyTree(widgetsDir() + QLatin1Char('/') + m_spec.ui + QStringLiteral("/contents"), m_dir.path() + QStringLiteral("/contents"));
    QDir().mkpath(ui + QStringLiteral("/lib"));
    const QDir common(widgetsDir() + QStringLiteral("/shared/common"));
    for (const QString &name : common.entryList({QStringLiteral("*.qml"), QStringLiteral("*.js")}, QDir::Files)) {
        replaceFile(common.filePath(name), ui + QStringLiteral("/lib/") + name);
    }
    const QDir shared(widgetsDir() + QStringLiteral("/shared"));
    for (const QString &name : shared.entryList({QStringLiteral("*.qml")}, QDir::Files)) {
        replaceFile(shared.filePath(name), ui + QStringLiteral("/lib/") + name);
    }
    for (const QString &lib : std::as_const(m_spec.libs)) {
        copyTree(widgetsDir() + QLatin1Char('/') + lib, ui + QStringLiteral("/lib"));
    }
}

void PlasmoidHarness::loadConfig()
{
    QFile file(stagedPath(QStringLiteral("contents/config/main.xml")));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QXmlStreamReader xml(&file);
    QString name;
    QString type;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) {
            continue;
        }
        if (xml.name() == QLatin1String("entry")) {
            name = xml.attributes().value(QLatin1String("name")).toString();
            type = xml.attributes().value(QLatin1String("type")).toString();
            m_plasmoid->configuration()->insert(name, kcfgValue(type, QString()));
        } else if (xml.name() == QLatin1String("default")) {
            m_plasmoid->configuration()->insert(name, kcfgValue(type, xml.readElementText()));
        }
    }
}

void PlasmoidHarness::setUp(int formFactor, const QVariantMap &config)
{
    stage();
    registerPlasmaDoubles();
    messages().clear();
    previousHandler = qInstallMessageHandler(collectMessage);
    m_engine = std::make_unique<QQmlEngine>();
    m_engine->rootContext()->setContextObject(new KLocalizedQmlContext(m_engine.get()));
    QObject::connect(m_engine.get(), &QQmlEngine::warnings, m_engine.get(), [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) {
            messages().append(warning.toString());
        }
    });
    m_engine->setOutputWarningsToStandardError(false);
    m_plasmoid = std::make_unique<PlasmoidDouble>(m_spec.pluginId, m_spec.iconName);
    m_plasmoid->formFactor = formFactor;
    PlasmoidAttachedType::current = m_plasmoid.get();
    loadConfig();
    for (auto it = config.begin(); it != config.end(); ++it) {
        m_plasmoid->configuration()->insert(it.key(), it.value());
    }
    m_window = std::make_unique<QQuickWindow>();
    m_window->resize(800, 900);
}

QObject *PlasmoidHarness::create(const QString &relative, const QVariantMap &properties)
{
    QQmlComponent component(m_engine.get(), QUrl::fromLocalFile(stagedPath(relative)));
    m_root.reset(component.createWithInitialProperties(properties));
    if (!m_root) {
        error = component.errorString();
        return nullptr;
    }
    if (auto *item = qobject_cast<QQuickItem *>(m_root.get())) {
        item->setParentItem(m_window->contentItem());
        item->setSize(m_window->size());
    }
    m_window->show();
    return m_root.get();
}

QObject *PlasmoidHarness::load(int formFactor, const QVariantMap &config)
{
    setUp(formFactor, config);
    return create(QStringLiteral("contents/ui/main.qml"));
}

QQuickItem *PlasmoidHarness::showItem(QObject *object)
{
    auto *item = qobject_cast<QQuickItem *>(object);
    if (item) {
        item->setParentItem(m_window->contentItem());
        item->setSize(m_window->size());
    }
    return item;
}

QQuickItem *PlasmoidHarness::show(const char *representation)
{
    auto *component = m_root->property(representation).value<QQmlComponent *>();
    if (!component) {
        error = QStringLiteral("no %1").arg(QLatin1String(representation));
        return nullptr;
    }
    QObject *object = component->create(component->creationContext());
    if (!object) {
        error = component->errorString();
        return nullptr;
    }
    m_shown.append(object);
    return showItem(object);
}

QString PlasmoidHarness::command(const QString &prefix) const
{
    const QStringList &all = DataSourceDouble::commands();
    for (auto it = all.crbegin(); it != all.crend(); ++it) {
        if (it->startsWith(prefix)) {
            return *it;
        }
    }
    return {};
}

bool PlasmoidHarness::reply(const QString &prefix, const QString &out, int exitCode, const QString &err)
{
    const QList<DataSourceDouble *> sources = DataSourceDouble::instances();
    for (DataSourceDouble *source : sources) {
        const QStringList connected = source->connectedSources();
        for (const QString &command : connected) {
            if (command.startsWith(prefix)) {
                source->reply(command, out, exitCode, err);
                return true;
            }
        }
    }
    return false;
}

void PlasmoidHarness::resolveRuntime(const QString &prefix)
{
    QString path = command(prefix);
    static const QRegularExpression quoted(QStringLiteral("\"([^\"]*)\""));
    path = quoted.match(path).captured(1);
    path.replace(QStringLiteral("$XDG_RUNTIME_DIR"), qEnvironmentVariable("XDG_RUNTIME_DIR"));
    reply(prefix, path + QLatin1Char('\n'));
}

void PlasmoidHarness::writeFile(const QString &path, const QByteArray &content) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path + QStringLiteral(".tmp"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content);
        file.close();
        std::rename(QFile::encodeName(file.fileName()).constData(), QFile::encodeName(path).constData());
    }
}

bool PlasmoidHarness::deliver(const QString &path, const QByteArray &content, const std::function<bool()> &arrived) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (!QTest::qWaitFor([&] { return watching(path); })) {
        qWarning("nothing watches %s", qPrintable(path));
        return false;
    }
    writeFile(path, content);
    return QTest::qWaitFor(arrived);
}

void PlasmoidHarness::unload()
{
    qDeleteAll(m_shown);
    m_shown.clear();
    m_root.reset();
}

QString PlasmoidHarness::runtimePath(const QString &name) const
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QLatin1Char('/') + name;
}

QString PlasmoidHarness::stagedPath(const QString &relative) const
{
    return m_dir.filePath(relative);
}

QVariant PlasmoidHarness::eval(const QString &expression, QObject *scope) const
{
    QObject *target = scope ? scope : m_root.get();
    QQmlExpression evaluation(qmlContext(target), target, expression);
    const QVariant result = evaluation.evaluate();
    if (evaluation.hasError()) {
        messages().append(evaluation.error().toString());
    }
    return result.canConvert<QJSValue>() ? result.value<QJSValue>().toVariant() : result;
}

QVariant PlasmoidHarness::config(const QString &key) const
{
    return m_plasmoid->configuration()->value(key);
}

void PlasmoidHarness::setConfig(const QString &key, const QVariant &value)
{
    m_plasmoid->configuration()->insert(key, value);
}

QList<QObject *> PlasmoidHarness::findAll(const char *type) const
{
    QList<QObject *> found = findByType(m_root.get(), type);
    for (QObject *shown : m_shown) {
        found += findByType(shown, type);
    }
    return found;
}

QQuickItem *PlasmoidHarness::listOf(const QString &model) const
{
    const QList<QObject *> views = findAll("QQuickListView");
    QObject *wanted = eval(model).value<QObject *>();
    const auto found
        = std::find_if(views.cbegin(), views.cend(), [&](QObject *view) { return view->property("model").value<QObject *>() == wanted; });
    return found == views.cend() ? nullptr : qobject_cast<QQuickItem *>(*found);
}

bool PlasmoidHarness::watching(const QString &path) const
{
    if (::watching(m_root.get(), path)) {
        return true;
    }
    return std::any_of(m_shown.cbegin(), m_shown.cend(), [&](QObject *shown) { return ::watching(shown, path); });
}

QQuickItem *PlasmoidHarness::scene() const
{
    return m_window->contentItem();
}

QString PlasmoidHarness::report()
{
    return messages().join(QLatin1Char('\n'));
}
