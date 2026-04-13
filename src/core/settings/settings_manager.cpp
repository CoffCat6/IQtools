#include "settings_manager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "core/services/log_service.h"
#include "infra/storage/settings_storage.h"

namespace iqtools::core {

namespace {

// YAML keys
constexpr auto kKeyTheme = "default_theme";
constexpr auto kKeyAutoStart = "auto_start";
constexpr auto kKeyCheckUpdate = "check_update";
constexpr auto kKeyLanguage = "language";
constexpr auto kKeyTranslationSourceLanguage = "translation_source_language";
constexpr auto kKeyTranslationTargetLanguage = "translation_target_language";
constexpr auto kKeyTranslationProviderType = "translation_provider_type";
constexpr auto kKeyTranslationCustomEndpoint = "translation_custom_endpoint";
constexpr auto kKeyTranslationCustomMethod = "translation_custom_method";
constexpr auto kKeyTranslationCustomHeadersJson = "translation_custom_headers_json";
constexpr auto kKeyTranslationCustomQueryTemplate = "translation_custom_query_template";
constexpr auto kKeyTranslationCustomBodyTemplate = "translation_custom_body_template";
constexpr auto kKeyTranslationCustomResultTextPath = "translation_custom_result_text_path";
constexpr auto kKeyTranslationCustomDetectedSourcePath = "translation_custom_detected_source_path";
constexpr auto kKeyTranslationCustomApiKey = "translation_custom_api_key";
constexpr auto kKeyMinToTray = "minimize_to_tray";
constexpr auto kKeyConfirmExit = "confirm_on_exit";
// Logging keys
constexpr auto kKeyLogConsole = "log_console_enabled";
constexpr auto kKeyLogFile = "log_file_enabled";
constexpr auto kKeyLogLevel = "log_minimum_level";
constexpr auto kKeyLogDirectory = "log_directory";
constexpr auto kKeyShortcutPrefix = "shortcut.";
constexpr auto kKeyCaptureOutputDirectory = "capture_output_directory";
constexpr auto kKeyCaptureAutoCopy = "capture_auto_copy";
constexpr auto kKeyCaptureDelaySeconds = "capture_delay_seconds";
constexpr auto kKeyCaptureScalePercent = "capture_scale_percent";
constexpr auto kKeyCaptureOutputFormat = "capture_output_format";
constexpr auto kKeyCaptureJpegQuality = "capture_jpeg_quality";
constexpr auto kKeyCaptureDpi = "capture_dpi";
constexpr auto kKeyCaptureAnnotationEnabled = "capture_annotation_enabled";
constexpr auto kKeyCapturePinAfterCapture = "capture_pin_after_capture";
constexpr auto kKeyCaptureAnnotationLineWidth = "capture_annotation_line_width";
constexpr auto kKeyCaptureAnnotationTextSize = "capture_annotation_text_size";
constexpr auto kKeyCaptureAnnotationMosaicBlockSize = "capture_annotation_mosaic_block_size";
constexpr auto kKeyCaptureAnnotationColorIndex = "capture_annotation_color_index";
constexpr auto kKeyCaptureAnnotationShortcutRectangle = "capture_annotation_shortcut_rectangle";
constexpr auto kKeyCaptureAnnotationShortcutArrow = "capture_annotation_shortcut_arrow";
constexpr auto kKeyCaptureAnnotationShortcutMosaic = "capture_annotation_shortcut_mosaic";
constexpr auto kKeyCaptureAnnotationShortcutText = "capture_annotation_shortcut_text";
constexpr auto kKeyCaptureAnnotationShortcutUndo = "capture_annotation_shortcut_undo";
constexpr auto kKeyCaptureAnnotationShortcutRedo = "capture_annotation_shortcut_redo";
constexpr auto kKeyCaptureAnnotationShortcutColorCycle = "capture_annotation_shortcut_color_cycle";

QString defaultSettingsPath() {
  const QString appDataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (appDataDir.isEmpty()) {
    return QDir::homePath() + QStringLiteral("/.iqtools/setting.yaml");
  }
  return QDir(appDataDir).filePath(QStringLiteral("setting.yaml"));
}

QString normalizeCaptureOutputFormat(const QString& format) {
  const QString normalized = format.trimmed().toLower();
  if (normalized == QStringLiteral("jpg") ||
      normalized == QStringLiteral("jpeg")) {
    return QStringLiteral("jpg");
  }
  return QStringLiteral("png");
}

int normalizeCaptureJpegQuality(int quality) {
  return qBound(80, quality, 100);
}

int normalizeCaptureDpi(int dpi) {
  switch (dpi) {
  case 0:
  case 96:
  case 144:
  case 300:
    return dpi;
  default:
    return 0;
  }
}

QString normalizeTranslationProviderType(const QString& providerType) {
  const QString normalized = providerType.trimmed().toLower();
  if (normalized == QStringLiteral("custom_rest")) {
    return QStringLiteral("custom_rest");
  }
  if (normalized == QStringLiteral("mymemory") ||
      normalized == QStringLiteral("google_web")) {
    return QStringLiteral("google_web");
  }
  return QStringLiteral("google_web");
}

QString normalizeTranslationMethod(const QString& method) {
  const QString normalized = method.trimmed().toUpper();
  if (normalized == QStringLiteral("GET")) {
    return QStringLiteral("GET");
  }
  return QStringLiteral("POST");
}

QString normalizeHeadersJson(const QString& headersJson) {
  const QString normalized = headersJson.trimmed();
  return normalized.isEmpty() ? QStringLiteral("{}") : normalized;
}

TranslationProviderSettings normalizeTranslationProviderSettings(
    const TranslationProviderSettings& settings) {
  const QString defaultQueryTemplate =
      QStringLiteral("q={{text}}&source={{source}}&target={{target}}&apiKey={{apiKey}}");
  const QString defaultBodyTemplate =
      QStringLiteral("{\n  \"text\": \"{{text}}\",\n  \"source\": \"{{source}}\",\n  \"target\": \"{{target}}\",\n  \"apiKey\": \"{{apiKey}}\"\n}");
  TranslationProviderSettings normalized;
  normalized.providerType =
      normalizeTranslationProviderType(settings.providerType);
  normalized.customEndpoint = settings.customEndpoint.trimmed();
  normalized.customMethod =
      normalizeTranslationMethod(settings.customMethod);
  normalized.customHeadersJson =
      normalizeHeadersJson(settings.customHeadersJson);
  normalized.customQueryTemplate = settings.customQueryTemplate.trimmed();
  if (normalized.customQueryTemplate.isEmpty()) {
    normalized.customQueryTemplate = defaultQueryTemplate;
  }
  normalized.customBodyTemplate = settings.customBodyTemplate.trimmed();
  if (normalized.customBodyTemplate.isEmpty()) {
    normalized.customBodyTemplate = defaultBodyTemplate;
  }
  normalized.customResultTextPath =
      settings.customResultTextPath.trimmed();
  if (normalized.customResultTextPath.isEmpty()) {
    normalized.customResultTextPath = QStringLiteral("translatedText");
  }
  normalized.customDetectedSourcePath =
      settings.customDetectedSourcePath.trimmed();
  normalized.customApiKey = settings.customApiKey.trimmed();
  return normalized;
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
  const bool saved = m_storage->save();
  if (!saved) {
    LogService::warning(QStringLiteral("settings"),
                        QStringLiteral("Settings save failed: %1")
                            .arg(m_storage->filePath()));
  }
  return saved;
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

QString SettingsManager::translationSourceLanguage() const {
  return m_translationSourceLanguage;
}

void SettingsManager::setTranslationSourceLanguage(const QString& lang) {
  if (m_translationSourceLanguage == lang) {
    return;
  }

  m_translationSourceLanguage = lang;
  save();
  emit settingsChanged();
}

QString SettingsManager::translationTargetLanguage() const {
  return m_translationTargetLanguage;
}

void SettingsManager::setTranslationTargetLanguage(const QString& lang) {
  if (m_translationTargetLanguage == lang) {
    return;
  }

  m_translationTargetLanguage = lang;
  save();
  emit settingsChanged();
}

TranslationProviderSettings SettingsManager::translationProviderSettings() const {
  return {
      m_translationProviderType,
      m_translationCustomEndpoint,
      m_translationCustomMethod,
      m_translationCustomHeadersJson,
      m_translationCustomQueryTemplate,
      m_translationCustomBodyTemplate,
      m_translationCustomResultTextPath,
      m_translationCustomDetectedSourcePath,
      m_translationCustomApiKey,
  };
}

bool SettingsManager::setTranslationProviderSettings(
    const TranslationProviderSettings& settings) {
  const TranslationProviderSettings normalized =
      normalizeTranslationProviderSettings(settings);
  if (translationProviderSettings() == normalized) {
    return true;
  }

  const TranslationProviderSettings previous = translationProviderSettings();

  m_translationProviderType = normalized.providerType;
  m_translationCustomEndpoint = normalized.customEndpoint;
  m_translationCustomMethod = normalized.customMethod;
  m_translationCustomHeadersJson = normalized.customHeadersJson;
  m_translationCustomQueryTemplate = normalized.customQueryTemplate;
  m_translationCustomBodyTemplate = normalized.customBodyTemplate;
  m_translationCustomResultTextPath = normalized.customResultTextPath;
  m_translationCustomDetectedSourcePath =
      normalized.customDetectedSourcePath;
  m_translationCustomApiKey = normalized.customApiKey;

  const bool saved = save();
  if (saved) {
    emit settingsChanged();
    return true;
  }

  m_translationProviderType = previous.providerType;
  m_translationCustomEndpoint = previous.customEndpoint;
  m_translationCustomMethod = previous.customMethod;
  m_translationCustomHeadersJson = previous.customHeadersJson;
  m_translationCustomQueryTemplate = previous.customQueryTemplate;
  m_translationCustomBodyTemplate = previous.customBodyTemplate;
  m_translationCustomResultTextPath = previous.customResultTextPath;
  m_translationCustomDetectedSourcePath =
      previous.customDetectedSourcePath;
  m_translationCustomApiKey = previous.customApiKey;
  return false;
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

QString SettingsManager::captureOutputDirectory() const {
  return m_captureOutputDirectory;
}

void SettingsManager::setCaptureOutputDirectory(const QString& directory) {
  const QString normalized = directory.trimmed();
  if (m_captureOutputDirectory == normalized) {
    return;
  }

  m_captureOutputDirectory = normalized;
  save();
  emit settingsChanged();
}

bool SettingsManager::captureAutoCopyToClipboard() const {
  return m_captureAutoCopyToClipboard;
}

void SettingsManager::setCaptureAutoCopyToClipboard(bool enabled) {
  if (m_captureAutoCopyToClipboard == enabled) {
    return;
  }

  m_captureAutoCopyToClipboard = enabled;
  save();
  emit settingsChanged();
}

int SettingsManager::captureDelaySeconds() const {
  return m_captureDelaySeconds;
}

void SettingsManager::setCaptureDelaySeconds(int seconds) {
  const int normalized = qBound(0, seconds, 10);
  if (m_captureDelaySeconds == normalized) {
    return;
  }

  m_captureDelaySeconds = normalized;
  save();
  emit settingsChanged();
}

int SettingsManager::captureScalePercent() const {
  return m_captureScalePercent;
}

void SettingsManager::setCaptureScalePercent(int percent) {
  const int normalized = qBound(50, percent, 200);
  if (m_captureScalePercent == normalized) {
    return;
  }

  m_captureScalePercent = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureOutputFormat() const {
  return m_captureOutputFormat;
}

void SettingsManager::setCaptureOutputFormat(const QString& format) {
  const QString normalized = normalizeCaptureOutputFormat(format);
  if (m_captureOutputFormat == normalized) {
    return;
  }

  m_captureOutputFormat = normalized;
  save();
  emit settingsChanged();
}

int SettingsManager::captureJpegQuality() const {
  return m_captureJpegQuality;
}

void SettingsManager::setCaptureJpegQuality(int quality) {
  const int normalized = normalizeCaptureJpegQuality(quality);
  if (m_captureJpegQuality == normalized) {
    return;
  }

  m_captureJpegQuality = normalized;
  save();
  emit settingsChanged();
}

int SettingsManager::captureDpi() const {
  return m_captureDpi;
}

void SettingsManager::setCaptureDpi(int dpi) {
  const int normalized = normalizeCaptureDpi(dpi);
  if (m_captureDpi == normalized) {
    return;
  }

  m_captureDpi = normalized;
  save();
  emit settingsChanged();
}

bool SettingsManager::captureAnnotationEnabled() const {
  return m_captureAnnotationEnabled;
}

void SettingsManager::setCaptureAnnotationEnabled(bool enabled) {
  if (m_captureAnnotationEnabled == enabled) {
    return;
  }

  m_captureAnnotationEnabled = enabled;
  save();
  emit settingsChanged();
}

bool SettingsManager::capturePinAfterCapture() const {
  return m_capturePinAfterCapture;
}

void SettingsManager::setCapturePinAfterCapture(bool enabled) {
  if (m_capturePinAfterCapture == enabled) {
    return;
  }

  m_capturePinAfterCapture = enabled;
  save();
  emit settingsChanged();
}

int SettingsManager::captureAnnotationLineWidth() const {
  return m_captureAnnotationLineWidth;
}

void SettingsManager::setCaptureAnnotationLineWidth(int width) {
  const int normalized = qBound(1, width, 12);
  if (m_captureAnnotationLineWidth == normalized) {
    return;
  }

  m_captureAnnotationLineWidth = normalized;
  save();
  emit settingsChanged();
}

int SettingsManager::captureAnnotationTextSize() const {
  return m_captureAnnotationTextSize;
}

void SettingsManager::setCaptureAnnotationTextSize(int size) {
  const int normalized = qBound(12, size, 96);
  if (m_captureAnnotationTextSize == normalized) {
    return;
  }

  m_captureAnnotationTextSize = normalized;
  save();
  emit settingsChanged();
}

int SettingsManager::captureAnnotationMosaicBlockSize() const {
  return m_captureAnnotationMosaicBlockSize;
}

void SettingsManager::setCaptureAnnotationMosaicBlockSize(int size) {
  const int normalized = qBound(4, size, 64);
  if (m_captureAnnotationMosaicBlockSize == normalized) {
    return;
  }

  m_captureAnnotationMosaicBlockSize = normalized;
  save();
  emit settingsChanged();
}

int SettingsManager::captureAnnotationColorIndex() const {
  return m_captureAnnotationColorIndex;
}

void SettingsManager::setCaptureAnnotationColorIndex(int index) {
  const int normalized = qBound(0, index, 7);
  if (m_captureAnnotationColorIndex == normalized) {
    return;
  }

  m_captureAnnotationColorIndex = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutRectangle() const {
  return m_captureAnnotationShortcutRectangle;
}

void SettingsManager::setCaptureAnnotationShortcutRectangle(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutRectangle == normalized) {
    return;
  }

  m_captureAnnotationShortcutRectangle = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutArrow() const {
  return m_captureAnnotationShortcutArrow;
}

void SettingsManager::setCaptureAnnotationShortcutArrow(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutArrow == normalized) {
    return;
  }

  m_captureAnnotationShortcutArrow = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutMosaic() const {
  return m_captureAnnotationShortcutMosaic;
}

void SettingsManager::setCaptureAnnotationShortcutMosaic(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutMosaic == normalized) {
    return;
  }

  m_captureAnnotationShortcutMosaic = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutText() const {
  return m_captureAnnotationShortcutText;
}

void SettingsManager::setCaptureAnnotationShortcutText(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutText == normalized) {
    return;
  }

  m_captureAnnotationShortcutText = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutUndo() const {
  return m_captureAnnotationShortcutUndo;
}

void SettingsManager::setCaptureAnnotationShortcutUndo(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutUndo == normalized) {
    return;
  }

  m_captureAnnotationShortcutUndo = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutRedo() const {
  return m_captureAnnotationShortcutRedo;
}

void SettingsManager::setCaptureAnnotationShortcutRedo(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutRedo == normalized) {
    return;
  }

  m_captureAnnotationShortcutRedo = normalized;
  save();
  emit settingsChanged();
}

QString SettingsManager::captureAnnotationShortcutColorCycle() const {
  return m_captureAnnotationShortcutColorCycle;
}

void SettingsManager::setCaptureAnnotationShortcutColorCycle(const QString& portableText) {
  const QString normalized = portableText.trimmed();
  if (m_captureAnnotationShortcutColorCycle == normalized) {
    return;
  }

  m_captureAnnotationShortcutColorCycle = normalized;
  save();
  emit settingsChanged();
}

// ─── Private ───

void SettingsManager::loadDefaults() {
  m_defaultTheme = QStringLiteral("dark");
  m_autoStart = false;
  m_checkUpdate = true;
  m_language = QStringLiteral("zh_CN");
  m_translationSourceLanguage = QStringLiteral("auto");
  m_translationTargetLanguage = QStringLiteral("zh-CN");
  m_translationProviderType = QStringLiteral("google_web");
  m_translationCustomEndpoint.clear();
  m_translationCustomMethod = QStringLiteral("POST");
  m_translationCustomHeadersJson = QStringLiteral("{}");
  m_translationCustomQueryTemplate =
      QStringLiteral("q={{text}}&source={{source}}&target={{target}}&apiKey={{apiKey}}");
  m_translationCustomBodyTemplate =
      QStringLiteral("{\n  \"text\": \"{{text}}\",\n  \"source\": \"{{source}}\",\n  \"target\": \"{{target}}\",\n  \"apiKey\": \"{{apiKey}}\"\n}");
  m_translationCustomResultTextPath = QStringLiteral("translatedText");
  m_translationCustomDetectedSourcePath.clear();
  m_translationCustomApiKey.clear();
  m_minimizeToTray = false;
  m_confirmOnExit = true;
  // Logging defaults
  m_logConsoleEnabled = true;
  m_logFileEnabled = true;
  m_logMinimumLevel = QStringLiteral("Debug");
  m_logDirectory = QString();  // Will use default in Logger
  m_shortcuts.clear();
  m_captureOutputDirectory.clear();
  m_captureAutoCopyToClipboard = true;
  m_captureDelaySeconds = 3;
  m_captureScalePercent = 100;
  m_captureOutputFormat = QStringLiteral("png");
  m_captureJpegQuality = 92;
  m_captureDpi = 0;
  m_captureAnnotationEnabled = false;
  m_capturePinAfterCapture = false;
  m_captureAnnotationLineWidth = 3;
  m_captureAnnotationTextSize = 28;
  m_captureAnnotationMosaicBlockSize = 10;
  m_captureAnnotationColorIndex = 0;
  m_captureAnnotationShortcutRectangle = QStringLiteral("1");
  m_captureAnnotationShortcutArrow = QStringLiteral("2");
  m_captureAnnotationShortcutMosaic = QStringLiteral("3");
  m_captureAnnotationShortcutText = QStringLiteral("4");
  m_captureAnnotationShortcutUndo = QStringLiteral("Ctrl+Z");
  m_captureAnnotationShortcutRedo = QStringLiteral("Ctrl+Y");
  m_captureAnnotationShortcutColorCycle = QStringLiteral("C");
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
  m_translationSourceLanguage =
      m_storage->value(QLatin1String(kKeyTranslationSourceLanguage),
                       QStringLiteral("auto"))
          .toString()
          .trimmed();
  m_translationTargetLanguage =
      m_storage->value(QLatin1String(kKeyTranslationTargetLanguage),
                       QStringLiteral("zh-CN"))
          .toString()
          .trimmed();
  m_translationProviderType =
      normalizeTranslationProviderType(
          m_storage->value(QLatin1String(kKeyTranslationProviderType),
                           QStringLiteral("google_web"))
              .toString());
  m_translationCustomEndpoint =
      m_storage->value(QLatin1String(kKeyTranslationCustomEndpoint),
                       QString())
          .toString()
          .trimmed();
  m_translationCustomMethod =
      normalizeTranslationMethod(
          m_storage->value(QLatin1String(kKeyTranslationCustomMethod),
                           QStringLiteral("POST"))
              .toString());
  m_translationCustomHeadersJson =
      normalizeHeadersJson(
          m_storage->value(QLatin1String(kKeyTranslationCustomHeadersJson),
                           QStringLiteral("{}"))
              .toString());
  m_translationCustomQueryTemplate =
      m_storage->value(QLatin1String(kKeyTranslationCustomQueryTemplate),
                       QStringLiteral("q={{text}}&source={{source}}&target={{target}}&apiKey={{apiKey}}"))
          .toString()
          .trimmed();
  m_translationCustomBodyTemplate =
      m_storage->value(QLatin1String(kKeyTranslationCustomBodyTemplate),
                       QStringLiteral("{\n  \"text\": \"{{text}}\",\n  \"source\": \"{{source}}\",\n  \"target\": \"{{target}}\",\n  \"apiKey\": \"{{apiKey}}\"\n}"))
          .toString()
          .trimmed();
  m_translationCustomResultTextPath =
      m_storage->value(QLatin1String(kKeyTranslationCustomResultTextPath),
                       QStringLiteral("translatedText"))
          .toString()
          .trimmed();
  m_translationCustomDetectedSourcePath =
      m_storage->value(QLatin1String(kKeyTranslationCustomDetectedSourcePath),
                       QString())
          .toString()
          .trimmed();
  m_translationCustomApiKey =
      m_storage->value(QLatin1String(kKeyTranslationCustomApiKey), QString())
          .toString()
          .trimmed();
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
  m_captureOutputDirectory =
      m_storage->value(QLatin1String(kKeyCaptureOutputDirectory), QString())
          .toString();
  m_captureAutoCopyToClipboard =
      m_storage->value(QLatin1String(kKeyCaptureAutoCopy), true).toBool();
  m_captureDelaySeconds =
      qBound(0,
             m_storage->value(QLatin1String(kKeyCaptureDelaySeconds), 3).toInt(),
             10);
  m_captureScalePercent =
      qBound(50,
             m_storage->value(QLatin1String(kKeyCaptureScalePercent), 100).toInt(),
             200);
  m_captureOutputFormat =
      normalizeCaptureOutputFormat(
          m_storage->value(QLatin1String(kKeyCaptureOutputFormat),
                           QStringLiteral("png")).toString());
  m_captureJpegQuality =
      normalizeCaptureJpegQuality(
          m_storage->value(QLatin1String(kKeyCaptureJpegQuality), 92).toInt());
  m_captureDpi =
      normalizeCaptureDpi(
          m_storage->value(QLatin1String(kKeyCaptureDpi), 0).toInt());
    m_captureAnnotationEnabled =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationEnabled), false).toBool();
    m_capturePinAfterCapture =
      m_storage->value(QLatin1String(kKeyCapturePinAfterCapture), false).toBool();
    m_captureAnnotationLineWidth =
      qBound(1,
         m_storage->value(QLatin1String(kKeyCaptureAnnotationLineWidth), 3).toInt(),
         12);
    m_captureAnnotationTextSize =
      qBound(12,
         m_storage->value(QLatin1String(kKeyCaptureAnnotationTextSize), 28).toInt(),
         96);
    m_captureAnnotationMosaicBlockSize =
      qBound(4,
         m_storage->value(QLatin1String(kKeyCaptureAnnotationMosaicBlockSize), 10).toInt(),
         64);
    m_captureAnnotationColorIndex =
      qBound(0,
         m_storage->value(QLatin1String(kKeyCaptureAnnotationColorIndex), 0).toInt(),
         7);
    m_captureAnnotationShortcutRectangle =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutRectangle),
               QStringLiteral("1")).toString().trimmed();
    m_captureAnnotationShortcutArrow =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutArrow),
               QStringLiteral("2")).toString().trimmed();
    m_captureAnnotationShortcutMosaic =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutMosaic),
               QStringLiteral("3")).toString().trimmed();
    m_captureAnnotationShortcutText =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutText),
               QStringLiteral("4")).toString().trimmed();
    m_captureAnnotationShortcutUndo =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutUndo),
               QStringLiteral("Ctrl+Z")).toString().trimmed();
    m_captureAnnotationShortcutRedo =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutRedo),
               QStringLiteral("Ctrl+Y")).toString().trimmed();
    m_captureAnnotationShortcutColorCycle =
      m_storage->value(QLatin1String(kKeyCaptureAnnotationShortcutColorCycle),
               QStringLiteral("C")).toString().trimmed();

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
  values.insert(QLatin1String(kKeyTranslationSourceLanguage),
                m_translationSourceLanguage);
  values.insert(QLatin1String(kKeyTranslationTargetLanguage),
                m_translationTargetLanguage);
  values.insert(QLatin1String(kKeyTranslationProviderType),
                m_translationProviderType);
  values.insert(QLatin1String(kKeyTranslationCustomEndpoint),
                m_translationCustomEndpoint);
  values.insert(QLatin1String(kKeyTranslationCustomMethod),
                m_translationCustomMethod);
  values.insert(QLatin1String(kKeyTranslationCustomHeadersJson),
                m_translationCustomHeadersJson);
  values.insert(QLatin1String(kKeyTranslationCustomQueryTemplate),
                m_translationCustomQueryTemplate);
  values.insert(QLatin1String(kKeyTranslationCustomBodyTemplate),
                m_translationCustomBodyTemplate);
  values.insert(QLatin1String(kKeyTranslationCustomResultTextPath),
                m_translationCustomResultTextPath);
  values.insert(QLatin1String(kKeyTranslationCustomDetectedSourcePath),
                m_translationCustomDetectedSourcePath);
  values.insert(QLatin1String(kKeyTranslationCustomApiKey),
                m_translationCustomApiKey);
  values.insert(QLatin1String(kKeyMinToTray), m_minimizeToTray);
  values.insert(QLatin1String(kKeyConfirmExit), m_confirmOnExit);
  values.insert(QLatin1String(kKeyLogConsole), m_logConsoleEnabled);
  values.insert(QLatin1String(kKeyLogFile), m_logFileEnabled);
  values.insert(QLatin1String(kKeyLogLevel), m_logMinimumLevel);
  values.insert(QLatin1String(kKeyLogDirectory), m_logDirectory);
  values.insert(QLatin1String(kKeyCaptureOutputDirectory),
                m_captureOutputDirectory);
  values.insert(QLatin1String(kKeyCaptureAutoCopy),
                m_captureAutoCopyToClipboard);
  values.insert(QLatin1String(kKeyCaptureDelaySeconds), m_captureDelaySeconds);
  values.insert(QLatin1String(kKeyCaptureScalePercent), m_captureScalePercent);
  values.insert(QLatin1String(kKeyCaptureOutputFormat), m_captureOutputFormat);
  values.insert(QLatin1String(kKeyCaptureJpegQuality), m_captureJpegQuality);
  values.insert(QLatin1String(kKeyCaptureDpi), m_captureDpi);
  values.insert(QLatin1String(kKeyCaptureAnnotationEnabled),
                m_captureAnnotationEnabled);
  values.insert(QLatin1String(kKeyCapturePinAfterCapture),
                m_capturePinAfterCapture);
  values.insert(QLatin1String(kKeyCaptureAnnotationLineWidth),
                m_captureAnnotationLineWidth);
  values.insert(QLatin1String(kKeyCaptureAnnotationTextSize),
                m_captureAnnotationTextSize);
  values.insert(QLatin1String(kKeyCaptureAnnotationMosaicBlockSize),
                m_captureAnnotationMosaicBlockSize);
  values.insert(QLatin1String(kKeyCaptureAnnotationColorIndex),
                m_captureAnnotationColorIndex);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutRectangle),
                m_captureAnnotationShortcutRectangle);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutArrow),
                m_captureAnnotationShortcutArrow);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutMosaic),
                m_captureAnnotationShortcutMosaic);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutText),
                m_captureAnnotationShortcutText);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutUndo),
                m_captureAnnotationShortcutUndo);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutRedo),
                m_captureAnnotationShortcutRedo);
  values.insert(QLatin1String(kKeyCaptureAnnotationShortcutColorCycle),
                m_captureAnnotationShortcutColorCycle);

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
