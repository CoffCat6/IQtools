#include "theme_controller.h"

namespace iqtools::app::bridge {

ThemeController::ThemeController(QObject* parent)
    : QObject(parent)
    , m_currentTheme(QStringLiteral("dark"))
{
}

QString ThemeController::currentTheme() const
{
    return m_currentTheme;
}

QVariantMap ThemeController::palette() const
{
    return dark() ? buildDarkPalette() : buildLightPalette();
}

bool ThemeController::dark() const
{
    return m_currentTheme == QStringLiteral("dark");
}

void ThemeController::setTheme(const QString& themeName)
{
    QString normalized = themeName.trimmed().toLower();
    if (normalized != QStringLiteral("dark") && normalized != QStringLiteral("light")) {
        return;
    }

    if (m_currentTheme == normalized) {
        return;
    }

    m_currentTheme = normalized;
    emit currentThemeChanged();
    emit themeChanged();
}

void ThemeController::toggleTheme()
{
    setTheme(dark() ? QStringLiteral("light") : QStringLiteral("dark"));
}

QVariantMap ThemeController::buildDarkPalette() const
{
    return {
        {QStringLiteral("bgApp"), QStringLiteral("#0b1020")},
        {QStringLiteral("bgPanel"), QStringLiteral("#121a2b")},
        {QStringLiteral("bgCard"), QStringLiteral("#162033")},
        {QStringLiteral("bgCardHighlight"), QStringLiteral("#1b2740")},
        {QStringLiteral("textPrimary"), QStringLiteral("#f3f7ff")},
        {QStringLiteral("textSecondary"), QStringLiteral("#b6c2d9")},
        {QStringLiteral("textMuted"), QStringLiteral("#8291ad")},
        {QStringLiteral("borderDefault"), QStringLiteral("#273452")},
        {QStringLiteral("accentPrimary"), QStringLiteral("#7c9cff")},
        {QStringLiteral("accentSecondary"), QStringLiteral("#6ee7d8")},
        {QStringLiteral("shadowColor"), QStringLiteral("#40000000")},
        {QStringLiteral("radiusCard"), 24},
        {QStringLiteral("radiusPanel"), 28},
        {QStringLiteral("spacingSm"), 8},
        {QStringLiteral("spacingMd"), 16},
        {QStringLiteral("spacingLg"), 24},
    };
}

QVariantMap ThemeController::buildLightPalette() const
{
    return {
        {QStringLiteral("bgApp"), QStringLiteral("#f3f6fb")},
        {QStringLiteral("bgPanel"), QStringLiteral("#ffffff")},
        {QStringLiteral("bgCard"), QStringLiteral("#ffffff")},
        {QStringLiteral("bgCardHighlight"), QStringLiteral("#eef4ff")},
        {QStringLiteral("textPrimary"), QStringLiteral("#10203a")},
        {QStringLiteral("textSecondary"), QStringLiteral("#4d6184")},
        {QStringLiteral("textMuted"), QStringLiteral("#7f90ac")},
        {QStringLiteral("borderDefault"), QStringLiteral("#d9e1ef")},
        {QStringLiteral("accentPrimary"), QStringLiteral("#4f6fff")},
        {QStringLiteral("accentSecondary"), QStringLiteral("#1cc6b7")},
        {QStringLiteral("shadowColor"), QStringLiteral("#18000000")},
        {QStringLiteral("radiusCard"), 24},
        {QStringLiteral("radiusPanel"), 28},
        {QStringLiteral("spacingSm"), 8},
        {QStringLiteral("spacingMd"), 16},
        {QStringLiteral("spacingLg"), 24},
    };
}

}  // namespace iqtools::app::bridge
