#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace iqtools::app::bridge {

class ThemeController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY currentThemeChanged)
    Q_PROPERTY(QVariantMap palette READ palette NOTIFY themeChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY themeChanged)

public:
    explicit ThemeController(QObject* parent = nullptr);

    QString currentTheme() const;
    QVariantMap palette() const;
    bool dark() const;

    Q_INVOKABLE void setTheme(const QString& themeName);
    Q_INVOKABLE void toggleTheme();

signals:
    void currentThemeChanged();
    void themeChanged();

private:
    QVariantMap buildDarkPalette() const;
    QVariantMap buildLightPalette() const;

private:
    QString m_currentTheme;
};

}  // namespace iqtools::app::bridge
