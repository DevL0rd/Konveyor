#include "store/settingsstore.h"

#include "config/loader.h"
#include "document/nodecodec.h"
#include "store/configfile.h"
#include "system/keynames.h"
#include "values/configvalues.h"
#include "values/livematching.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace Konveyor::Settings
{

namespace
{

constexpr int SaveDelayMs = 350;

QVariantMap leaf(const QString &path, const QVariantList &arguments, const QVariantMap &properties)
{
    return {
        {QStringLiteral("name"), path.section(QLatin1Char('/'), -1).section(QLatin1Char('#'), 0, 0)},
        {QStringLiteral("args"), arguments},
        {QStringLiteral("props"), properties},
    };
}

}

SettingsStore::SettingsStore(QObject *parent)
    : QObject(parent)
    , m_live(new LiveSession(this))
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &SettingsStore::save);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &SettingsStore::fileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &SettingsStore::fileChanged);
    load();
}

int SettingsStore::revision() const
{
    return m_revision;
}

QString SettingsStore::configError() const
{
    return m_configError;
}

QString SettingsStore::configPath() const
{
    return Config::configPath();
}

QVariantMap SettingsStore::values() const
{
    return m_values;
}

LiveSession *SettingsStore::live() const
{
    return m_live;
}

bool SettingsStore::needsSave() const
{
    return m_document.text() != m_savedText;
}

bool SettingsStore::representsDefaults() const
{
    return !m_defaultText.isEmpty() && m_document.text() == m_defaultText;
}

bool SettingsStore::canUndo() const
{
    return m_history.canUndo();
}

bool SettingsStore::autoSave() const
{
    return m_autoSave;
}

void SettingsStore::setAutoSave(bool autoSave)
{
    if (m_autoSave == autoSave) {
        return;
    }
    m_autoSave = autoSave;
    Q_EMIT autoSaveChanged();
}

void SettingsStore::load()
{
    const QString defaultPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konveyor/default-config.kdl"));
    m_defaultText = readConfigText(defaultPath).value_or(QString());
    m_defaults = ConfigDocument(m_defaultText);
    m_savedText = readConfigText(configPath()).value_or(m_defaultText);
    m_history.clear();
    replaceText(m_savedText);
    watch();
    if (defaultPath.isEmpty()) {
        Q_EMIT editFailed(QStringLiteral("The default Konveyor config is not installed, so Defaults is unavailable."));
    }
}

void SettingsStore::save()
{
    m_saveTimer.stop();
    if (!m_configError.isEmpty()) {
        Q_EMIT editFailed(QStringLiteral("Not saved, the config has an error: ") + m_configError);
        return;
    }
    const QString text = m_document.text();
    if (auto written = writeConfigText(configPath(), text); !written) {
        Q_EMIT editFailed(written.error());
        return;
    }
    m_savedText = text;
    watch();
    refresh();
    Q_EMIT saved();
}

void SettingsStore::defaults()
{
    if (m_defaultText.isEmpty()) {
        Q_EMIT editFailed(QStringLiteral("The default Konveyor config is not installed."));
        return;
    }
    const QString before = m_document.text();
    replaceText(m_defaultText);
    apply(EditResult(QString()), before);
}

void SettingsStore::undo()
{
    const std::optional<QString> previous = m_history.undo();
    if (!previous) {
        return;
    }
    replaceText(*previous);
    if (m_autoSave) {
        m_saveTimer.start();
    }
}

bool SettingsStore::has(const QString &path) const
{
    return m_document.find(path) != nullptr;
}

bool SettingsStore::isDefault(const QString &path) const
{
    const Kdl::Node *current = m_document.find(path);
    const Kdl::Node *shipped = m_defaults.find(path);
    if (!current || !shipped) {
        return !current && !shipped;
    }
    return writeNode(nodeToVariant(*current), QString()) == writeNode(nodeToVariant(*shipped), QString());
}

bool SettingsStore::resetToDefault(const QString &path)
{
    const Kdl::Node *shipped = m_defaults.find(path);
    if (!shipped) {
        return remove(path);
    }
    return setNode(path, nodeToVariant(*shipped));
}

QVariantMap SettingsStore::node(const QString &path) const
{
    const Kdl::Node *found = m_document.find(path);
    return found ? nodeToVariant(*found) : QVariantMap();
}

QVariantList SettingsStore::children(const QString &parentPath, const QString &name) const
{
    return m_document.childPaths(parentPath, name);
}

QVariantMap SettingsStore::scope(const QString &layoutPath) const
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

bool SettingsStore::setNode(const QString &path, const QVariantMap &node)
{
    const QString before = m_document.text();
    return apply(m_document.setNode(path, node), before);
}

bool SettingsStore::setValue(const QString &path, const QVariantList &arguments, const QVariantMap &properties)
{
    const QString before = m_document.text();
    return apply(m_document.setNode(path, leaf(path, arguments, properties)), before);
}

bool SettingsStore::setFlag(const QString &path, bool enabled)
{
    return enabled ? setValue(path, {}) : remove(path);
}

bool SettingsStore::setToggle(const QString &path, bool enabled)
{
    const QString before = m_document.text();
    const bool removed
        = apply(m_document.remove(path + QStringLiteral("/on")), before) && apply(m_document.remove(path + QStringLiteral("/off")), before);
    return removed && apply(m_document.append(path, leaf(enabled ? QStringLiteral("on") : QStringLiteral("off"), {}, {})), before);
}

bool SettingsStore::remove(const QString &path)
{
    const QString before = m_document.text();
    return apply(m_document.remove(path), before);
}

QString SettingsStore::append(const QString &parentPath, const QVariantMap &node)
{
    const QString before = m_document.text();
    const EditResult result = m_document.append(parentPath, node);
    return apply(result, before) ? *result : QString();
}

QString SettingsStore::move(const QString &path, int delta)
{
    const QString before = m_document.text();
    const EditResult result = m_document.move(path, delta);
    return apply(result, before) ? *result : QString();
}

QString SettingsStore::keyName(const QKeySequence &sequence) const
{
    return bindKeyName(sequence, m_values.value(QStringLiteral("input/mod-key")).toString());
}

QVariantMap SettingsStore::checkRule(const QVariantMap &ruleNode) const
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

QString SettingsStore::profileForOutput(const QVariantMap &output) const
{
    const auto current = Config::loadString(m_document.text(), configPath());
    return current ? profileNameFor(current->config, output) : QString();
}

void SettingsStore::openConfigFile() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(configPath()));
}

bool SettingsStore::apply(const EditResult &result, const QString &before)
{
    if (!result) {
        Q_EMIT editFailed(result.error());
        return false;
    }
    if (m_document.text() != before) {
        m_history.record(before);
    }
    refresh();
    if (m_autoSave && needsSave()) {
        m_saveTimer.start();
    }
    return true;
}

void SettingsStore::replaceText(const QString &text)
{
    m_document = ConfigDocument(text);
    refresh();
}

void SettingsStore::refresh()
{
    auto loaded = Config::loadString(m_document.text(), configPath());
    m_configError = loaded ? QString() : loaded.error().toString();
    if (loaded) {
        m_values = globalValues(loaded->config);
    }
    ++m_revision;
    Q_EMIT documentChanged();
}

void SettingsStore::watch()
{
    const QString path = configPath();
    const QString folder = QFileInfo(path).path();
    if (!m_watcher.directories().contains(folder) && QFileInfo::exists(folder)) {
        m_watcher.addPath(folder);
    }
    if (!m_watcher.files().contains(path) && QFileInfo::exists(path)) {
        m_watcher.addPath(path);
    }
}

void SettingsStore::fileChanged()
{
    watch();
    const std::optional<QString> disk = readConfigText(configPath());
    if (!disk || *disk == m_savedText || m_saveTimer.isActive() || needsSave()) {
        return;
    }
    m_savedText = *disk;
    m_history.clear();
    replaceText(*disk);
}

}
