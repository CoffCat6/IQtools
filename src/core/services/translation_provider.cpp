#include "translation_provider.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkReply>

namespace iqtools::core {

namespace {

constexpr auto kProviderGoogleWeb = "google_web";
constexpr auto kProviderMyMemoryLegacy = "mymemory";
constexpr auto kProviderCustomRest = "custom_rest";
constexpr auto kGoogleTranslateWebApiBaseUrl =
    "https://translate.google.com/translate_a/single";
constexpr auto kGoogleTranslateWebPageUrl =
    "https://translate.google.com/?hl=zh-cn&sl=auto&tl=zh-CN&op=translate";

QString normalizeLanguageCode(const QString& languageCode)
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

QString googleResponseErrorMessage(const QJsonObject& rootObject)
{
    const QJsonValue errorValue = rootObject.value(QStringLiteral("error"));
    if (errorValue.isString()) {
        const QString details = errorValue.toString().trimmed();
        if (!details.isEmpty()) {
            return QStringLiteral("翻译失败：%1").arg(details);
        }
    }

    const int status = rootObject.value(QStringLiteral("status")).toInt();
    if (status == 429 || status == 403) {
        return QStringLiteral("翻译服务请求过于频繁，请稍后再试");
    }
    if (status != 0 && status != 200) {
        return QStringLiteral("翻译服务返回异常状态：%1").arg(status);
    }

    return QStringLiteral("翻译服务返回了无效数据");
}

QString joinGoogleTranslatedSentences(const QJsonArray& sentences)
{
    QString translatedText;
    for (const auto& sentenceValue : sentences) {
        const QJsonObject sentenceObject = sentenceValue.toObject();
        const QString sentence =
            sentenceObject.value(QStringLiteral("trans")).toString();
        if (!sentence.isEmpty()) {
            translatedText += sentence;
        }
    }
    return translatedText.trimmed();
}

QString googleTranslatedTextFromLegacyArray(const QJsonArray& rootArray)
{
    if (rootArray.isEmpty() || !rootArray.at(0).isArray()) {
        return QString();
    }

    QString translatedText;
    const QJsonArray sentenceArray = rootArray.at(0).toArray();
    for (const auto& sentenceValue : sentenceArray) {
        if (!sentenceValue.isArray()) {
            continue;
        }

        const QJsonArray sentenceParts = sentenceValue.toArray();
        if (sentenceParts.isEmpty()) {
            continue;
        }

        translatedText += sentenceParts.at(0).toString();
    }
    return translatedText.trimmed();
}

QString googleDetectedSourceLanguage(const QJsonObject& rootObject,
                                     const QJsonArray& rootArray)
{
    QString detected =
        normalizeLanguageCode(rootObject.value(QStringLiteral("src")).toString());
    if (!detected.isEmpty() && detected != QStringLiteral("auto")) {
        return detected;
    }

    if (rootArray.size() > 2) {
        detected = normalizeLanguageCode(rootArray.at(2).toString());
        if (!detected.isEmpty() && detected != QStringLiteral("auto")) {
            return detected;
        }
    }

    const QJsonObject ldResult =
        rootObject.value(QStringLiteral("ld_result")).toObject();
    const QJsonArray sourceLanguages =
        ldResult.value(QStringLiteral("srclangs")).toArray();
    for (const auto& sourceValue : sourceLanguages) {
        detected = normalizeLanguageCode(sourceValue.toString());
        if (!detected.isEmpty() && detected != QStringLiteral("auto")) {
            return detected;
        }
    }

    return QString();
}

QString escapeJsonString(const QString& value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    escaped.replace(QLatin1Char('\b'), QStringLiteral("\\b"));
    escaped.replace(QLatin1Char('\f'), QStringLiteral("\\f"));
    escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    escaped.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    escaped.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
    return escaped;
}

enum class TemplateEncoding {
    Raw,
    UrlEncoded,
    JsonEscaped,
};

QString resolveTemplateValue(const QString& placeholder,
                             const TranslationRequestContext& requestContext,
                             const TranslationProviderSettings& settings)
{
    if (placeholder == QStringLiteral("text")) {
        return requestContext.text;
    }
    if (placeholder == QStringLiteral("source")) {
        return requestContext.sourceLanguage;
    }
    if (placeholder == QStringLiteral("target")) {
        return requestContext.targetLanguage;
    }
    if (placeholder == QStringLiteral("apiKey")) {
        return settings.customApiKey;
    }
    return QString();
}

QString renderTemplate(const QString& input,
                       const TranslationRequestContext& requestContext,
                       const TranslationProviderSettings& settings,
                       TemplateEncoding encoding)
{
    QString output = input;
    const QStringList placeholders = {
        QStringLiteral("text"),
        QStringLiteral("source"),
        QStringLiteral("target"),
        QStringLiteral("apiKey"),
    };
    for (const QString& placeholder : placeholders) {
        QString value = resolveTemplateValue(placeholder, requestContext,
                                             settings);
        switch (encoding) {
        case TemplateEncoding::Raw:
            break;
        case TemplateEncoding::UrlEncoded:
            value = QString::fromLatin1(QUrl::toPercentEncoding(value));
            break;
        case TemplateEncoding::JsonEscaped:
            value = escapeJsonString(value);
            break;
        }
        output.replace(QStringLiteral("{{%1}}").arg(placeholder), value);
    }
    return output;
}

bool parseHeadersObject(const QString& headersJson, QJsonObject* headersObject,
                        QString* errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(headersJson.trimmed().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage =
                QStringLiteral("请求头 JSON 格式无效：%1")
                    .arg(parseError.errorString());
        }
        return false;
    }
    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage =
                QStringLiteral("请求头必须是 JSON 对象，例如 {\"Authorization\":\"Bearer xxx\"}");
        }
        return false;
    }

    if (headersObject != nullptr) {
        *headersObject = document.object();
    }
    return true;
}

bool hasContentTypeHeader(const QJsonObject& headersObject)
{
    for (auto it = headersObject.constBegin(); it != headersObject.constEnd();
         ++it) {
        if (it.key().compare(QStringLiteral("Content-Type"),
                             Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool resolveJsonPath(const QJsonValue& rootValue, const QString& path,
                     QJsonValue* resolvedValue)
{
    const QStringList segments =
        path.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (segments.isEmpty()) {
        return false;
    }

    QJsonValue currentValue = rootValue;
    for (const QString& segment : segments) {
        bool indexOk = false;
        const int index = segment.toInt(&indexOk);
        if (indexOk) {
            if (!currentValue.isArray()) {
                return false;
            }
            const QJsonArray array = currentValue.toArray();
            if (index < 0 || index >= array.size()) {
                return false;
            }
            currentValue = array.at(index);
            continue;
        }

        if (!currentValue.isObject()) {
            return false;
        }
        const QJsonObject object = currentValue.toObject();
        if (!object.contains(segment)) {
            return false;
        }
        currentValue = object.value(segment);
    }

    if (resolvedValue != nullptr) {
        *resolvedValue = currentValue;
    }
    return true;
}

QString jsonValueToString(const QJsonValue& value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true")
                              : QStringLiteral("false");
    }
    if (value.isObject()) {
        return QString::fromUtf8(
            QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if (value.isArray()) {
        return QString::fromUtf8(
            QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    return QString();
}

class GoogleWebTranslationProvider final : public ITranslationProvider {
public:
    QString id() const override
    {
        return QString::fromLatin1(kProviderGoogleWeb);
    }

    QString displayName() const override
    {
        return QStringLiteral("Google Translate Web");
    }

    QString validateSettings(
        const TranslationProviderSettings& settings) const override
    {
        Q_UNUSED(settings);
        return QString();
    }

    bool prepareRequest(const TranslationProviderSettings& settings,
                        const TranslationRequestContext& requestContext,
                        TranslationPreparedRequest* preparedRequest,
                        QString* errorMessage) const override
    {
        Q_UNUSED(settings);
        if (preparedRequest == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("翻译请求初始化失败");
            }
            return false;
        }

        QUrl url(QString::fromLatin1(kGoogleTranslateWebApiBaseUrl));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("client"), QStringLiteral("gtx"));
        query.addQueryItem(QStringLiteral("dj"), QStringLiteral("1"));
        query.addQueryItem(QStringLiteral("dt"), QStringLiteral("t"));
        query.addQueryItem(QStringLiteral("sl"), requestContext.sourceLanguage);
        query.addQueryItem(QStringLiteral("tl"), requestContext.targetLanguage);
        query.addQueryItem(QStringLiteral("hl"), QStringLiteral("zh-CN"));
        query.addQueryItem(QStringLiteral("source"), QStringLiteral("input"));
        query.addQueryItem(QStringLiteral("q"), requestContext.text);
        url.setQuery(query);

        preparedRequest->networkRequest = QNetworkRequest(url);
        preparedRequest->networkRequest.setHeader(
            QNetworkRequest::UserAgentHeader, QStringLiteral("IQtools/1.0"));
        preparedRequest->networkRequest.setRawHeader("Accept",
                                                     "application/json");
        preparedRequest->networkRequest.setRawHeader(
            "Referer", QByteArray(kGoogleTranslateWebPageUrl));
        preparedRequest->operation = QNetworkAccessManager::GetOperation;
        preparedRequest->body.clear();
        return true;
    }

    bool parseResponse(const TranslationProviderSettings& settings,
                       const TranslationRequestContext& requestContext,
                       const QByteArray& responseData, TranslationResult* result,
                       QString* errorMessage) const override
    {
        Q_UNUSED(settings);
        if (result == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("翻译结果写入失败");
            }
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (errorMessage != nullptr) {
                *errorMessage =
                    QStringLiteral("翻译服务响应解析失败：%1")
                        .arg(parseError.errorString());
            }
            return false;
        }

        const QJsonObject root = document.object();
        const QJsonArray legacyRootArray = document.isArray()
                                               ? document.array()
                                               : QJsonArray();
        if (!root.isEmpty()) {
            const int responseStatus =
                root.value(QStringLiteral("status")).toInt(200);
            if (responseStatus != 200) {
                if (errorMessage != nullptr) {
                    *errorMessage = googleResponseErrorMessage(root);
                }
                return false;
            }
        }

        QString translatedText;
        if (!root.isEmpty()) {
            translatedText = joinGoogleTranslatedSentences(
                root.value(QStringLiteral("sentences")).toArray());
        }
        if (translatedText.isEmpty() && !legacyRootArray.isEmpty()) {
            translatedText = googleTranslatedTextFromLegacyArray(legacyRootArray);
        }
        if (translatedText.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = googleResponseErrorMessage(root);
            }
            return false;
        }

        const QString sourceLanguage =
            requestContext.sourceLanguage == QStringLiteral("auto")
                ? googleDetectedSourceLanguage(root, legacyRootArray)
                : requestContext.sourceLanguage;

        *result = {translatedText,
                   sourceLanguage,
                   requestContext.targetLanguage,
                   true,
                   QString()};
        return true;
    }
};

class CustomRestTranslationProvider final : public ITranslationProvider {
public:
    QString id() const override
    {
        return QString::fromLatin1(kProviderCustomRest);
    }

    QString displayName() const override
    {
        return QStringLiteral("Custom REST API");
    }

    QString validateSettings(
        const TranslationProviderSettings& settings) const override
    {
        const QUrl endpoint(settings.customEndpoint);
        if (!endpoint.isValid() || endpoint.scheme().trimmed().isEmpty() ||
            endpoint.host().trimmed().isEmpty()) {
            return QStringLiteral("请输入有效的翻译接口 URL");
        }

        if (settings.customMethod != QStringLiteral("GET") &&
            settings.customMethod != QStringLiteral("POST")) {
            return QStringLiteral("自定义翻译接口仅支持 GET 或 POST");
        }

        QString parseErrorMessage;
        if (!parseHeadersObject(settings.customHeadersJson, nullptr,
                                &parseErrorMessage)) {
            return parseErrorMessage;
        }

        if (settings.customResultTextPath.trimmed().isEmpty()) {
            return QStringLiteral("请填写译文字段路径");
        }

        if (settings.customMethod == QStringLiteral("GET") &&
            settings.customQueryTemplate.trimmed().isEmpty()) {
            return QStringLiteral("GET 接口必须填写 Query Template");
        }

        if (settings.customMethod == QStringLiteral("POST") &&
            settings.customBodyTemplate.trimmed().isEmpty()) {
            return QStringLiteral("POST 接口必须填写 Body Template");
        }

        return QString();
    }

    bool prepareRequest(const TranslationProviderSettings& settings,
                        const TranslationRequestContext& requestContext,
                        TranslationPreparedRequest* preparedRequest,
                        QString* errorMessage) const override
    {
        if (preparedRequest == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("翻译请求初始化失败");
            }
            return false;
        }

        const QString validationError = validateSettings(settings);
        if (!validationError.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = validationError;
            }
            return false;
        }

        QUrl url(settings.customEndpoint);
        QJsonObject headersObject;
        if (!parseHeadersObject(settings.customHeadersJson, &headersObject,
                                errorMessage)) {
            return false;
        }

        preparedRequest->networkRequest = QNetworkRequest(url);
        preparedRequest->networkRequest.setHeader(
            QNetworkRequest::UserAgentHeader, QStringLiteral("IQtools/1.0"));
        preparedRequest->body.clear();

        for (auto it = headersObject.constBegin(); it != headersObject.constEnd();
             ++it) {
            const QString headerName = it.key().trimmed();
            if (headerName.isEmpty()) {
                continue;
            }

            const QString headerValue =
                renderTemplate(jsonValueToString(it.value()), requestContext,
                               settings, TemplateEncoding::Raw);
            preparedRequest->networkRequest.setRawHeader(
                headerName.toUtf8(), headerValue.toUtf8());
        }

        if (settings.customMethod == QStringLiteral("GET")) {
            const QString renderedQuery =
                renderTemplate(settings.customQueryTemplate, requestContext,
                               settings, TemplateEncoding::UrlEncoded);
            QString currentQuery = url.query(QUrl::FullyEncoded);
            if (!renderedQuery.isEmpty()) {
                if (currentQuery.isEmpty()) {
                    currentQuery = renderedQuery;
                } else {
                    currentQuery += QStringLiteral("&") + renderedQuery;
                }
            }
            url.setQuery(currentQuery);
            preparedRequest->networkRequest.setUrl(url);
            preparedRequest->operation = QNetworkAccessManager::GetOperation;
            return true;
        }

        if (!hasContentTypeHeader(headersObject)) {
            preparedRequest->networkRequest.setRawHeader(
                "Content-Type", "application/json");
        }

        preparedRequest->operation = QNetworkAccessManager::PostOperation;
        preparedRequest->body = renderTemplate(settings.customBodyTemplate,
                                               requestContext, settings,
                                               TemplateEncoding::JsonEscaped)
                                    .toUtf8();
        return true;
    }

    bool parseResponse(const TranslationProviderSettings& settings,
                       const TranslationRequestContext& requestContext,
                       const QByteArray& responseData, TranslationResult* result,
                       QString* errorMessage) const override
    {
        if (result == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("翻译结果写入失败");
            }
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (errorMessage != nullptr) {
                *errorMessage =
                    QStringLiteral("自定义翻译接口返回了无法解析的 JSON 响应：%1")
                        .arg(parseError.errorString());
            }
            return false;
        }

        const QJsonValue rootValue =
            document.isArray() ? QJsonValue(document.array())
                               : QJsonValue(document.object());
        QJsonValue translatedTextValue;
        if (!resolveJsonPath(rootValue, settings.customResultTextPath,
                             &translatedTextValue)) {
            if (errorMessage != nullptr) {
                *errorMessage =
                    QStringLiteral("未在响应中找到译文字段：%1")
                        .arg(settings.customResultTextPath);
            }
            return false;
        }

        const QString translatedText = jsonValueToString(translatedTextValue);
        if (translatedText.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("响应中的译文字段为空");
            }
            return false;
        }

        QString detectedSourceLanguage;
        if (!settings.customDetectedSourcePath.trimmed().isEmpty()) {
            QJsonValue detectedSourceValue;
            if (resolveJsonPath(rootValue, settings.customDetectedSourcePath,
                                &detectedSourceValue)) {
                detectedSourceLanguage =
                    normalizeLanguageCode(jsonValueToString(detectedSourceValue));
            }
        }

        *result = {translatedText,
                   requestContext.sourceLanguage == QStringLiteral("auto")
                       ? detectedSourceLanguage
                       : requestContext.sourceLanguage,
                   requestContext.targetLanguage,
                   true,
                   QString()};
        return true;
    }
};

const GoogleWebTranslationProvider kGoogleWebProvider;
const CustomRestTranslationProvider kCustomRestProvider;

}  // namespace

QString normalizeTranslationProviderType(const QString& providerType)
{
    const QString normalized = providerType.trimmed().toLower();
    if (normalized == QString::fromLatin1(kProviderCustomRest)) {
        return QString::fromLatin1(kProviderCustomRest);
    }
    if (normalized == QString::fromLatin1(kProviderMyMemoryLegacy) ||
        normalized == QString::fromLatin1(kProviderGoogleWeb)) {
        return QString::fromLatin1(kProviderGoogleWeb);
    }
    return QString::fromLatin1(kProviderGoogleWeb);
}

QString translationProviderDisplayName(const QString& providerType)
{
    const QString normalized = normalizeTranslationProviderType(providerType);
    if (normalized == QString::fromLatin1(kProviderCustomRest)) {
        return kCustomRestProvider.displayName();
    }
    return kGoogleWebProvider.displayName();
}

QStringList translationProviderTypes()
{
    return {QString::fromLatin1(kProviderGoogleWeb),
            QString::fromLatin1(kProviderCustomRest)};
}

QStringList translationProviderMethodOptions()
{
    return {QStringLiteral("GET"), QStringLiteral("POST")};
}

QString validateTranslationProviderSettings(
    const TranslationProviderSettings& settings)
{
    const ITranslationProvider* provider =
        translationProviderForType(settings.providerType);
    if (provider == nullptr) {
        return QStringLiteral("当前翻译 Provider 不受支持");
    }
    return provider->validateSettings(settings);
}

const ITranslationProvider* translationProviderForType(
    const QString& providerType)
{
    const QString normalized = normalizeTranslationProviderType(providerType);
    if (normalized == QString::fromLatin1(kProviderCustomRest)) {
        return &kCustomRestProvider;
    }
    return &kGoogleWebProvider;
}

}  // namespace iqtools::core
