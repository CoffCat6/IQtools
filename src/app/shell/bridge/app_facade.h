#pragma once

#include <QtCore/QObject>

namespace iqtools::app::bridge {

class NavigationController;
class ThemeController;
class ToolListModel;
class LoggingSettingsController;

class AppFacade : public QObject {
    Q_OBJECT

public:
    AppFacade(ThemeController* theme,
              NavigationController* navigation,
              ToolListModel* tools,
              LoggingSettingsController* loggingSettings,
              QObject* parent = nullptr);

    Q_INVOKABLE void logInfo(const QString& category, const QString& message) const;
    Q_INVOKABLE QObject* loggingSettings() const;

private:
    ThemeController* m_theme {nullptr};
    NavigationController* m_navigation {nullptr};
    ToolListModel* m_tools {nullptr};
    QObject* m_loggingSettings {nullptr};
};

}  // namespace iqtools::app::bridge
