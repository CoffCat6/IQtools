#include "translation_controller.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

#include "core/services/translation_service.h"
#include "core/settings/settings_manager.h"
#include "infra/logging/logger.h"

namespace iqtools::app::bridge {

namespace {

QString normalizeLanguageCodeInternal(const QString& code)
{
    const QString trimmed = code.trimmed();
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

QVariantMap toLanguageMap(const iqtools::core::LanguageInfo& languageInfo)
{
    QVariantMap map;
    map[QStringLiteral("code")] = languageInfo.code;
    map[QStringLiteral("name")] = languageInfo.name;
    map[QStringLiteral("nativeName")] = languageInfo.nativeName;
    return map;
}

}  // namespace

TranslationController::TranslationController(
    iqtools::core::TranslationService* translationService,
    iqtools::core::SettingsManager* settingsManager, QObject* parent)
    : QObject(parent)
    , m_translationService(translationService)
    , m_settingsManager(settingsManager)
    , m_sourceLanguages(buildLanguageList(true))
    , m_targetLanguages(buildLanguageList(false))
{
    if (m_settingsManager != nullptr) {
        m_sourceLanguage = normalizeLanguageCodeInternal(
            m_settingsManager->translationSourceLanguage());
        m_targetLanguage = normalizeLanguageCodeInternal(
            m_settingsManager->translationTargetLanguage());
    }

    if (m_sourceLanguage.isEmpty()) {
        m_sourceLanguage = QStringLiteral("auto");
    }
    if (m_targetLanguage.isEmpty() ||
        m_targetLanguage == QStringLiteral("auto")) {
        m_targetLanguage = QStringLiteral("zh-CN");
    }
    if (languageCodeAt(m_sourceLanguages, sourceLanguageIndex()) !=
        m_sourceLanguage) {
        m_sourceLanguage = QStringLiteral("auto");
    }
    if (languageCodeAt(m_targetLanguages, targetLanguageIndex()) !=
        m_targetLanguage) {
        m_targetLanguage = QStringLiteral("zh-CN");
    }

    setStatus(QStringLiteral("info"),
              QStringLiteral("等待输入要翻译的文本"));

    if (m_translationService != nullptr) {
        connect(m_translationService,
                &iqtools::core::TranslationService::translationFinished, this,
                &TranslationController::onTranslationFinished);
        connect(m_translationService,
                &iqtools::core::TranslationService::translationFailed, this,
                &TranslationController::onTranslationFailed);
        connect(m_translationService,
                &iqtools::core::TranslationService::translatingChanged, this,
                &TranslationController::onTranslatingChanged);
        connect(m_translationService,
                &iqtools::core::TranslationService::translationCanceled, this,
                &TranslationController::onTranslationCanceled);
        m_isTranslating = m_translationService->isTranslating();
    }
}

TranslationController::~TranslationController() = default;

QString TranslationController::sourceText() const
{
    return m_sourceText;
}

QString TranslationController::translatedText() const
{
    return m_translatedText;
}

QString TranslationController::sourceLanguage() const
{
    return m_sourceLanguage;
}

QString TranslationController::targetLanguage() const
{
    return m_targetLanguage;
}

int TranslationController::sourceLanguageIndex() const
{
    return indexOfLanguage(m_sourceLanguages, m_sourceLanguage);
}

int TranslationController::targetLanguageIndex() const
{
    return indexOfLanguage(m_targetLanguages, m_targetLanguage);
}

QString TranslationController::resultSourceLanguage() const
{
    return m_resultSourceLanguage;
}

bool TranslationController::isTranslating() const
{
    return m_isTranslating;
}

bool TranslationController::resultStale() const
{
    return m_resultStale;
}

QString TranslationController::errorMessage() const
{
    return m_lastErrorMessage;
}

QString TranslationController::statusMessage() const
{
    return m_statusMessage;
}

QString TranslationController::statusLevel() const
{
    return m_statusLevel;
}

QVariantList TranslationController::availableLanguages() const
{
    return m_sourceLanguages;
}

QVariantList TranslationController::sourceLanguages() const
{
    return m_sourceLanguages;
}

QVariantList TranslationController::targetLanguages() const
{
    return m_targetLanguages;
}

void TranslationController::setSourceText(const QString& text)
{
    if (m_sourceText == text) {
        return;
    }

    m_sourceText = text;
    emit sourceTextChanged();
    setErrorMessage(QString());

    if (m_sourceText.trimmed().isEmpty()) {
        if (!m_translatedText.isEmpty()) {
            m_translatedText.clear();
            emit translatedTextChanged();
        }
        if (!m_resultSourceLanguage.isEmpty()) {
            m_resultSourceLanguage.clear();
            emit resultSourceLanguageChanged();
        }
        setResultStale(false);
        setStatus(QStringLiteral("info"),
                  QStringLiteral("等待输入要翻译的文本"));
        return;
    }

    markResultStale();
}

void TranslationController::setSourceLanguage(const QString& lang)
{
    const QString normalizedLanguage = normalizeLanguageCodeInternal(lang);
    const QString normalized =
        normalizedLanguage.isEmpty() ? QStringLiteral("auto")
                                     : normalizedLanguage;
    if (m_sourceLanguage == normalized) {
        return;
    }

    m_sourceLanguage = normalized;
    if (m_settingsManager != nullptr) {
        m_settingsManager->setTranslationSourceLanguage(m_sourceLanguage);
    }

    emit sourceLanguageChanged();
    setErrorMessage(QString());
    markResultStale();
}

void TranslationController::setTargetLanguage(const QString& lang)
{
    QString normalized = normalizeLanguageCodeInternal(lang);
    if (normalized.isEmpty() || normalized == QStringLiteral("auto")) {
        normalized = QStringLiteral("zh-CN");
    }
    if (m_targetLanguage == normalized) {
        return;
    }

    m_targetLanguage = normalized;
    if (m_settingsManager != nullptr) {
        m_settingsManager->setTranslationTargetLanguage(m_targetLanguage);
    }

    emit targetLanguageChanged();
    setErrorMessage(QString());
    markResultStale();
}

void TranslationController::translate()
{
    if (m_translationService == nullptr) {
        setErrorMessage(QStringLiteral("翻译服务当前不可用"));
        setStatus(QStringLiteral("error"), m_lastErrorMessage);
        return;
    }

    if (m_sourceText.trimmed().isEmpty()) {
        setErrorMessage(QStringLiteral("请输入要翻译的文本"));
        setStatus(QStringLiteral("error"), m_lastErrorMessage);
        return;
    }

    setErrorMessage(QString());
    if (!m_resultSourceLanguage.isEmpty()) {
        m_resultSourceLanguage.clear();
        emit resultSourceLanguageChanged();
    }
    setStatus(QStringLiteral("info"), QStringLiteral("正在翻译，请稍候"));
    m_translationService->translate(m_sourceText, m_sourceLanguage,
                                    m_targetLanguage);
}

void TranslationController::cancelTranslation()
{
    if (m_translationService == nullptr || !m_isTranslating) {
        return;
    }

    m_translationService->cancel();
}

void TranslationController::swapLanguages()
{
    if (!canSwapLanguages()) {
        setStatus(QStringLiteral("warning"),
                  QStringLiteral("自动检测模式下，请先完成一次翻译后再交换语言"));
        return;
    }

    const QString resolvedSourceLanguage =
        (m_sourceLanguage == QStringLiteral("auto")) ? m_resultSourceLanguage
                                                     : m_sourceLanguage;
    const QString previousSourceText = m_sourceText;
    const QString previousTranslatedText = m_translatedText;

    m_sourceLanguage = m_targetLanguage;
    m_targetLanguage = resolvedSourceLanguage;
    if (m_settingsManager != nullptr) {
        m_settingsManager->setTranslationSourceLanguage(m_sourceLanguage);
        m_settingsManager->setTranslationTargetLanguage(m_targetLanguage);
    }

    m_resultSourceLanguage.clear();
    setResultStale(false);
    setErrorMessage(QString());

    emit sourceLanguageChanged();
    emit targetLanguageChanged();
    emit resultSourceLanguageChanged();

    if (!previousTranslatedText.isEmpty()) {
        m_sourceText = previousTranslatedText;
        m_translatedText = previousSourceText;
        emit sourceTextChanged();
        emit translatedTextChanged();
    }

    setStatus(QStringLiteral("info"),
              QStringLiteral("已交换源语言和目标语言"));
    infra::logging::Logger::info(
        QStringLiteral("translation"),
        QStringLiteral("Swapped languages: %1 -> %2")
            .arg(m_sourceLanguage, m_targetLanguage));
}

void TranslationController::clear()
{
    const bool hadSourceText = !m_sourceText.isEmpty();
    const bool hadTranslatedText = !m_translatedText.isEmpty();
    const bool hadDetectedLanguage = !m_resultSourceLanguage.isEmpty();

    m_sourceText.clear();
    m_translatedText.clear();
    m_resultSourceLanguage.clear();
    setResultStale(false);
    setErrorMessage(QString());

    if (hadSourceText) {
        emit sourceTextChanged();
    }
    if (hadTranslatedText) {
        emit translatedTextChanged();
    }
    if (hadDetectedLanguage) {
        emit resultSourceLanguageChanged();
    }

    setStatus(QStringLiteral("info"),
              QStringLiteral("已清空翻译内容"));
}

void TranslationController::pasteSource()
{
    const QString clipboardText = QGuiApplication::clipboard()->text();
    if (clipboardText.trimmed().isEmpty()) {
        setStatus(QStringLiteral("warning"),
                  QStringLiteral("剪贴板中没有可粘贴的文本"));
        return;
    }

    setSourceText(clipboardText);
    setStatus(QStringLiteral("info"),
              QStringLiteral("已从剪贴板粘贴文本"));
}

void TranslationController::copySource()
{
    if (m_sourceText.isEmpty()) {
        setStatus(QStringLiteral("warning"),
                  QStringLiteral("当前没有可复制的原文"));
        return;
    }

    copyTextToClipboard(m_sourceText);
    setStatus(QStringLiteral("success"),
              QStringLiteral("已复制原文到剪贴板"));
}

void TranslationController::copyResult()
{
    if (m_translatedText.isEmpty()) {
        setStatus(QStringLiteral("warning"),
                  QStringLiteral("当前没有可复制的译文"));
        return;
    }

    copyTextToClipboard(m_translatedText);
    setStatus(QStringLiteral("success"),
              QStringLiteral("已复制译文到剪贴板"));
}

void TranslationController::setSourceLanguageByIndex(int index)
{
    setSourceLanguage(languageCodeAt(m_sourceLanguages, index));
}

void TranslationController::setTargetLanguageByIndex(int index)
{
    setTargetLanguage(languageCodeAt(m_targetLanguages, index));
}

QString TranslationController::languageDisplayName(const QString& code) const
{
    const QString normalized = normalizeLanguageCodeInternal(code);
    const auto languageLists = {m_sourceLanguages, m_targetLanguages};
    for (const auto& languages : languageLists) {
        for (const auto& languageValue : languages) {
            const QVariantMap languageMap = languageValue.toMap();
            if (languageMap.value(QStringLiteral("code")).toString() ==
                normalized) {
                return languageMap.value(QStringLiteral("nativeName"))
                    .toString();
            }
        }
    }
    return code;
}

bool TranslationController::canSwapLanguages() const
{
    if (m_targetLanguage.isEmpty() ||
        m_targetLanguage == QStringLiteral("auto")) {
        return false;
    }
    if (m_sourceLanguage != QStringLiteral("auto")) {
        return true;
    }
    return !m_resultSourceLanguage.isEmpty();
}

void TranslationController::onTranslatingChanged(bool translating)
{
    if (m_isTranslating == translating) {
        return;
    }

    m_isTranslating = translating;
    emit translatingChanged();

    if (m_isTranslating) {
        setStatus(QStringLiteral("info"),
                  QStringLiteral("正在翻译，请稍候"));
    } else if (m_lastErrorMessage.isEmpty() && !m_resultStale &&
               !m_translatedText.isEmpty()) {
        setStatus(QStringLiteral("success"),
                  QStringLiteral("翻译完成"));
    } else if (m_sourceText.trimmed().isEmpty()) {
        setStatus(QStringLiteral("info"),
                  QStringLiteral("等待输入要翻译的文本"));
    }
}

void TranslationController::onTranslationFinished(
    const iqtools::core::TranslationResult& result)
{
    const QString normalizedResultSourceLanguage =
        normalizeLanguageCodeInternal(result.sourceLanguage);
    const QString resolvedSourceLanguage =
        (m_sourceLanguage == QStringLiteral("auto"))
            ? normalizedResultSourceLanguage
            : QString();

    const bool translatedTextUpdated =
        (m_translatedText != result.translatedText);
    const bool detectedLanguageChanged =
        (m_resultSourceLanguage != resolvedSourceLanguage);

    m_translatedText = result.translatedText;
    m_resultSourceLanguage =
        (resolvedSourceLanguage == QStringLiteral("auto")) ? QString()
                                                           : resolvedSourceLanguage;
    setResultStale(false);
    setErrorMessage(QString());

    if (translatedTextUpdated) {
        emit translatedTextChanged();
    }
    if (detectedLanguageChanged) {
        emit resultSourceLanguageChanged();
    }

    setStatus(QStringLiteral("success"),
              QStringLiteral("翻译完成，可继续复制或交换语言"));

    infra::logging::Logger::info(
        QStringLiteral("translation"),
        QStringLiteral("Translation completed: %1")
            .arg(m_translatedText.left(30)));
}

void TranslationController::onTranslationFailed(const QString& errorMessage)
{
    if (!m_translatedText.isEmpty()) {
        setResultStale(true);
    }
    setErrorMessage(errorMessage);
    setStatus(QStringLiteral("error"), errorMessage);
}

void TranslationController::onTranslationCanceled()
{
    if (!m_translatedText.isEmpty()) {
        setResultStale(true);
    }
    setStatus(QStringLiteral("warning"),
              QStringLiteral("已取消当前翻译"));
}

void TranslationController::setResultStale(bool stale)
{
    if (m_resultStale == stale) {
        return;
    }

    m_resultStale = stale;
    emit resultStaleChanged();
}

void TranslationController::setErrorMessage(const QString& errorMessage)
{
    if (m_lastErrorMessage == errorMessage) {
        return;
    }

    m_lastErrorMessage = errorMessage;
    emit errorMessageChanged();
    if (!m_lastErrorMessage.isEmpty()) {
        emit errorOccurred(m_lastErrorMessage);
    }
}

void TranslationController::setStatus(const QString& level,
                                      const QString& message)
{
    if (m_statusLevel == level && m_statusMessage == message) {
        return;
    }

    m_statusLevel = level;
    m_statusMessage = message;
    emit statusChanged();
}

void TranslationController::copyTextToClipboard(const QString& text) const
{
    QGuiApplication::clipboard()->setText(text);
}

int TranslationController::indexOfLanguage(const QVariantList& languages,
                                           const QString& code) const
{
    const QString normalized = normalizeLanguageCodeInternal(code);
    for (int i = 0; i < languages.size(); ++i) {
        const QVariantMap languageMap = languages[i].toMap();
        if (languageMap.value(QStringLiteral("code")).toString() ==
            normalized) {
            return i;
        }
    }
    return 0;
}

QString TranslationController::languageCodeAt(const QVariantList& languages,
                                              int index) const
{
    if (index < 0 || index >= languages.size()) {
        return QString();
    }
    return languages[index]
        .toMap()
        .value(QStringLiteral("code"))
        .toString();
}

QVariantList TranslationController::buildLanguageList(bool includeAuto)
{
    QVariantList result;
    const auto languages = iqtools::core::TranslationService::supportedLanguages();
    for (const auto& languageInfo : languages) {
        if (!includeAuto && languageInfo.code == QStringLiteral("auto")) {
            continue;
        }
        result.append(toLanguageMap(languageInfo));
    }
    return result;
}

QString TranslationController::normalizeLanguageCode(const QString& code)
{
    return normalizeLanguageCodeInternal(code);
}

void TranslationController::markResultStale()
{
    if (m_isTranslating) {
        setStatus(QStringLiteral("info"),
                  QStringLiteral("正在翻译，请稍候"));
        return;
    }

    if (m_translatedText.isEmpty()) {
        setResultStale(false);
        setStatus(QStringLiteral("info"),
                  QStringLiteral("准备就绪，可开始翻译"));
        return;
    }

    setResultStale(true);
    setStatus(QStringLiteral("warning"),
              QStringLiteral("原文或语言已变更，当前译文可能已过期"));
}

}  // namespace iqtools::app::bridge
