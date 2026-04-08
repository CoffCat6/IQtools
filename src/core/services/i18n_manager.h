#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTranslator>

#include <memory>

namespace iqtools::core {

/// Internationalization manager.
/// Handles loading and switching application translations at runtime.
class I18nManager : public QObject {
    Q_OBJECT

public:
    explicit I18nManager(QObject* parent = nullptr);
    ~I18nManager() override;

    /// Initialize with the given language code.
    void initialize(const QString& languageCode);

    /// Switch to a new language. Returns true on success.
    bool setLanguage(const QString& languageCode);

    /// Get the current language code.
    QString currentLanguage() const;

    /// Get available language codes.
    static QStringList availableLanguages();

    /// Get display name for a language code.
    static QString languageDisplayName(const QString& languageCode);

signals:
    /// Emitted when language changes successfully.
    void languageChanged(const QString& languageCode);

    /// Emitted to request QML engine to retranslate.
    void retranslateRequested();

private:
    bool loadTranslation(const QString& languageCode);
    void unloadCurrentTranslation();

private:
    std::unique_ptr<QTranslator> m_translator;
    std::unique_ptr<QTranslator> m_qtTranslator;
    QString m_currentLanguage;
};

}  // namespace iqtools::core
