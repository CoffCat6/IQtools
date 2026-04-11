#include "translation_service.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkRequest>

#include "infra/logging/logger.h"

namespace iqtools::core {

// MyMemory API endpoint
static constexpr const char* kApiBaseUrl = "https://api.mymemory.translated.net/get";
static constexpr int kTranslateTimeoutMs = 15000;

namespace {

QString normalizeLanguageCode(const QString& languageCode)
{
    const QString trimmed = languageCode.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QString normalized = trimmed;
    const QString lower = normalized.toLower().replace(QLatin1Char('_'),
                                                       QLatin1Char('-'));
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

QString detectSourceLanguage(const QJsonArray& matches)
{
    for (const auto& matchValue : matches) {
        const QJsonObject matchObject = matchValue.toObject();
        const QString source =
            normalizeLanguageCode(matchObject.value(QStringLiteral("source"))
                                      .toString());
        if (!source.isEmpty() && source != QStringLiteral("auto")) {
            return source;
        }
    }
    return QString();
}

QString responseErrorMessage(const QJsonObject& rootObject)
{
    const QString details =
        rootObject.value(QStringLiteral("responseDetails")).toString().trimmed();
    if (!details.isEmpty() && details != QStringLiteral("null")) {
        return QStringLiteral("翻译失败：%1").arg(details);
    }

    const int status = rootObject.value(QStringLiteral("responseStatus")).toInt();
    if (status == 429) {
        return QStringLiteral("翻译服务请求过于频繁，请稍后再试");
    }
    if (status != 0 && status != 200) {
        return QStringLiteral("翻译服务返回异常状态：%1").arg(status);
    }

    return QStringLiteral("翻译服务返回了无效数据");
}

}  // namespace

QString LanguagePair::toApiString() const
{
    return source + "|" + target;
}

QList<LanguageInfo> TranslationService::supportedLanguages()
{
    // Most commonly used languages
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
    // MyMemory doesn't have explicit detect, use 'autodetect' as source
    Q_UNUSED(text);
    return QStringLiteral("autodetect");
}

TranslationService::TranslationService(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TranslationService::onNetworkReply);
}

TranslationService::~TranslationService() = default;

void TranslationService::translate(const QString& text, const QString& sourceLang, const QString& targetLang)
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
        // No translation needed, just return original
        m_lastResult = {
            normalizedText,
            normalizedSourceLanguage,
            normalizedTargetLanguage,
            true,
            QString()
        };
        emit translationFinished(m_lastResult);
        return;
    }

    m_isTranslating = true;
    emit translatingChanged(true);
    m_activeSourceLanguage =
        normalizedSourceLanguage.isEmpty() ? QStringLiteral("auto")
                                           : normalizedSourceLanguage;
    m_activeTargetLanguage = normalizedTargetLanguage;

    // Build URL with query parameters
    QUrl url(QString::fromLatin1(kApiBaseUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), normalizedText);
    query.addQueryItem(QStringLiteral("langpair"),
                       QStringLiteral("%1|%2").arg(m_activeSourceLanguage,
                                                   m_activeTargetLanguage));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("IQtools/1.0"));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(kTranslateTimeoutMs);

    m_activeReply = m_networkManager->get(request);

    infra::logging::Logger::info(
        QStringLiteral("translation"),
        QStringLiteral("Translating: %1 (%2 -> %3)")
            .arg(normalizedText.left(50), m_activeSourceLanguage,
                 m_activeTargetLanguage));
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
        const QString errorMsg = friendlyNetworkError(reply);
        infra::logging::Logger::warning(
            QStringLiteral("translation"),
            QStringLiteral("Translation failed: %1").arg(errorMsg));
        m_lastResult = {QString(), QString(), QString(), false, errorMsg};
        emit translationFailed(errorMsg);
        return;
    }

    const QByteArray data = reply->readAll();
    parseResponse(data);
}

void TranslationService::parseResponse(const QByteArray& data)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        const QString errorMsg = QStringLiteral("Failed to parse response: %1").arg(parseError.errorString());
        infra::logging::Logger::warning(QStringLiteral("translation"), errorMsg);
        m_lastResult = {QString(), QString(), QString(), false, errorMsg};
        emit translationFailed(errorMsg);
        return;
    }

    const QJsonObject root = doc.object();
    const int responseStatus =
        root.value(QStringLiteral("responseStatus")).toInt(200);
    if (responseStatus != 200) {
        const QString errorMsg = responseErrorMessage(root);
        infra::logging::Logger::warning(QStringLiteral("translation"), errorMsg);
        m_lastResult = {QString(), QString(), QString(), false, errorMsg};
        emit translationFailed(errorMsg);
        return;
    }

    const QJsonObject responseData = root.value(QStringLiteral("responseData")).toObject();

    if (responseData.isEmpty()) {
        const QString errorMsg = QStringLiteral("翻译服务返回了无效响应");
        infra::logging::Logger::warning(QStringLiteral("translation"), errorMsg);
        m_lastResult = {QString(), QString(), QString(), false, errorMsg};
        emit translationFailed(errorMsg);
        return;
    }

    const QString translatedText =
        responseData.value(QStringLiteral("translatedText")).toString();

    if (translatedText.isEmpty()) {
        const QString errorMsg = responseErrorMessage(root);
        infra::logging::Logger::warning(QStringLiteral("translation"), errorMsg);
        m_lastResult = {QString(), QString(), QString(), false, errorMsg};
        emit translationFailed(errorMsg);
        return;
    }

    const QJsonArray matches = root.value(QStringLiteral("matches")).toArray();
    const QString detectedSourceLanguage =
        (m_activeSourceLanguage == QStringLiteral("auto"))
            ? detectSourceLanguage(matches)
            : m_activeSourceLanguage;

    m_lastResult = {
        translatedText,
        detectedSourceLanguage,
        m_activeTargetLanguage,
        true,
        QString()
    };

    infra::logging::Logger::info(
        QStringLiteral("translation"),
        QStringLiteral("Translation result: %1").arg(translatedText.left(50)));

    emit translationFinished(m_lastResult);
}

}  // namespace iqtools::core
