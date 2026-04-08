#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include <memory>

namespace iqtools::infra::storage {
class SettingsStorage;
}

namespace iqtools::core {

/// Application settings manager.
/// Manages all persistent settings: theme, auto-start, update checking,
/// language, etc. Loads from and saves to a YAML file (setting.yaml in app data
/// directory).
class SettingsManager : public QObject {
  Q_OBJECT

public:
  explicit SettingsManager(QObject *parent = nullptr);
  ~SettingsManager() override;

  /// Load settings from disk. Creates default file if none exists.
  void load();

  /// Save current settings to disk.
  bool save();

  /// Get the settings file path.
  QString settingsFilePath() const;

  // ─── Theme ───
  QString defaultTheme() const;
  void setDefaultTheme(const QString &theme);

  // ─── Auto-start ───
  bool autoStartEnabled() const;
  void setAutoStartEnabled(bool enabled);

  // ─── Check updates ───
  bool checkUpdateEnabled() const;
  void setCheckUpdateEnabled(bool enabled);

  // ─── Language ───
  QString language() const;
  void setLanguage(const QString &lang);

  // ─── Minimize to tray ───
  bool minimizeToTray() const;
  void setMinimizeToTray(bool enabled);

  // ─── Confirm on exit ───
  bool confirmOnExit() const;
  void setConfirmOnExit(bool enabled);

  // ─── Logging settings ───
  bool logConsoleEnabled() const;
  void setLogConsoleEnabled(bool enabled);

  bool logFileEnabled() const;
  void setLogFileEnabled(bool enabled);

  QString logMinimumLevel() const;
  void setLogMinimumLevel(const QString &level);

  QString logDirectory() const;
  void setLogDirectory(const QString &directory);

signals:
  void settingsChanged();
  void defaultThemeChanged(const QString &theme);
  void loggingSettingsChanged();

private:
  void loadDefaults();
  void syncFromStorage();
  void syncToStorage();

private:
  std::unique_ptr<iqtools::infra::storage::SettingsStorage> m_storage;

  QString m_defaultTheme;
  bool m_autoStart{false};
  bool m_checkUpdate{true};
  QString m_language;
  bool m_minimizeToTray{false};
  bool m_confirmOnExit{true};
  
  // Logging settings
  bool m_logConsoleEnabled{true};
  bool m_logFileEnabled{true};
  QString m_logMinimumLevel;
  QString m_logDirectory;
};

} // namespace iqtools::core
