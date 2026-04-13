#pragma once

#ifndef IQTOOLS_TRANSLATION_SERVICE_H
#define IQTOOLS_TRANSLATION_SERVICE_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "core/services/translation_provider.h"

namespace iqtools::core {

class TranslationService : public QObject {
    Q_OBJECT

public:
    explicit TranslationService(iqtools::core::SettingsManager* settingsManager,
                                QObject* parent = nullptr);
    ~TranslationService() override;

    void translate(const QString& text, const QString& sourceLang,
                   const QString& targetLang);
    void cancel();

    static QList<LanguageInfo> supportedLanguages();
    static QString detectLanguage(const QString& text);

    bool isTranslating() const;
    TranslationResult lastResult() const;

signals:
    void translationFinished(const iqtools::core::TranslationResult& result);
    void translationFailed(const QString& errorMessage);
    void translatingChanged(bool translating);
    void translationCanceled();

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    void parseResponse(const QByteArray& data);
    static QString normalizeLanguageCode(const QString& languageCode);

private:
    iqtools::core::SettingsManager* m_settingsManager{nullptr};
    QNetworkAccessManager* m_networkManager{nullptr};
    QPointer<QNetworkReply> m_activeReply;
    bool m_isTranslating{false};
    TranslationResult m_lastResult;
    QString m_activeSourceLanguage;
    QString m_activeTargetLanguage;
    TranslationProviderSettings m_activeProviderSettings;
    TranslationRequestContext m_activeRequestContext;
    const ITranslationProvider* m_activeProvider{nullptr};
};

}  // namespace iqtools::core

#endif  // IQTOOLS_TRANSLATION_SERVICE_H
