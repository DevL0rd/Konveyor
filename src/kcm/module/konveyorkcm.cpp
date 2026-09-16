#include "module/konveyorkcm.h"

#include "config/loader.h"
#include "document/nodecodec.h"
#include "system/keynames.h"
#include "system/livesession.h"
#include "values/configvalues.h"
#include "values/livematching.h"

#include <KPluginFactory>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

K_PLUGIN_CLASS_WITH_JSON(Konveyor::Settings::KonveyorKcm, "kcm_konveyor.json")

namespace Konveyor::Settings
{

namespace
{

std::optional<QString> readText(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    return QString::fromUtf8(file.readAll());
}

QVariantMap leaf(const QString &path, const QVariantList &arguments, const QVariantMap &properties)
{
    return {
        {QStringLiteral("name"), path.section(QLatin1Char('/'), -1).section(QLatin1Char('#'), 0, 0)},
        {QStringLiteral("args"), arguments},
        {QStringLiteral("props"), properties},
    };
}

}

KonveyorKcm::KonveyorKcm(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : KQuickConfigModule(parent, metaData)
    , m_initialPage(args.value(0).toString())
    , m_live(new LiveSession(this))
{
    setButtons(Apply | Default);
    KonveyorKcm::load();
}

int KonveyorKcm::revision() const
{
    return m_revision;
}

QString KonveyorKcm::configError() const
{
    return m_configError;
}

QString KonveyorKcm::configPath() const
{
    return Config::configPath();
}

QString KonveyorKcm::initialPage() const
{
    return m_initialPage;
}

QVariantMap KonveyorKcm::values() const
{
    return m_values;
}

LiveSession *KonveyorKcm::live() const
{
    return m_live;
}

void KonveyorKcm::load()
{
    const QString defaultPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konveyor/default-config.kdl"));
    m_defaultText = readText(defaultPath).value_or(QString());
    m_defaults = ConfigDocument(m_defaultText);
    const std::optional<QString> current = readText(configPath());
    m_savedText = current.value_or(m_defaultText);
    setDocumentText(m_savedText);
    if (defaultPath.isEmpty()) {
        Q_EMIT editFailed(QStringLiteral("The default Konveyor config is not installed, so Defaults is unavailable."));
    }
}

void KonveyorKcm::save()
{
    if (!m_configError.isEmpty()) {
        Q_EMIT editFailed(QStringLiteral("Not saved, the config has an error: ") + m_configError);
        return;
    }
    const QString path = configPath();
    QDir().mkpath(QFileInfo(path).path());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Q_EMIT editFailed(QStringLiteral("Could not write %1: %2").arg(path, file.errorString()));
        return;
    }
    file.write(m_document.text().toUtf8());
    if (!file.commit()) {
        Q_EMIT editFailed(QStringLiteral("Could not write %1: %2").arg(path, file.errorString()));
        return;
    }
    m_savedText = m_document.text();
    refresh();
}

void KonveyorKcm::defaults()
{
    if (m_defaultText.isEmpty()) {
        Q_EMIT editFailed(QStringLiteral("The default Konveyor config is not installed."));
        return;
    }
    setDocumentText(m_defaultText);
}

bool KonveyorKcm::has(const QString &path) const
{
    return m_document.find(path) != nullptr;
}

bool KonveyorKcm::isDefault(const QString &path) const
{
    const Kdl::Node *current = m_document.find(path);
    const Kdl::Node *shipped = m_defaults.find(path);
    if (!current || !shipped) {
        return !current && !shipped;
    }
    return writeNode(nodeToVariant(*current), QString()) == writeNode(nodeToVariant(*shipped), QString());
}

bool KonveyorKcm::resetToDefault(const QString &path)
{
    const Kdl::Node *shipped = m_defaults.find(path);
    if (!shipped) {
        return remove(path);
    }
    return setNode(path, nodeToVariant(*shipped));
}

QVariantMap KonveyorKcm::node(const QString &path) const
{
    const Kdl::Node *found = m_document.find(path);
    return found ? nodeToVariant(*found) : QVariantMap();
}

QVariantList KonveyorKcm::children(const QString &parentPath, const QString &name) const
{
    return m_document.childPaths(parentPath, name);
}

QVariantMap KonveyorKcm::scope(const QString &layoutPath) const
{
    auto loaded = Config::loadString(m_document.text(), configPath());
    if (!loaded) {
        return {};
    }
    const QString owner = layoutPath.section(QLatin1Char('/'), 0, -2);
    const Kdl::Node *ownerNode = owner.isEmpty() ? nullptr : m_document.find(owner);
    const QString ownerName = ownerNode && !ownerNode->arguments.isEmpty() && ownerNode->arguments.first().isString()
        ? ownerNode->arguments.first().toString()
        : QString();
    return layoutValues(scopedLayout(loaded->config, owner.section(QLatin1Char('#'), 0, 0), ownerName));
}

bool KonveyorKcm::setNode(const QString &path, const QVariantMap &node)
{
    return apply(m_document.setNode(path, node));
}

bool KonveyorKcm::setValue(const QString &path, const QVariantList &arguments, const QVariantMap &properties)
{
    return apply(m_document.setNode(path, leaf(path, arguments, properties)));
}

bool KonveyorKcm::setFlag(const QString &path, bool enabled)
{
    return enabled ? setValue(path, {}) : remove(path);
}

bool KonveyorKcm::setToggle(const QString &path, bool enabled)
{
    const bool removed = apply(m_document.remove(path + QStringLiteral("/on"))) && apply(m_document.remove(path + QStringLiteral("/off")));
    return removed && apply(m_document.append(path, leaf(enabled ? QStringLiteral("on") : QStringLiteral("off"), {}, {})));
}

bool KonveyorKcm::remove(const QString &path)
{
    return apply(m_document.remove(path));
}

QString KonveyorKcm::append(const QString &parentPath, const QVariantMap &node)
{
    const EditResult result = m_document.append(parentPath, node);
    return apply(result) ? *result : QString();
}

QString KonveyorKcm::move(const QString &path, int delta)
{
    const EditResult result = m_document.move(path, delta);
    return apply(result) ? *result : QString();
}

QString KonveyorKcm::keyName(const QKeySequence &sequence) const
{
    return bindKeyName(sequence, m_values.value(QStringLiteral("input/mod-key")).toString());
}

QVariantMap KonveyorKcm::checkRule(const QVariantMap &ruleNode) const
{
    const auto current = Config::loadString(m_document.text(), configPath());
    if (!current) {
        return {{QStringLiteral("error"), current.error().message}};
    }
    const auto rule = Config::loadString(writeNode(ruleNode, QString()), QStringLiteral("rule.kdl"));
    if (!rule || rule->config.windowRules.isEmpty()) {
        return {{QStringLiteral("error"), rule ? QStringLiteral("not a window rule") : rule.error().message}};
    }
    return {{QStringLiteral("windows"),
        windowsMatchingRule(
            current->config, rule->config.windowRules.first(), m_live->windows(), m_live->workspaces(), m_live->outputs())}};
}

QString KonveyorKcm::profileForOutput(const QVariantMap &output) const
{
    const auto current = Config::loadString(m_document.text(), configPath());
    return current ? profileNameFor(current->config, output) : QString();
}

void KonveyorKcm::openConfigFile() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(configPath()));
}

bool KonveyorKcm::apply(const EditResult &result)
{
    if (!result) {
        Q_EMIT editFailed(result.error());
        return false;
    }
    refresh();
    return true;
}

void KonveyorKcm::setDocumentText(const QString &text)
{
    m_document = ConfigDocument(text);
    refresh();
}

void KonveyorKcm::refresh()
{
    const QString text = m_document.text();
    auto loaded = Config::loadString(text, configPath());
    m_configError = loaded ? QString() : loaded.error().toString();
    if (loaded) {
        m_values = globalValues(loaded->config);
    }
    setNeedsSave(text != m_savedText);
    setRepresentsDefaults(!m_defaultText.isEmpty() && text == m_defaultText);
    ++m_revision;
    Q_EMIT documentChanged();
}

}

#include "konveyorkcm.moc"
