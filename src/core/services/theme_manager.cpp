#include "theme_manager.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>

#include "core/services/log_service.h"

namespace iqtools::core {

bool ThemeManager::applyTheme(QApplication* application, AppTheme theme)
{
    if (theme == AppTheme::Custom) {
        LogService::warning(QStringLiteral("core.theme"),
                            QStringLiteral("Custom theme requires explicit file path"));
        return false;
    }

    return applyStylesheetFile(application, themeFilePath(theme));
}

bool ThemeManager::applyCustomTheme(QApplication* application, const QString& qssFilePath)
{
    return applyStylesheetFile(application, qssFilePath);
}

QString ThemeManager::themeName(AppTheme theme)
{
    switch (theme) {
    case AppTheme::Light:
        return QStringLiteral("light");
    case AppTheme::Dark:
        return QStringLiteral("dark");
    case AppTheme::Custom:
        return QStringLiteral("custom");
    default:
        return QStringLiteral("unknown");
    }
}

QString ThemeManager::themeFilePath(AppTheme theme)
{
    switch (theme) {
    case AppTheme::Light:
        return QStringLiteral("src/resources/qss/light.qss");
    case AppTheme::Dark:
        return QStringLiteral("src/resources/qss/dark.qss");
    case AppTheme::Custom:
    default:
        return QString();
    }
}

bool ThemeManager::applyStylesheetFile(QApplication* application, const QString& filePath)
{
    if (application == nullptr) {
        LogService::error(QStringLiteral("core.theme"), QStringLiteral("QApplication is null"));
        return false;
    }

    if (filePath.isEmpty()) {
        LogService::error(QStringLiteral("core.theme"), QStringLiteral("Theme file path is empty"));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LogService::error(QStringLiteral("core.theme"),
                          QStringLiteral("Failed to open theme file: %1").arg(filePath));
        return false;
    }

    QTextStream stream(&file);
    const QString styleSheet = stream.readAll();
    file.close();

    application->setStyleSheet(styleSheet);
    LogService::info(QStringLiteral("core.theme"),
                     QStringLiteral("Applied theme file: %1").arg(filePath));
    return true;
}

}  // namespace iqtools::core
