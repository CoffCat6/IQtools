#pragma once

#ifndef IQTOOLS_TRANSLATION_SERVICE_H
#define IQTOOLS_TRANSLATION_SERVICE_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace iqtools::core {

/// Language pair for translation.
struct LanguagePair {
    QString source;
    QString target;

    QString toApiString() const;
};

/// Translation result.
struct TranslationResult {
    QString translatedText;
    QString sourceLanguage;
    QString targetLanguage;
    bool success{false};
    QString errorMessage;
};

/// Supported languages info.
struct LanguageInfo {
    QString code;
    QString name;
    QString nativeName;
};

/// Translation service using MyMemory API.
/// Ref: https://api.mymemory.translated.net
class TranslationService : public QObject {
    Q_OBJECT

public:
    explicit TranslationService(QObject* parent = nullptr);
    ~TranslationService() override;

    /// Translate text from source to target language.
    void translate(const QString& text, const QString& sourceLang, const QString& targetLang);

    /// Cancel the in-flight translation request.
    void cancel();

    /// Get supported languages list.
    static QList<LanguageInfo> supportedLanguages();

    /// Detect language of input text (best effort via target lang hint).
    static QString detectLanguage(const QString& text);

    /// Check if currently translating.
    bool isTranslating() const;

    /// Get last translation result.
    TranslationResult lastResult() const;

signals:
    /// Emitted when translation completes.
    void translationFinished(const iqtools::core::TranslationResult& result);

    /// Emitted when translation fails.
    void translationFailed(const QString& errorMessage);

    /// Emitted when translating state changes.
    void translatingChanged(bool translating);

    /// Emitted when the in-flight translation is canceled.
    void translationCanceled();

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    void parseResponse(const QByteArray& data);

private:
    QNetworkAccessManager* m_networkManager{nullptr};
    QPointer<QNetworkReply> m_activeReply;
    bool m_isTranslating{false};
    TranslationResult m_lastResult;
    QString m_activeSourceLanguage;
    QString m_activeTargetLanguage;
};

}  // namespace iqtools::core

#endif  // IQTOOLS_TRANSLATION_SERVICE_H
