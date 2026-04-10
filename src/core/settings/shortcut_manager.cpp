#include "shortcut_manager.h"

#include <QtCore/QStringList>
#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtWidgets/QWidget>

#include "core/services/log_service.h"
#include "core/settings/settings_manager.h"

namespace iqtools::core {

namespace {

struct DefaultShortcutDefinition {
    const char* id;
    const char* category;
    const char* description;
    const char* key;
};

constexpr DefaultShortcutDefinition kDefaultAppShortcuts[] = {
    {"file.new", "文件", "新建项目", "Ctrl+N"},
    {"file.open", "文件", "打开文件/项目", "Ctrl+O"},
    {"file.save", "文件", "保存", "Ctrl+S"},
    {"file.saveas", "文件", "另存为", "Ctrl+Shift+S"},
    {"file.quit", "文件", "退出程序", "Ctrl+Q"},
    {"edit.undo", "编辑", "撤销", "Ctrl+Z"},
    {"edit.redo", "编辑", "重做", "Ctrl+Y"},
    {"edit.copy", "编辑", "复制", "Ctrl+C"},
    {"edit.paste", "编辑", "粘贴", "Ctrl+V"},
    {"edit.cut", "编辑", "剪切", "Ctrl+X"},
    {"view.zoomin", "视图", "放大视图", "Ctrl+="},
    {"view.zoomout", "视图", "缩小视图", "Ctrl+-"},
    {"view.reset", "视图", "恢复默认视图", "Ctrl+0"},
    {"tool.run", "工具/执行", "运行当前工具/脚本", "F5"},
    {"tool.stop", "工具/执行", "停止/终止运行", "Shift+F5"},
    {"tool.build", "工具/执行", "编译/构建", "Ctrl+B"},
    {"tool.capture", "工具/执行", "触发截图", "Alt+C"},
    {"tool.clear_log", "工具/执行", "清空输出日志", "Ctrl+L"},
    {"help.doc", "帮助/系统", "查看帮助文档", "F1"},
    {"app.settings", "帮助/系统", "打开系统设置/偏好设置", "Ctrl+,"},
    {"app.search", "帮助/系统", "全局搜索/查找", "Ctrl+F"},
    {"app.find_next", "帮助/系统", "查找下一个", "F3"},
};

}  // namespace

ShortcutManager::ShortcutManager(SettingsManager* settingsManager, QObject* parent)
    : QObject(parent)
    , m_settingsManager(settingsManager)
{
}

void ShortcutManager::loadConfig()
{
    if (m_settingsManager == nullptr) {
        m_persistedShortcuts.clear();
        return;
    }

    m_persistedShortcuts = m_settingsManager->shortcutMappings();

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        const QString portable = m_persistedShortcuts.value(it.key());
        if (portable.isEmpty()) {
            continue;
        }

        const QKeySequence persistedKey = fromPortableText(portable);
        if (persistedKey.isEmpty()) {
            continue;
        }

        const QString conflictId = checkConflict(persistedKey, it.key());
        if (!conflictId.isEmpty()) {
            IQ_LOG_WARNING(QStringLiteral("shortcut"),
                           QStringLiteral("Persisted shortcut conflict: %1 and %2 use %3")
                               .arg(it.key(), conflictId,
                                    persistedKey.toString(QKeySequence::PortableText)));
            continue;
        }

        it->currentKey = persistedKey;
        applyBindings(it.value());
    }
}

bool ShortcutManager::saveConfig()
{
    if (m_settingsManager == nullptr) {
        return false;
    }

    QMap<QString, QString> customizedShortcuts;
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        const ShortcutItem& item = it.value();
        if (item.currentKey.isEmpty() || item.currentKey == item.defaultKey) {
            continue;
        }
        customizedShortcuts.insert(item.id, toPortableText(item.currentKey));
    }

    const bool saved = m_settingsManager->setShortcutMappings(customizedShortcuts);
    if (saved) {
        m_persistedShortcuts = customizedShortcuts;
    }

    return saved;
}

bool ShortcutManager::reserveShortcut(const QString& id,
                                      const QString& category,
                                      const QString& description,
                                      const QKeySequence& defaultKey)
{
    return registerItem(id, category, description, defaultKey, nullptr, nullptr);
}

bool ShortcutManager::registerAction(const QString& id,
                                     const QString& category,
                                     const QString& description,
                                     const QKeySequence& defaultKey,
                                     QAction* action)
{
    if (action == nullptr) {
        return false;
    }

    return registerItem(id, category, description, defaultKey, action, nullptr);
}

bool ShortcutManager::registerShortcut(const QString& id,
                                       const QString& category,
                                       const QString& description,
                                       const QKeySequence& defaultKey,
                                       QWidget* parent,
                                       QObject* receiver,
                                       const char* slot)
{
    if (parent == nullptr || receiver == nullptr || slot == nullptr) {
        return false;
    }

    auto* shortcut = new QShortcut(parent);
    const bool connected = QObject::connect(shortcut, SIGNAL(activated()), receiver, slot);
    if (!connected) {
        delete shortcut;
        return false;
    }

    if (!registerItem(id, category, description, defaultKey, nullptr, shortcut)) {
        delete shortcut;
        return false;
    }

    return true;
}

bool ShortcutManager::registerPluginAction(const QString& pluginId,
                                           const QString& actionId,
                                           const QString& category,
                                           const QString& description,
                                           const QKeySequence& defaultKey,
                                           QAction* action)
{
    const QString id = composePluginShortcutId(pluginId, actionId);
    if (id.isEmpty()) {
        return false;
    }

    return registerAction(id, category, description, defaultKey, action);
}

bool ShortcutManager::registerPluginShortcut(const QString& pluginId,
                                             const QString& actionId,
                                             const QString& category,
                                             const QString& description,
                                             const QKeySequence& defaultKey,
                                             QWidget* parent,
                                             QObject* receiver,
                                             const char* slot)
{
    const QString id = composePluginShortcutId(pluginId, actionId);
    if (id.isEmpty()) {
        return false;
    }

    return registerShortcut(id, category, description, defaultKey,
                            parent, receiver, slot);
}

void ShortcutManager::unregisterPluginShortcuts(const QString& pluginId)
{
    const QString normalizedPlugin = normalizePluginName(pluginId);
    if (normalizedPlugin.isEmpty()) {
        return;
    }

    unregisterPrefix(QStringLiteral("plugin.%1.").arg(normalizedPlugin));
}

void ShortcutManager::unregisterPrefix(const QString& idPrefix)
{
    const QString prefix = idPrefix.trimmed();
    if (prefix.isEmpty()) {
        return;
    }

    QStringList removedIds;
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        if (it.key().startsWith(prefix)) {
            removedIds.push_back(it.key());
        }
    }

    if (removedIds.isEmpty()) {
        return;
    }

    for (const QString& id : removedIds) {
        ShortcutItem item = m_shortcuts.take(id);
        if (item.action != nullptr) {
            item.action->setShortcut(QKeySequence());
        }
        if (item.shortcut != nullptr) {
            item.shortcut->setKey(QKeySequence());
        }
    }

    saveConfig();
}

bool ShortcutManager::updateShortcut(const QString& id, const QKeySequence& newKey)
{
    const QString normalizedId = normalizeId(id);
    if (!m_shortcuts.contains(normalizedId)) {
        return false;
    }

    if (!newKey.isEmpty()) {
        const QString conflict = checkConflict(newKey, normalizedId);
        if (!conflict.isEmpty()) {
            return false;
        }
    }

    ShortcutItem& item = m_shortcuts[normalizedId];
    if (item.currentKey == newKey) {
        return true;
    }

    const QKeySequence previousKey = item.currentKey;
    item.currentKey = newKey;
    applyBindings(item);

    if (!saveConfig()) {
        item.currentKey = previousKey;
        applyBindings(item);
        return false;
    }

    emit shortcutChanged(item.id, item.currentKey);
    return true;
}

QString ShortcutManager::checkConflict(const QKeySequence& key,
                                       const QString& ignoreId) const
{
    if (key.isEmpty()) {
        return QString();
    }

    const QString normalizedIgnoreId = normalizeId(ignoreId);
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        if (it.key() == normalizedIgnoreId) {
            continue;
        }
        if (!it->currentKey.isEmpty() && it->currentKey == key) {
            return it.key();
        }
    }
    return QString();
}

QKeySequence ShortcutManager::shortcut(const QString& id) const
{
    const QString normalizedId = normalizeId(id);
    const auto it = m_shortcuts.constFind(normalizedId);
    if (it == m_shortcuts.constEnd()) {
        return {};
    }
    return it->currentKey;
}

QVector<ShortcutItem> ShortcutManager::allShortcuts() const
{
    QVector<ShortcutItem> shortcuts;
    shortcuts.reserve(m_shortcuts.size());
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        shortcuts.push_back(it.value());
    }
    return shortcuts;
}

void ShortcutManager::resetToDefault(const QString& id)
{
    const QString normalizedId = normalizeId(id);
    if (normalizedId.isEmpty()) {
        resetAllToDefault();
        return;
    }

    auto it = m_shortcuts.find(normalizedId);
    if (it == m_shortcuts.end()) {
        return;
    }

    if (!it->defaultKey.isEmpty()) {
        const QString conflictId = checkConflict(it->defaultKey, normalizedId);
        if (!conflictId.isEmpty()) {
            IQ_LOG_WARNING(QStringLiteral("shortcut"),
                           QStringLiteral("Cannot reset %1 to default. Conflict with %2")
                               .arg(normalizedId, conflictId));
            return;
        }
    }

    it->currentKey = it->defaultKey;
    applyBindings(it.value());
    saveConfig();
    emit shortcutChanged(it->id, it->currentKey);
}

void ShortcutManager::resetAllToDefault()
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        it->currentKey = it->defaultKey;
        applyBindings(it.value());
        emit shortcutChanged(it->id, it->currentKey);
    }
    saveConfig();
}

void ShortcutManager::registerDefaultAppShortcuts()
{
    for (const DefaultShortcutDefinition& shortcut : kDefaultAppShortcuts) {
        const bool ok = reserveShortcut(QString::fromLatin1(shortcut.id),
                                        QString::fromUtf8(shortcut.category),
                                        QString::fromUtf8(shortcut.description),
                                        QKeySequence(QString::fromLatin1(shortcut.key)));
        if (!ok) {
            IQ_LOG_WARNING(QStringLiteral("shortcut"),
                           QStringLiteral("Failed to reserve default shortcut: %1")
                               .arg(QString::fromLatin1(shortcut.id)));
        }
    }
}

bool ShortcutManager::registerItem(const QString& id,
                                   const QString& category,
                                   const QString& description,
                                   const QKeySequence& defaultKey,
                                   QAction* action,
                                   QShortcut* shortcut)
{
    const QString normalizedId = normalizeId(id);
    if (normalizedId.isEmpty()) {
        return false;
    }

    const bool alreadyRegistered = m_shortcuts.contains(normalizedId);
    ShortcutItem item = m_shortcuts.value(normalizedId);

    item.id = normalizedId;
    if (!category.trimmed().isEmpty()) {
        item.category = category;
    }
    if (!description.trimmed().isEmpty()) {
        item.description = description;
    }
    if (!defaultKey.isEmpty() || !alreadyRegistered) {
        item.defaultKey = defaultKey;
    }
    if (action != nullptr && item.action != nullptr && item.action != action) {
        item.action->setShortcut(QKeySequence());
    }
    if (action != nullptr) {
        item.action = action;
    }
    if (shortcut != nullptr && item.shortcut != nullptr && item.shortcut != shortcut) {
        item.shortcut->setKey(QKeySequence());
    }
    if (shortcut != nullptr) {
        item.shortcut = shortcut;
    }

    QKeySequence effectiveKey = item.currentKey;
    if (effectiveKey.isEmpty()) {
        const QString persistedText = m_persistedShortcuts.value(normalizedId);
        if (!persistedText.isEmpty()) {
            effectiveKey = fromPortableText(persistedText);
        }
    }
    if (effectiveKey.isEmpty()) {
        effectiveKey = item.defaultKey;
    }

    if (!effectiveKey.isEmpty()) {
        const QString conflictId = checkConflict(effectiveKey, normalizedId);
        if (!conflictId.isEmpty()) {
            return false;
        }
    }

    item.currentKey = effectiveKey;
    m_shortcuts.insert(normalizedId, item);
    applyBindings(m_shortcuts[normalizedId]);

    return true;
}

void ShortcutManager::applyBindings(ShortcutItem& item) const
{
    if (item.action != nullptr) {
        item.action->setShortcut(item.currentKey);
    }
    if (item.shortcut != nullptr) {
        item.shortcut->setKey(item.currentKey);
    }
}

QString ShortcutManager::composePluginShortcutId(const QString& pluginId,
                                                 const QString& actionId) const
{
    const QString action = normalizeId(actionId).toLower().replace(QLatin1Char(' '), QLatin1Char('_'));
    if (action.startsWith(QStringLiteral("plugin."))) {
        return action;
    }

    const QString plugin = normalizePluginName(pluginId);
    if (plugin.isEmpty() || action.isEmpty()) {
        return QString();
    }

    return QStringLiteral("plugin.%1.%2").arg(plugin, action);
}

QString ShortcutManager::normalizeId(const QString& id)
{
    return id.trimmed();
}

QString ShortcutManager::normalizePluginName(const QString& pluginId)
{
    QString normalized = pluginId.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("plugin."))) {
        normalized.remove(0, QStringLiteral("plugin.").size());
    }
    while (normalized.endsWith(QLatin1Char('.'))) {
        normalized.chop(1);
    }
    normalized.replace(QLatin1Char(' '), QLatin1Char('_'));
    return normalized;
}

QString ShortcutManager::toPortableText(const QKeySequence& key)
{
    return key.toString(QKeySequence::PortableText);
}

QKeySequence ShortcutManager::fromPortableText(const QString& text)
{
    return QKeySequence::fromString(text, QKeySequence::PortableText);
}

}  // namespace iqtools::core
