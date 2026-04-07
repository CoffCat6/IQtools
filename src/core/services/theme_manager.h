#pragma once

#include <QtCore/QString>

class QApplication;

namespace iqtools::core {

enum class AppTheme {
    Light = 0,
    Dark,
    Custom
};

class ThemeManager {
public:
    static bool applyTheme(QApplication* application, AppTheme theme);
    static bool applyCustomTheme(QApplication* application, const QString& qssFilePath);

    static QString themeName(AppTheme theme);
    static QString themeFilePath(AppTheme theme);

private:
    static bool applyStylesheetFile(QApplication* application, const QString& filePath);
};

}  // namespace iqtools::core
