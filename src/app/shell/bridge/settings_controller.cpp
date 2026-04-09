#include "settings_controller.h"

#include <algorithm>

#include <QDesktopServices>
#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>

#include "core/services/auto_start_manager.h"
#include "core/services/log_service.h"
#include "core/settings/shortcut_manager.h"
#include "core/settings/settings_manager.h"
#include "theme_controller.h"

namespace iqtools::app::bridge {

namespace {

QKeySequence parseShortcutSequence(const QString& sequenceText)
{
    const QString trimmed = sequenceText.trimmed();
    if (trimmed.isEmpty()) {
        return QKeySequence();
    }

    QKeySequence key = QKeySequence::fromString(trimmed, QKeySequence::PortableText);
    if (key.isEmpty()) {
        key = QKeySequence::fromString(trimmed, QKeySequence::NativeText);
    }
    return key;
}

QString toDisplayShortcut(const QKeySequence& key)
{
    return key.toString(QKeySequence::NativeText);
}

QString translateShortcutText(const QString& source)
{
    const QByteArray utf8 = source.toUtf8();
    return QCoreApplication::translate("ShortcutManager", utf8.constData());
}

QVariantMap toShortcutVariantMap(const iqtools::core::ShortcutItem& item)
{
    const QString localizedCategory = translateShortcutText(item.category);
    const QString localizedDescription = translateShortcutText(item.description);

    return {
        {QStringLiteral("id"), item.id},
        {QStringLiteral("category"), localizedCategory},
        {QStringLiteral("description"), localizedDescription},
        {QStringLiteral("defaultKey"), toDisplayShortcut(item.defaultKey)},
        {QStringLiteral("currentKey"), toDisplayShortcut(item.currentKey)},
        {QStringLiteral("customized"), item.currentKey != item.defaultKey},
        {QStringLiteral("hasBinding"), !item.currentKey.isEmpty()},
    };
}

}  // namespace

SettingsController::SettingsController(iqtools::core::SettingsManager* manager,
                                       iqtools::core::ShortcutManager* shortcutManager,
                                       ThemeController* themeController,
                                       QObject* parent)
    : QObject(parent)
    , m_manager(manager)
    , m_shortcutManager(shortcutManager)
    , m_themeController(themeController)
{
    // When SettingsManager signals a change, refresh our QML properties.
    connect(m_manager, &iqtools::core::SettingsManager::settingsChanged,
            this, &SettingsController::settingsChanged);

    // Auto-clear status timer (single-shot)
    m_statusClearTimer.setSingleShot(true);
    connect(&m_statusClearTimer, &QTimer::timeout, this, [this]() {
        m_statusMessage.clear();
        emit statusMessageChanged();
    });

    // Apply saved theme on construction
    if (m_themeController != nullptr) {
        m_themeController->setTheme(m_manager->defaultTheme());
    }

    // Sync OS auto-start state with persisted setting
    iqtools::core::AutoStartManager::setEnabled(m_manager->autoStartEnabled());

    if (m_shortcutManager != nullptr) {
        connect(m_shortcutManager, &iqtools::core::ShortcutManager::shortcutChanged,
                this, &SettingsController::shortcutsChanged);
    }
}

// ─── Reads ───

QString SettingsController::defaultTheme() const
{
    return m_manager->defaultTheme();
}

QStringList SettingsController::themeOptions() const
{
    return {
        QStringLiteral("light"),
        QStringLiteral("dark"),
    };
}

bool SettingsController::autoStart() const
{
    return m_manager->autoStartEnabled();
}

bool SettingsController::checkUpdate() const
{
    return m_manager->checkUpdateEnabled();
}

QString SettingsController::language() const
{
    return m_manager->language();
}

QStringList SettingsController::languageOptions() const
{
    return {
        QStringLiteral("zh_CN"),
        QStringLiteral("en_US"),
    };
}

bool SettingsController::minimizeToTray() const
{
    return m_manager->minimizeToTray();
}

bool SettingsController::confirmOnExit() const
{
    return m_manager->confirmOnExit();
}

QString SettingsController::settingsFilePath() const
{
    return m_manager->settingsFilePath();
}

QString SettingsController::statusMessage() const
{
    return m_statusMessage;
}

QVariantList SettingsController::shortcuts() const
{
    if (m_shortcutManager == nullptr) {
        return {};
    }

    QVector<iqtools::core::ShortcutItem> shortcutItems = m_shortcutManager->allShortcuts();
    std::sort(shortcutItems.begin(), shortcutItems.end(),
              [](const iqtools::core::ShortcutItem& left,
                 const iqtools::core::ShortcutItem& right) {
                  if (left.category != right.category) {
                      return left.category < right.category;
                  }
                  if (left.description != right.description) {
                      return left.description < right.description;
                  }
                  return left.id < right.id;
              });

    QVariantList list;
    list.reserve(shortcutItems.size());
    for (const iqtools::core::ShortcutItem& item : shortcutItems) {
        list.push_back(toShortcutVariantMap(item));
    }
    return list;
}

iqtools::core::SettingsManager* SettingsController::manager() const
{
    return m_manager;
}

// ─── Writes ───

void SettingsController::setDefaultTheme(const QString& theme)
{
    m_manager->setDefaultTheme(theme);

    // Live-apply the theme immediately
    if (m_themeController != nullptr) {
        m_themeController->setTheme(theme);
    }

    setStatusAndClear(QStringLiteral("主题已切换为 ") + theme);

    iqtools::core::LogService::info(QStringLiteral("settings"),
                                    QStringLiteral("Default theme changed to: %1").arg(theme));
}

void SettingsController::setAutoStart(bool enabled)
{
    m_manager->setAutoStartEnabled(enabled);

    // Actually register/unregister with the OS
    const bool ok = iqtools::core::AutoStartManager::setEnabled(enabled);
    if (ok) {
        setStatusAndClear(enabled ? QStringLiteral("已启用开机自启") : QStringLiteral("已关闭开机自启"));
    } else {
        setStatusAndClear(QStringLiteral("开机自启设置失败，请检查权限"), 5000);
    }

    iqtools::core::LogService::info(QStringLiteral("settings"),
                                    QStringLiteral("Auto-start: %1 (registry: %2)")
                                        .arg(enabled ? "enabled" : "disabled")
                                        .arg(ok ? "success" : "failed"));
}

void SettingsController::setCheckUpdate(bool enabled)
{
    m_manager->setCheckUpdateEnabled(enabled);

    setStatusAndClear(enabled ? QStringLiteral("已启用自动检查更新") : QStringLiteral("已关闭自动检查更新"));

    iqtools::core::LogService::info(QStringLiteral("settings"),
                                    QStringLiteral("Check-update: %1").arg(enabled ? "enabled" : "disabled"));
}

void SettingsController::setLanguage(const QString& lang)
{
    m_manager->setLanguage(lang);

    setStatusAndClear(QStringLiteral("语言已切换为 ") + lang);

    // Notify the application to reload translations
    emit languageRequested(lang);
    emit shortcutsChanged();

    iqtools::core::LogService::info(QStringLiteral("settings"),
                                    QStringLiteral("Language changed to: %1").arg(lang));
}

void SettingsController::setMinimizeToTray(bool enabled)
{
    m_manager->setMinimizeToTray(enabled);

    setStatusAndClear(enabled ? QStringLiteral("已启用最小化到托盘") : QStringLiteral("已关闭最小化到托盘"));

    emit minimizeToTrayChanged(enabled);
}

void SettingsController::setConfirmOnExit(bool enabled)
{
    m_manager->setConfirmOnExit(enabled);

    setStatusAndClear(enabled ? QStringLiteral("已启用退出确认") : QStringLiteral("已关闭退出确认"));

    emit confirmOnExitChanged(enabled);
}

// ─── Actions ───

void SettingsController::resetToDefaults()
{
    m_manager->setDefaultTheme(QStringLiteral("dark"));
    m_manager->setAutoStartEnabled(false);
    m_manager->setCheckUpdateEnabled(true);
    m_manager->setLanguage(QStringLiteral("zh_CN"));
    m_manager->setMinimizeToTray(false);
    m_manager->setConfirmOnExit(true);

    // Sync OS auto-start
    iqtools::core::AutoStartManager::setEnabled(false);

    if (m_themeController != nullptr) {
        m_themeController->setTheme(QStringLiteral("dark"));
    }

    if (m_shortcutManager != nullptr) {
        m_shortcutManager->resetAllToDefault();
        emit shortcutsChanged();
    }

    setStatusAndClear(QStringLiteral("所有设置已恢复默认值"));
    emit settingsChanged();

    iqtools::core::LogService::info(QStringLiteral("settings"), QStringLiteral("Settings reset to defaults"));
}

void SettingsController::openSettingsFile()
{
    const QUrl url = QUrl::fromLocalFile(m_manager->settingsFilePath());
    QDesktopServices::openUrl(url);
}

bool SettingsController::applyShortcut(const QString& id, const QString& sequenceText)
{
    if (m_shortcutManager == nullptr || id.trimmed().isEmpty()) {
        return false;
    }

    const QString trimmed = sequenceText.trimmed();
    const QKeySequence key = parseShortcutSequence(trimmed);
    if (!trimmed.isEmpty() && key.isEmpty()) {
        setStatusAndClear(tr("快捷键格式无效，请使用例如 Ctrl+Shift+K"), 5000);
        return false;
    }

    if (!key.isEmpty()) {
        const QString conflict = m_shortcutManager->checkConflict(key, id);
        if (!conflict.isEmpty()) {
            setStatusAndClear(tr("快捷键冲突：已被 %1 占用").arg(conflict), 5000);
            return false;
        }
    }

    const bool ok = m_shortcutManager->updateShortcut(id, key);
    if (!ok) {
        setStatusAndClear(tr("快捷键更新失败，请重试"), 5000);
        return false;
    }

    emit shortcutsChanged();
    setStatusAndClear(trimmed.isEmpty() ? tr("已清空快捷键绑定")
                                        : tr("快捷键已更新"));
    return true;
}

QString SettingsController::checkShortcutConflictText(const QString& sequenceText,
                                                      const QString& ignoreId) const
{
    if (m_shortcutManager == nullptr) {
        return QString();
    }

    const QKeySequence key = parseShortcutSequence(sequenceText);
    if (key.isEmpty()) {
        return QString();
    }

    return m_shortcutManager->checkConflict(key, ignoreId);
}

void SettingsController::resetShortcutToDefault(const QString& id)
{
    if (m_shortcutManager == nullptr || id.trimmed().isEmpty()) {
        return;
    }

    m_shortcutManager->resetToDefault(id);
    emit shortcutsChanged();
    setStatusAndClear(tr("已恢复默认快捷键"));
}

void SettingsController::resetAllShortcutsToDefault()
{
    if (m_shortcutManager == nullptr) {
        return;
    }

    m_shortcutManager->resetAllToDefault();
    emit shortcutsChanged();
    setStatusAndClear(tr("已恢复全部默认快捷键"));
}

void SettingsController::setStatusAndClear(const QString& message, int timeoutMs)
{
    m_statusMessage = message;
    emit statusMessageChanged();

    m_statusClearTimer.start(timeoutMs);
}

void SettingsController::refreshFromManager()
{
    emit settingsChanged();
}

}  // namespace iqtools::app::bridge
