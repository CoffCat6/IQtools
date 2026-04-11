#pragma once

#include <QtCore/QObject>
#include <QtCore/QMap>
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

  // ─── Translation settings ───
  QString translationSourceLanguage() const;
  void setTranslationSourceLanguage(const QString& lang);

  QString translationTargetLanguage() const;
  void setTranslationTargetLanguage(const QString& lang);

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

  // ─── Shortcut settings ───
  QMap<QString, QString> shortcutMappings() const;
  bool setShortcutMappings(const QMap<QString, QString>& shortcuts);

  // ─── Capture settings ───
  QString captureOutputDirectory() const;
  void setCaptureOutputDirectory(const QString& directory);

  bool captureAutoCopyToClipboard() const;
  void setCaptureAutoCopyToClipboard(bool enabled);

  int captureDelaySeconds() const;
  void setCaptureDelaySeconds(int seconds);

  int captureScalePercent() const;
  void setCaptureScalePercent(int percent);

  QString captureOutputFormat() const;
  void setCaptureOutputFormat(const QString& format);

  int captureJpegQuality() const;
  void setCaptureJpegQuality(int quality);

  int captureDpi() const;
  void setCaptureDpi(int dpi);

  bool captureAnnotationEnabled() const;
  void setCaptureAnnotationEnabled(bool enabled);

  bool capturePinAfterCapture() const;
  void setCapturePinAfterCapture(bool enabled);

  int captureAnnotationLineWidth() const;
  void setCaptureAnnotationLineWidth(int width);

  int captureAnnotationTextSize() const;
  void setCaptureAnnotationTextSize(int size);

  int captureAnnotationMosaicBlockSize() const;
  void setCaptureAnnotationMosaicBlockSize(int size);

  int captureAnnotationColorIndex() const;
  void setCaptureAnnotationColorIndex(int index);

  QString captureAnnotationShortcutRectangle() const;
  void setCaptureAnnotationShortcutRectangle(const QString& portableText);

  QString captureAnnotationShortcutArrow() const;
  void setCaptureAnnotationShortcutArrow(const QString& portableText);

  QString captureAnnotationShortcutMosaic() const;
  void setCaptureAnnotationShortcutMosaic(const QString& portableText);

  QString captureAnnotationShortcutText() const;
  void setCaptureAnnotationShortcutText(const QString& portableText);

  QString captureAnnotationShortcutUndo() const;
  void setCaptureAnnotationShortcutUndo(const QString& portableText);

  QString captureAnnotationShortcutRedo() const;
  void setCaptureAnnotationShortcutRedo(const QString& portableText);

  QString captureAnnotationShortcutColorCycle() const;
  void setCaptureAnnotationShortcutColorCycle(const QString& portableText);

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
  QString m_translationSourceLanguage;
  QString m_translationTargetLanguage;
  bool m_minimizeToTray{false};
  bool m_confirmOnExit{true};
  
  // Logging settings
  bool m_logConsoleEnabled{true};
  bool m_logFileEnabled{true};
  QString m_logMinimumLevel;
  QString m_logDirectory;
  QMap<QString, QString> m_shortcuts;

  QString m_captureOutputDirectory;
  bool m_captureAutoCopyToClipboard{true};
  int m_captureDelaySeconds{3};
  int m_captureScalePercent{100};
  QString m_captureOutputFormat;
  int m_captureJpegQuality{92};
  int m_captureDpi{0};
  bool m_captureAnnotationEnabled{false};
  bool m_capturePinAfterCapture{false};
  int m_captureAnnotationLineWidth{3};
  int m_captureAnnotationTextSize{28};
  int m_captureAnnotationMosaicBlockSize{10};
  int m_captureAnnotationColorIndex{0};
  QString m_captureAnnotationShortcutRectangle;
  QString m_captureAnnotationShortcutArrow;
  QString m_captureAnnotationShortcutMosaic;
  QString m_captureAnnotationShortcutText;
  QString m_captureAnnotationShortcutUndo;
  QString m_captureAnnotationShortcutRedo;
  QString m_captureAnnotationShortcutColorCycle;
};

} // namespace iqtools::core
