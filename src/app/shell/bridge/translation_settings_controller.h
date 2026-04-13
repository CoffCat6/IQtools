#pragma once

#ifndef IQTOOLS_TRANSLATION_SETTINGS_CONTROLLER_H
#define IQTOOLS_TRANSLATION_SETTINGS_CONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include "core/settings/settings_manager.h"

namespace iqtools::core {
class SettingsManager;
}

namespace iqtools::app::bridge {

class TranslationSettingsController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString providerType READ providerType WRITE setProviderType NOTIFY settingsChanged)
    Q_PROPERTY(QString customEndpoint READ customEndpoint WRITE setCustomEndpoint NOTIFY settingsChanged)
    Q_PROPERTY(QString customMethod READ customMethod WRITE setCustomMethod NOTIFY settingsChanged)
    Q_PROPERTY(QString customHeadersJson READ customHeadersJson WRITE setCustomHeadersJson NOTIFY settingsChanged)
    Q_PROPERTY(QString customQueryTemplate READ customQueryTemplate WRITE setCustomQueryTemplate NOTIFY settingsChanged)
    Q_PROPERTY(QString customBodyTemplate READ customBodyTemplate WRITE setCustomBodyTemplate NOTIFY settingsChanged)
    Q_PROPERTY(QString customResultTextPath READ customResultTextPath WRITE setCustomResultTextPath NOTIFY settingsChanged)
    Q_PROPERTY(QString customDetectedSourcePath READ customDetectedSourcePath WRITE setCustomDetectedSourcePath NOTIFY settingsChanged)
    Q_PROPERTY(QString customApiKey READ customApiKey WRITE setCustomApiKey NOTIFY settingsChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY settingsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit TranslationSettingsController(
        iqtools::core::SettingsManager* settingsManager,
        QObject* parent = nullptr);

    QString providerType() const;
    QString customEndpoint() const;
    QString customMethod() const;
    QString customHeadersJson() const;
    QString customQueryTemplate() const;
    QString customBodyTemplate() const;
    QString customResultTextPath() const;
    QString customDetectedSourcePath() const;
    QString customApiKey() const;
    bool dirty() const;
    QString statusMessage() const;

    void setProviderType(const QString& providerType);
    void setCustomEndpoint(const QString& endpoint);
    void setCustomMethod(const QString& method);
    void setCustomHeadersJson(const QString& headersJson);
    void setCustomQueryTemplate(const QString& queryTemplate);
    void setCustomBodyTemplate(const QString& bodyTemplate);
    void setCustomResultTextPath(const QString& path);
    void setCustomDetectedSourcePath(const QString& path);
    void setCustomApiKey(const QString& apiKey);

    Q_INVOKABLE bool apply();
    Q_INVOKABLE void resetPending();

signals:
    void settingsChanged();
    void statusMessageChanged();

private:
    void clearStatusMessage();
    void setPendingSettings(
        const iqtools::core::TranslationProviderSettings& settings);
    void refreshFromManager();

private:
    iqtools::core::SettingsManager* m_settingsManager{nullptr};
    iqtools::core::TranslationProviderSettings m_appliedSettings;
    iqtools::core::TranslationProviderSettings m_pendingSettings;
    QString m_statusMessage;
};

}  // namespace iqtools::app::bridge

#endif  // IQTOOLS_TRANSLATION_SETTINGS_CONTROLLER_H
