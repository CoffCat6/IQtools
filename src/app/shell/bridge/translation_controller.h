#pragma once

#ifndef IQTOOLS_TRANSLATION_CONTROLLER_H
#define IQTOOLS_TRANSLATION_CONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>

#include "core/services/translation_service.h"

namespace iqtools::core {
class TranslationService;
class SettingsManager;
}

namespace iqtools::app::bridge {

class TranslationController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString sourceText READ sourceText WRITE setSourceText NOTIFY sourceTextChanged)
    Q_PROPERTY(QString translatedText READ translatedText NOTIFY translatedTextChanged)
    Q_PROPERTY(QString sourceLanguage READ sourceLanguage WRITE setSourceLanguage NOTIFY sourceLanguageChanged)
    Q_PROPERTY(QString targetLanguage READ targetLanguage WRITE setTargetLanguage NOTIFY targetLanguageChanged)
    Q_PROPERTY(int sourceLanguageIndex READ sourceLanguageIndex NOTIFY sourceLanguageChanged)
    Q_PROPERTY(int targetLanguageIndex READ targetLanguageIndex NOTIFY targetLanguageChanged)
    Q_PROPERTY(QString resultSourceLanguage READ resultSourceLanguage NOTIFY resultSourceLanguageChanged)
    Q_PROPERTY(bool isTranslating READ isTranslating NOTIFY translatingChanged)
    Q_PROPERTY(bool resultStale READ resultStale NOTIFY resultStaleChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(QString statusLevel READ statusLevel NOTIFY statusChanged)
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages CONSTANT)
    Q_PROPERTY(QVariantList sourceLanguages READ sourceLanguages CONSTANT)
    Q_PROPERTY(QVariantList targetLanguages READ targetLanguages CONSTANT)

public:
    explicit TranslationController(iqtools::core::TranslationService* translationService,
                                   iqtools::core::SettingsManager* settingsManager = nullptr,
                                   QObject* parent = nullptr);
    ~TranslationController() override;

    QString sourceText() const;
    QString translatedText() const;
    QString sourceLanguage() const;
    QString targetLanguage() const;
    int sourceLanguageIndex() const;
    int targetLanguageIndex() const;
    QString resultSourceLanguage() const;
    bool isTranslating() const;
    bool resultStale() const;
    QString errorMessage() const;
    QString statusMessage() const;
    QString statusLevel() const;
    QVariantList availableLanguages() const;
    QVariantList sourceLanguages() const;
    QVariantList targetLanguages() const;

    void setSourceText(const QString& text);
    void setSourceLanguage(const QString& lang);
    void setTargetLanguage(const QString& lang);

    Q_INVOKABLE void translate();
    Q_INVOKABLE void cancelTranslation();
    Q_INVOKABLE void swapLanguages();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void pasteSource();
    Q_INVOKABLE void copySource();
    Q_INVOKABLE void copyResult();
    Q_INVOKABLE void setSourceLanguageByIndex(int index);
    Q_INVOKABLE void setTargetLanguageByIndex(int index);
    Q_INVOKABLE QString languageDisplayName(const QString& code) const;
    Q_INVOKABLE bool canSwapLanguages() const;

signals:
    void sourceTextChanged();
    void translatedTextChanged();
    void sourceLanguageChanged();
    void targetLanguageChanged();
    void resultSourceLanguageChanged();
    void translatingChanged();
    void resultStaleChanged();
    void errorMessageChanged();
    void statusChanged();
    void errorOccurred(const QString& errorMessage);

private slots:
    void onTranslatingChanged(bool translating);
    void onTranslationFinished(const iqtools::core::TranslationResult& result);
    void onTranslationFailed(const QString& errorMessage);
    void onTranslationCanceled();

private:
    void setResultStale(bool stale);
    void setErrorMessage(const QString& errorMessage);
    void setStatus(const QString& level, const QString& message);
    void copyTextToClipboard(const QString& text) const;
    int indexOfLanguage(const QVariantList& languages, const QString& code) const;
    QString languageCodeAt(const QVariantList& languages, int index) const;
    static QVariantList buildLanguageList(bool includeAuto);
    static QString normalizeLanguageCode(const QString& code);
    void markResultStale();

private:
    iqtools::core::TranslationService* m_translationService{nullptr};
    iqtools::core::SettingsManager* m_settingsManager{nullptr};
    QVariantList m_sourceLanguages;
    QVariantList m_targetLanguages;
    QString m_sourceText;
    QString m_translatedText;
    QString m_sourceLanguage{QStringLiteral("auto")};
    QString m_targetLanguage{QStringLiteral("zh-CN")};
    QString m_resultSourceLanguage;
    QString m_lastErrorMessage;
    QString m_statusMessage;
    QString m_statusLevel{QStringLiteral("info")};
    bool m_isTranslating{false};
    bool m_resultStale{false};
};

}  // namespace iqtools::app::bridge

#endif  // IQTOOLS_TRANSLATION_CONTROLLER_H
