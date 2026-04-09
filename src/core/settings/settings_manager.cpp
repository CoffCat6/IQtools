#include "settings_manager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "infra/storage/settings_storage.h"

namespace iqtools::core {

namespace {

// YAML keys
constexpr auto kKeyTheme = "default_theme";
constexpr auto kKeyAutoStart = "auto_start";
constexpr auto kKeyCheckUpdate = "check_update";
constexpr auto kKeyLanguage = "language";
constexpr auto kKeyMinToTray = "minimize_to_tray";
constexpr auto kKeyConfirmExit = "confirm_on_exit";
// Logging keys
constexpr auto kKeyLogConsole = "log_console_enabled";
constexpr auto kKeyLogFile = "log_file_enabled";
constexpr auto kKeyLogLevel = "log_minimum_level";
constexpr auto kKeyLogDirectory = "log_directory";
constexpr auto kKeyShortcutPrefix = "shortcut.";

QString defaultSettingsPath() {
  const QString appDataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (appDataDir.isEmpty()) {
    return QDir::homePath() + QStringLiteral("/.iqtools/setting.yaml");
  }
  return QDir(appDataDir).filePath(QStringLiteral("setting.yaml"));
}

} // namespace

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent),
      m_storage(std::make_unique<iqtools::infra::storage::SettingsStorage>(
          defaultSettingsPath())) {
  loadDefaults();
}

SettingsManager::~SettingsManager() = default;

void SettingsManager::load() {
  if (m_storage->exists() && m_storage->load()) {
    syncFromStorage();
  } else {
    // First launch — write defaults
    loadDefaults();
    save();
  }
}

bool SettingsManager::save() {
  syncToStorage();
  return m_storage->save();
}

QString SettingsManager::settingsFilePath() const {
  return m_storage->filePath();
}

// ─── Theme ───

QString SettingsManager::defaultTheme() const { return m_defaultTheme; }

void SettingsManager::setDefaultTheme(const QString &theme) {
  const QString normalized = theme.trimmed().toLower();
  if (m_defaultTheme == normalized) {
    return;
  }
  m_defaultTheme = normalized;
  save();
  emit defaultThemeChanged(m_defaultTheme);
  emit settingsChanged();
}

// ─── Auto-start ───

bool SettingsManager::autoStartEnabled() const { return m_autoStart; }

void SettingsManager::setAutoStartEnabled(bool enabled) {
  if (m_autoStart == enabled) {
    return;
  }
  m_autoStart = enabled;
  save();
  emit settingsChanged();
}

// ─── Check updates ───

bool SettingsManager::checkUpdateEnabled() const { return m_checkUpdate; }

void SettingsManager::setCheckUpdateEnabled(bool enabled) {
  if (m_checkUpdate == enabled) {
    return;
  }
  m_checkUpdate = enabled;
  save();
  emit settingsChanged();
}

// ─── Language ───

QString SettingsManager::language() const { return m_language; }

void SettingsManager::setLanguage(const QString &lang) {
  if (m_language == lang) {
    return;
  }
  m_language = lang;
  save();
  emit settingsChanged();
}

// ─── Minimize to tray ───

bool SettingsManager::minimizeToTray() const { return m_minimizeToTray; }

void SettingsManager::setMinimizeToTray(bool enabled) {
  if (m_minimizeToTray == enabled) {
    return;
  }
  m_minimizeToTray = enabled;
  save();
  emit settingsChanged();
}

// ─── Confirm on exit ───

bool SettingsManager::confirmOnExit() const { return m_confirmOnExit; }

void SettingsManager::setConfirmOnExit(bool enabled) {
  if (m_confirmOnExit == enabled) {
    return;
  }
  m_confirmOnExit = enabled;
  save();
  emit settingsChanged();
}

// ─── Logging settings ───

bool SettingsManager::logConsoleEnabled() const { return m_logConsoleEnabled; }

void SettingsManager::setLogConsoleEnabled(bool enabled) {
  if (m_logConsoleEnabled == enabled) {
    return;
  }
  m_logConsoleEnabled = enabled;
  save();
  emit loggingSettingsChanged();
}

bool SettingsManager::logFileEnabled() const { return m_logFileEnabled; }

void SettingsManager::setLogFileEnabled(bool enabled) {
  if (m_logFileEnabled == enabled) {
    return;
  }
  m_logFileEnabled = enabled;
  save();
  emit loggingSettingsChanged();
}

QString SettingsManager::logMinimumLevel() const { return m_logMinimumLevel; }

void SettingsManager::setLogMinimumLevel(const QString &level) {
  if (m_logMinimumLevel == level) {
    return;
  }
  m_logMinimumLevel = level;
  save();
  emit loggingSettingsChanged();
}

QString SettingsManager::logDirectory() const { return m_logDirectory; }

void SettingsManager::setLogDirectory(const QString &directory) {
  if (m_logDirectory == directory) {
    return;
  }
  m_logDirectory = directory;
  save();
  emit loggingSettingsChanged();
}

QMap<QString, QString> SettingsManager::shortcutMappings() const {
  return m_shortcuts;
}

bool SettingsManager::setShortcutMappings(
    const QMap<QString, QString>& shortcuts) {
  if (m_shortcuts == shortcuts) {
    return true;
  }

  const QMap<QString, QString> previousShortcuts = m_shortcuts;
  m_shortcuts = shortcuts;
  const bool saved = save();
  if (saved) {
    emit settingsChanged();
  } else {
    m_shortcuts = previousShortcuts;
  }
  return saved;
}

// ─── Private ───

void SettingsManager::loadDefaults() {
  m_defaultTheme = QStringLiteral("dark");
  m_autoStart = false;
  m_checkUpdate = true;
  m_language = QStringLiteral("zh_CN");
  m_minimizeToTray = false;
  m_confirmOnExit = true;
  // Logging defaults
  m_logConsoleEnabled = true;
  m_logFileEnabled = true;
  m_logMinimumLevel = QStringLiteral("Debug");
  m_logDirectory = QString();  // Will use default in Logger
  m_shortcuts.clear();
}

void SettingsManager::syncFromStorage() {
  m_defaultTheme =
      m_storage->value(QLatin1String(kKeyTheme), QStringLiteral("dark"))
          .toString()
          .trimmed()
          .toLower();
  m_autoStart = m_storage->value(QLatin1String(kKeyAutoStart), false).toBool();
  m_checkUpdate =
      m_storage->value(QLatin1String(kKeyCheckUpdate), true).toBool();
  m_language =
      m_storage->value(QLatin1String(kKeyLanguage), QStringLiteral("zh_CN"))
          .toString();
  m_minimizeToTray =
      m_storage->value(QLatin1String(kKeyMinToTray), false).toBool();
  m_confirmOnExit =
      m_storage->value(QLatin1String(kKeyConfirmExit), true).toBool();
  // Logging
  m_logConsoleEnabled =
      m_storage->value(QLatin1String(kKeyLogConsole), true).toBool();
  m_logFileEnabled =
      m_storage->value(QLatin1String(kKeyLogFile), true).toBool();
  m_logMinimumLevel =
      m_storage->value(QLatin1String(kKeyLogLevel), QStringLiteral("Debug"))
          .toString();
  m_logDirectory =
      m_storage->value(QLatin1String(kKeyLogDirectory), QString()).toString();

  m_shortcuts.clear();
  const QString shortcutPrefix = QString::fromLatin1(kKeyShortcutPrefix);
  const QMap<QString, QVariant> allValues = m_storage->allValues();
  for (auto it = allValues.constBegin(); it != allValues.constEnd(); ++it) {
    if (!it.key().startsWith(shortcutPrefix)) {
      continue;
    }

    const QString id = it.key().mid(shortcutPrefix.size());
    if (id.isEmpty()) {
      continue;
    }

    const QString portableText = it.value().toString().trimmed();
    if (!portableText.isEmpty()) {
      m_shortcuts.insert(id, portableText);
    }
  }
}

void SettingsManager::syncToStorage() {
  QMap<QString, QVariant> values = m_storage->allValues();

  values.insert(QLatin1String(kKeyTheme), m_defaultTheme);
  values.insert(QLatin1String(kKeyAutoStart), m_autoStart);
  values.insert(QLatin1String(kKeyCheckUpdate), m_checkUpdate);
  values.insert(QLatin1String(kKeyLanguage), m_language);
  values.insert(QLatin1String(kKeyMinToTray), m_minimizeToTray);
  values.insert(QLatin1String(kKeyConfirmExit), m_confirmOnExit);
  values.insert(QLatin1String(kKeyLogConsole), m_logConsoleEnabled);
  values.insert(QLatin1String(kKeyLogFile), m_logFileEnabled);
  values.insert(QLatin1String(kKeyLogLevel), m_logMinimumLevel);
  values.insert(QLatin1String(kKeyLogDirectory), m_logDirectory);

  const QString shortcutPrefix = QString::fromLatin1(kKeyShortcutPrefix);
  for (auto it = values.begin(); it != values.end();) {
    if (it.key().startsWith(shortcutPrefix)) {
      it = values.erase(it);
      continue;
    }
    ++it;
  }

  for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
    if (it.key().trimmed().isEmpty() || it.value().trimmed().isEmpty()) {
      continue;
    }
    values.insert(shortcutPrefix + it.key(), it.value());
  }

  m_storage->setAllValues(values);
}

} // namespace iqtools::core
