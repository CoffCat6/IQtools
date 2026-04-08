#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

namespace iqtools::core {
class SettingsManager;
}

namespace iqtools::app::bridge {

class ThemeController;

/// QML bridge for the settings page.
/// Exposes SettingsManager properties to QML with Q_PROPERTY bindings.
class SettingsController : public QObject {
    Q_OBJECT

    // ─── Theme ───
    Q_PROPERTY(QString defaultTheme READ defaultTheme WRITE setDefaultTheme NOTIFY settingsChanged)
    Q_PROPERTY(QStringList themeOptions READ themeOptions CONSTANT)

    // ─── Auto-start ───
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY settingsChanged)

    // ─── Check updates ───
    Q_PROPERTY(bool checkUpdate READ checkUpdate WRITE setCheckUpdate NOTIFY settingsChanged)

    // ─── Language ───
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY settingsChanged)
    Q_PROPERTY(QStringList languageOptions READ languageOptions CONSTANT)

    // ─── Minimize to tray ───
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY settingsChanged)

    // ─── Confirm on exit ───
    Q_PROPERTY(bool confirmOnExit READ confirmOnExit WRITE setConfirmOnExit NOTIFY settingsChanged)

    // ─── Status ───
    Q_PROPERTY(QString settingsFilePath READ settingsFilePath CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit SettingsController(iqtools::core::SettingsManager* manager,
                                ThemeController* themeController,
                                QObject* parent = nullptr);

    QString defaultTheme() const;
    QStringList themeOptions() const;
    bool autoStart() const;
    bool checkUpdate() const;
    QString language() const;
    QStringList languageOptions() const;
    bool minimizeToTray() const;
    bool confirmOnExit() const;
    QString settingsFilePath() const;
    QString statusMessage() const;

    void setDefaultTheme(const QString& theme);
    void setAutoStart(bool enabled);
    void setCheckUpdate(bool enabled);
    void setLanguage(const QString& lang);
    void setMinimizeToTray(bool enabled);
    void setConfirmOnExit(bool enabled);

    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void openSettingsFile();

    /// Expose the underlying manager for use by other components (e.g. AppWindow).
    iqtools::core::SettingsManager* manager() const;

signals:
    void settingsChanged();
    void statusMessageChanged();

    /// Emitted when the language setting is changed so the app can reload translations.
    void languageRequested(const QString& languageCode);

    /// Emitted to request minimize-to-tray state change on the window.
    void minimizeToTrayChanged(bool enabled);

    /// Emitted to request confirmOnExit state change on the window.
    void confirmOnExitChanged(bool enabled);

private:
    void setStatusAndClear(const QString& message, int timeoutMs = 3000);
    void refreshFromManager();

private:
    iqtools::core::SettingsManager* m_manager {nullptr};
    ThemeController* m_themeController {nullptr};
    QString m_statusMessage;
    QTimer m_statusClearTimer;
};

}  // namespace iqtools::app::bridge
