#include "settings_controller.h"

#include <QDesktopServices>
#include <QtCore/QUrl>

#include "core/services/auto_start_manager.h"
#include "core/services/log_service.h"
#include "core/settings/settings_manager.h"
#include "theme_controller.h"

namespace iqtools::app::bridge {

SettingsController::SettingsController(iqtools::core::SettingsManager* manager,
                                       ThemeController* themeController,
                                       QObject* parent)
    : QObject(parent)
    , m_manager(manager)
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

    setStatusAndClear(QStringLiteral("所有设置已恢复默认值"));
    emit settingsChanged();

    iqtools::core::LogService::info(QStringLiteral("settings"), QStringLiteral("Settings reset to defaults"));
}

void SettingsController::openSettingsFile()
{
    const QUrl url = QUrl::fromLocalFile(m_manager->settingsFilePath());
    QDesktopServices::openUrl(url);
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
