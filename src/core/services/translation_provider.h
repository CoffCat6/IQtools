#pragma once

#ifndef IQTOOLS_TRANSLATION_PROVIDER_H
#define IQTOOLS_TRANSLATION_PROVIDER_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include "core/services/translation_types.h"
#include "core/settings/settings_manager.h"

namespace iqtools::core {

struct TranslationRequestContext {
    QString text;
    QString sourceLanguage;
    QString targetLanguage;
};

struct TranslationPreparedRequest {
    QNetworkRequest networkRequest;
    QNetworkAccessManager::Operation operation{
        QNetworkAccessManager::GetOperation};
    QByteArray body;
};

class ITranslationProvider {
public:
    virtual ~ITranslationProvider() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString validateSettings(
        const TranslationProviderSettings& settings) const = 0;
    virtual bool prepareRequest(
        const TranslationProviderSettings& settings,
        const TranslationRequestContext& requestContext,
        TranslationPreparedRequest* preparedRequest,
        QString* errorMessage) const = 0;
    virtual bool parseResponse(
        const TranslationProviderSettings& settings,
        const TranslationRequestContext& requestContext,
        const QByteArray& responseData, TranslationResult* result,
        QString* errorMessage) const = 0;
};

QString normalizeTranslationProviderType(const QString& providerType);
QString translationProviderDisplayName(const QString& providerType);
QStringList translationProviderTypes();
QStringList translationProviderMethodOptions();
QString validateTranslationProviderSettings(
    const TranslationProviderSettings& settings);
const ITranslationProvider* translationProviderForType(
    const QString& providerType);

}  // namespace iqtools::core

#endif  // IQTOOLS_TRANSLATION_PROVIDER_H
