#include "translation_service.h"

#include <QtNetwork/QNetworkRequest>

#include "core/services/translation_provider.h"
#include "infra/logging/logger.h"

namespace iqtools::core {

namespace {

constexpr int kTranslateTimeoutMs = 15000;

QString friendlyNetworkError(QNetworkReply* reply)
{
    const int httpStatus =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (httpStatus == 429) {
        return QStringLiteral("翻译服务请求过于频繁，请稍后再试");
    }
    if (httpStatus >= 500) {
        return QStringLiteral("翻译服务暂时不可用，请稍后再试");
    }
    if (httpStatus >= 400) {
        return QStringLiteral("翻译服务请求失败（HTTP %1）").arg(httpStatus);
    }

    switch (reply->error()) {
    case QNetworkReply::TimeoutError:
        return QStringLiteral("翻译请求超时，请检查网络后重试");
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
        return QStringLiteral("无法连接翻译服务，请检查网络连接");
    case QNetworkReply::SslHandshakeFailedError:
        return QStringLiteral("翻译服务证书校验失败");
    default:
        break;
    }

    const QString fallback = reply->errorString().trimmed();
    if (!fallback.isEmpty()) {
        return QStringLiteral("翻译失败：%1").arg(fallback);
    }
    return QStringLiteral("翻译失败，请稍后重试");
}

}  // namespace

QList<LanguageInfo> TranslationService::supportedLanguages()
{
    return {
        {"auto", "Auto Detect", "自动检测"},
        {"zh-CN", "Chinese (Simplified)", "简体中文"},
        {"zh-TW", "Chinese (Traditional)", "繁體中文"},
        {"en", "English", "English"},
        {"ja", "Japanese", "日本語"},
        {"ko", "Korean", "한국어"},
        {"fr", "French", "Français"},
        {"de", "German", "Deutsch"},
        {"es", "Spanish", "Español"},
        {"ru", "Russian", "Русский"},
        {"ar", "Arabic", "العربية"},
        {"pt", "Portuguese", "Português"},
        {"it", "Italian", "Italiano"},
        {"vi", "Vietnamese", "Tiếng Việt"},
        {"th", "Thai", "ไทย"},
        {"id", "Indonesian", "Bahasa Indonesia"},
    };
}

QString TranslationService::detectLanguage(const QString& text)
{
    Q_UNUSED(text);
    return QStringLiteral("autodetect");
}

TranslationService::TranslationService(
    iqtools::core::SettingsManager* settingsManager, QObject* parent)
    : QObject(parent)
    , m_settingsManager(settingsManager)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished, this,
            &TranslationService::onNetworkReply);
}

TranslationService::~TranslationService() = default;

void TranslationService::translate(const QString& text, const QString& sourceLang,
                                   const QString& targetLang)
{
    const QString normalizedSourceLanguage = normalizeLanguageCode(sourceLang);
    const QString normalizedTargetLanguage = normalizeLanguageCode(targetLang);
    const QString normalizedText = text.trimmed();

    if (m_isTranslating) {
        emit translationFailed(QStringLiteral("已有翻译任务正在进行中"));
        return;
    }

    if (normalizedText.isEmpty()) {
        emit translationFailed(QStringLiteral("请输入要翻译的文本"));
        return;
    }

    if (normalizedTargetLanguage.isEmpty() ||
        normalizedTargetLanguage == QStringLiteral("auto")) {
        emit translationFailed(QStringLiteral("请选择明确的目标语言"));
        return;
    }

    if (!normalizedSourceLanguage.isEmpty() &&
        normalizedSourceLanguage != QStringLiteral("auto") &&
        normalizedSourceLanguage == normalizedTargetLanguage) {
        m_lastResult = {normalizedText, normalizedSourceLanguage,
                        normalizedTargetLanguage, true, QString()};
        emit translationFinished(m_lastResult);
        return;
    }

    TranslationProviderSettings providerSettings;
    providerSettings.providerType = QStringLiteral("google_web");
    providerSettings.customMethod = QStringLiteral("POST");
    providerSettings.customHeadersJson = QStringLiteral("{}");
    if (m_settingsManager != nullptr) {
        providerSettings = m_settingsManager->translationProviderSettings();
    }

    m_activeProvider = translationProviderForType(providerSettings.providerType);
    if (m_activeProvider == nullptr) {
        const QString errorMessage = QStringLiteral("当前翻译 Provider 不受支持");
        m_lastResult = {QString(), QString(), QString(), false, errorMessage};
        emit translationFailed(errorMessage);
        return;
    }

    const QString settingsError =
        m_activeProvider->validateSettings(providerSettings);
    if (!settingsError.isEmpty()) {
        m_lastResult = {QString(), QString(), QString(), false, settingsError};
        emit translationFailed(settingsError);
        return;
    }

    m_activeProviderSettings = providerSettings;
    m_activeSourceLanguage =
        normalizedSourceLanguage.isEmpty() ? QStringLiteral("auto")
                                           : normalizedSourceLanguage;
    m_activeTargetLanguage = normalizedTargetLanguage;
    m_activeRequestContext = {normalizedText, m_activeSourceLanguage,
                              m_activeTargetLanguage};

    TranslationPreparedRequest preparedRequest;
    QString prepareErrorMessage;
    if (!m_activeProvider->prepareRequest(m_activeProviderSettings,
                                          m_activeRequestContext,
                                          &preparedRequest,
                                          &prepareErrorMessage)) {
        if (prepareErrorMessage.isEmpty()) {
            prepareErrorMessage = QStringLiteral("翻译请求初始化失败");
        }
        m_lastResult = {QString(), QString(), QString(), false,
                        prepareErrorMessage};
        emit translationFailed(prepareErrorMessage);
        return;
    }

    preparedRequest.networkRequest.setTransferTimeout(kTranslateTimeoutMs);

    m_isTranslating = true;
    emit translatingChanged(true);

    switch (preparedRequest.operation) {
    case QNetworkAccessManager::PostOperation:
        m_activeReply = m_networkManager->post(preparedRequest.networkRequest,
                                               preparedRequest.body);
        break;
    case QNetworkAccessManager::GetOperation:
    default:
        m_activeReply = m_networkManager->get(preparedRequest.networkRequest);
        break;
    }

    infra::logging::Logger::info(
        QStringLiteral("translation"),
        QStringLiteral("Translating via %1: %2 (%3 -> %4)")
            .arg(m_activeProvider->displayName(), normalizedText.left(50),
                 m_activeSourceLanguage, m_activeTargetLanguage));
}

void TranslationService::cancel()
{
    if (m_activeReply == nullptr) {
        return;
    }

    infra::logging::Logger::info(QStringLiteral("translation"),
                                 QStringLiteral("Translation canceled by user"));
    m_activeReply->abort();
}

bool TranslationService::isTranslating() const
{
    return m_isTranslating;
}

TranslationResult TranslationService::lastResult() const
{
    return m_lastResult;
}

void TranslationService::onNetworkReply(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply != m_activeReply) {
        return;
    }

    m_activeReply = nullptr;
    if (m_isTranslating) {
        m_isTranslating = false;
        emit translatingChanged(false);
    }

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        emit translationCanceled();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const QString errorMessage = friendlyNetworkError(reply);
        infra::logging::Logger::warning(
            QStringLiteral("translation"),
            QStringLiteral("Translation failed: %1").arg(errorMessage));
        m_lastResult = {QString(), QString(), QString(), false, errorMessage};
        emit translationFailed(errorMessage);
        return;
    }

    parseResponse(reply->readAll());
}

void TranslationService::parseResponse(const QByteArray& data)
{
    if (m_activeProvider == nullptr) {
        const QString errorMessage = QStringLiteral("当前翻译 Provider 不可用");
        m_lastResult = {QString(), QString(), QString(), false, errorMessage};
        emit translationFailed(errorMessage);
        return;
    }

    TranslationResult result;
    QString parseErrorMessage;
    if (!m_activeProvider->parseResponse(m_activeProviderSettings,
                                         m_activeRequestContext, data, &result,
                                         &parseErrorMessage)) {
        if (parseErrorMessage.isEmpty()) {
            parseErrorMessage = QStringLiteral("翻译服务返回了无效响应");
        }
        infra::logging::Logger::warning(
            QStringLiteral("translation"),
            QStringLiteral("Translation parse failed: %1")
                .arg(parseErrorMessage));
        m_lastResult = {QString(), QString(), QString(), false,
                        parseErrorMessage};
        emit translationFailed(parseErrorMessage);
        return;
    }

    if (result.sourceLanguage.isEmpty() &&
        m_activeSourceLanguage != QStringLiteral("auto")) {
        result.sourceLanguage = m_activeSourceLanguage;
    }
    if (result.targetLanguage.isEmpty()) {
        result.targetLanguage = m_activeTargetLanguage;
    }
    result.success = true;

    m_lastResult = result;

    infra::logging::Logger::info(
        QStringLiteral("translation"),
        QStringLiteral("Translation result via %1: %2")
            .arg(m_activeProvider->displayName(),
                 m_lastResult.translatedText.left(50)));

    emit translationFinished(m_lastResult);
}

QString TranslationService::normalizeLanguageCode(const QString& languageCode)
{
    const QString trimmed = languageCode.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QString lower =
        trimmed.toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (lower == QStringLiteral("auto") ||
        lower == QStringLiteral("autodetect")) {
        return QStringLiteral("auto");
    }
    if (lower.startsWith(QStringLiteral("zh-tw")) ||
        lower.contains(QStringLiteral("hant"))) {
        return QStringLiteral("zh-TW");
    }
    if (lower.startsWith(QStringLiteral("zh"))) {
        return QStringLiteral("zh-CN");
    }

    const int separatorIndex = lower.indexOf(QLatin1Char('-'));
    if (separatorIndex > 0) {
        return lower.left(separatorIndex);
    }
    return lower;
}

}  // namespace iqtools::core
