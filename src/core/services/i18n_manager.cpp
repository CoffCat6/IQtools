#include "i18n_manager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>

#include "infra/logging/logger.h"

namespace iqtools::core {

I18nManager::I18nManager(QObject* parent)
    : QObject(parent)
    , m_translator(std::make_unique<QTranslator>())
    , m_qtTranslator(std::make_unique<QTranslator>())
{
}

I18nManager::~I18nManager() {
    unloadCurrentTranslation();
}

void I18nManager::initialize(const QString& languageCode) {
    if (!loadTranslation(languageCode)) {
        // Fallback to Chinese if requested language fails
        if (languageCode != QStringLiteral("zh_CN")) {
            loadTranslation(QStringLiteral("zh_CN"));
        }
    }
}

bool I18nManager::setLanguage(const QString& languageCode) {
    if (languageCode == m_currentLanguage) {
        return true;
    }

    if (!loadTranslation(languageCode)) {
        iqtools::infra::logging::Logger::warning(
            QStringLiteral("i18n"),
            QStringLiteral("Failed to load translation for: %1").arg(languageCode));
        return false;
    }

    emit languageChanged(m_currentLanguage);
    emit retranslateRequested();

    iqtools::infra::logging::Logger::info(
        QStringLiteral("i18n"),
        QStringLiteral("Language switched to: %1").arg(languageCode));

    return true;
}

QString I18nManager::currentLanguage() const {
    return m_currentLanguage;
}

QStringList I18nManager::availableLanguages() {
    return {
        QStringLiteral("zh_CN"),
        QStringLiteral("en_US"),
    };
}

QString I18nManager::languageDisplayName(const QString& languageCode) {
    static const QHash<QString, QString> names = {
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
        {QStringLiteral("en_US"), QStringLiteral("English")},
    };
    return names.value(languageCode, languageCode);
}

bool I18nManager::loadTranslation(const QString& languageCode) {
    unloadCurrentTranslation();

    auto* app = QCoreApplication::instance();
    if (app == nullptr) {
        return false;
    }

    // Load Qt's own translations
    const QString qtTransPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (m_qtTranslator->load(QStringLiteral("qt_%1").arg(languageCode), qtTransPath)) {
        app->installTranslator(m_qtTranslator.get());
    }

    // Load application translations from resources
    const QString transFile = QStringLiteral(":/translations/iqtools_%1").arg(languageCode);
    if (m_translator->load(transFile)) {
        app->installTranslator(m_translator.get());
        m_currentLanguage = languageCode;
        return true;
    }

    // Try loading from file system (for development)
    const QString devPath = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
    const QString devFile = QStringLiteral("iqtools_%1").arg(languageCode);
    if (m_translator->load(devFile, devPath)) {
        app->installTranslator(m_translator.get());
        m_currentLanguage = languageCode;
        return true;
    }

    // For zh_CN, we can proceed without a .qm file since the UI is already in Chinese
    if (languageCode == QStringLiteral("zh_CN")) {
        m_currentLanguage = languageCode;
        return true;
    }

    return false;
}

void I18nManager::unloadCurrentTranslation() {
    auto* app = QCoreApplication::instance();
    if (app == nullptr) {
        return;
    }

    if (!m_translator->isEmpty()) {
        app->removeTranslator(m_translator.get());
    }
    if (!m_qtTranslator->isEmpty()) {
        app->removeTranslator(m_qtTranslator.get());
    }
}

}  // namespace iqtools::core
