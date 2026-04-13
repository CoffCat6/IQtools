#pragma once

#ifndef IQTOOLS_TRANSLATION_TYPES_H
#define IQTOOLS_TRANSLATION_TYPES_H

#include <QtCore/QList>
#include <QtCore/QString>

namespace iqtools::core {

struct LanguagePair {
    QString source;
    QString target;

    QString toApiString() const
    {
        return source + QStringLiteral("|") + target;
    }
};

struct TranslationResult {
    QString translatedText;
    QString sourceLanguage;
    QString targetLanguage;
    bool success{false};
    QString errorMessage;
};

struct LanguageInfo {
    QString code;
    QString name;
    QString nativeName;
};

}  // namespace iqtools::core

Q_DECLARE_METATYPE(iqtools::core::TranslationResult)

#endif  // IQTOOLS_TRANSLATION_TYPES_H
