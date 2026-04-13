#include "translation_settings_controller.h"

#include "core/services/log_service.h"
#include "core/services/translation_provider.h"
#include "core/settings/settings_manager.h"

namespace iqtools::app::bridge {

TranslationSettingsController::TranslationSettingsController(
    iqtools::core::SettingsManager* settingsManager, QObject* parent)
    : QObject(parent)
    , m_settingsManager(settingsManager)
{
    refreshFromManager();

    if (m_settingsManager != nullptr) {
        connect(m_settingsManager, &iqtools::core::SettingsManager::settingsChanged,
                this, &TranslationSettingsController::refreshFromManager);
    }
}

QString TranslationSettingsController::providerType() const
{
    return m_pendingSettings.providerType;
}

QString TranslationSettingsController::customEndpoint() const
{
    return m_pendingSettings.customEndpoint;
}

QString TranslationSettingsController::customMethod() const
{
    return m_pendingSettings.customMethod;
}

QString TranslationSettingsController::customHeadersJson() const
{
    return m_pendingSettings.customHeadersJson;
}

QString TranslationSettingsController::customQueryTemplate() const
{
    return m_pendingSettings.customQueryTemplate;
}

QString TranslationSettingsController::customBodyTemplate() const
{
    return m_pendingSettings.customBodyTemplate;
}

QString TranslationSettingsController::customResultTextPath() const
{
    return m_pendingSettings.customResultTextPath;
}

QString TranslationSettingsController::customDetectedSourcePath() const
{
    return m_pendingSettings.customDetectedSourcePath;
}

QString TranslationSettingsController::customApiKey() const
{
    return m_pendingSettings.customApiKey;
}

bool TranslationSettingsController::dirty() const
{
    return m_pendingSettings != m_appliedSettings;
}

QString TranslationSettingsController::statusMessage() const
{
    return m_statusMessage;
}

void TranslationSettingsController::setProviderType(const QString& providerType)
{
    const QString normalized =
        iqtools::core::normalizeTranslationProviderType(providerType);
    if (m_pendingSettings.providerType == normalized) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.providerType = normalized;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomEndpoint(const QString& endpoint)
{
    if (m_pendingSettings.customEndpoint == endpoint) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customEndpoint = endpoint;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomMethod(const QString& method)
{
    const QString normalized = method.trimmed().toUpper();
    if (m_pendingSettings.customMethod == normalized) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customMethod = normalized;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomHeadersJson(
    const QString& headersJson)
{
    if (m_pendingSettings.customHeadersJson == headersJson) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customHeadersJson = headersJson;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomQueryTemplate(
    const QString& queryTemplate)
{
    if (m_pendingSettings.customQueryTemplate == queryTemplate) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customQueryTemplate = queryTemplate;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomBodyTemplate(
    const QString& bodyTemplate)
{
    if (m_pendingSettings.customBodyTemplate == bodyTemplate) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customBodyTemplate = bodyTemplate;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomResultTextPath(const QString& path)
{
    if (m_pendingSettings.customResultTextPath == path) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customResultTextPath = path;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomDetectedSourcePath(
    const QString& path)
{
    if (m_pendingSettings.customDetectedSourcePath == path) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customDetectedSourcePath = path;
    emit settingsChanged();
}

void TranslationSettingsController::setCustomApiKey(const QString& apiKey)
{
    if (m_pendingSettings.customApiKey == apiKey) {
        return;
    }

    clearStatusMessage();
    m_pendingSettings.customApiKey = apiKey;
    emit settingsChanged();
}

bool TranslationSettingsController::apply()
{
    if (m_settingsManager == nullptr) {
        m_statusMessage = QStringLiteral("翻译接口设置保存失败：设置管理器不可用");
        emit statusMessageChanged();
        return false;
    }

    const QString validationMessage =
        iqtools::core::validateTranslationProviderSettings(m_pendingSettings);
    if (!validationMessage.isEmpty()) {
        m_statusMessage = validationMessage;
        emit statusMessageChanged();
        return false;
    }

    if (!m_settingsManager->setTranslationProviderSettings(m_pendingSettings)) {
        m_statusMessage = QStringLiteral("翻译接口设置保存失败，请重试");
        emit statusMessageChanged();
        return false;
    }

    m_appliedSettings = m_settingsManager->translationProviderSettings();
    m_pendingSettings = m_appliedSettings;
    m_statusMessage =
        QStringLiteral("翻译接口设置已保存，将在下一次翻译时生效");

    emit settingsChanged();
    emit statusMessageChanged();

    iqtools::core::LogService::info(
        QStringLiteral("settings.translation"),
        QStringLiteral("Translation provider settings updated: %1")
            .arg(m_appliedSettings.providerType));
    return true;
}

void TranslationSettingsController::resetPending()
{
    m_pendingSettings = m_appliedSettings;
    m_statusMessage.clear();
    emit settingsChanged();
    emit statusMessageChanged();
}

void TranslationSettingsController::clearStatusMessage()
{
    if (m_statusMessage.isEmpty()) {
        return;
    }

    m_statusMessage.clear();
    emit statusMessageChanged();
}

void TranslationSettingsController::setPendingSettings(
    const iqtools::core::TranslationProviderSettings& settings)
{
    m_pendingSettings = settings;
    emit settingsChanged();
}

void TranslationSettingsController::refreshFromManager()
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const iqtools::core::TranslationProviderSettings latestSettings =
        m_settingsManager->translationProviderSettings();
    if (latestSettings == m_appliedSettings) {
        return;
    }

    m_appliedSettings = latestSettings;
    m_pendingSettings = latestSettings;
    m_statusMessage.clear();
    emit settingsChanged();
    emit statusMessageChanged();
}

}  // namespace iqtools::app::bridge
