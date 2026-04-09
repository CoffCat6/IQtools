#pragma once

#include <memory>

namespace iqtools::core {
class AppContext;
class LogService;
class SettingsManager;
class ShortcutManager;
class I18nManager;
class UpdateChecker;
}

class QQmlApplicationEngine;

namespace iqtools::app::bridge {
class ToolListModel;
class ThemeController;
class NavigationController;
class LoggingSettingsController;
class LogConsoleController;
class SettingsController;
class WindowController;
class UpdateController;
class AppFacade;
}

namespace iqtools::app {

class MainWindow;

class ApplicationBootstrap {
public:
    ApplicationBootstrap();
    ~ApplicationBootstrap();

    void initialize();

private:
    void initLogging();
    void initCore();
    void initPresentation();

private:
    std::unique_ptr<iqtools::core::AppContext> m_appContext;
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<QQmlApplicationEngine> m_qmlEngine;
    std::unique_ptr<iqtools::app::bridge::ToolListModel> m_toolListModel;
    std::unique_ptr<iqtools::app::bridge::ThemeController> m_themeController;
    std::unique_ptr<iqtools::app::bridge::NavigationController> m_navigationController;
    std::unique_ptr<iqtools::app::bridge::LoggingSettingsController> m_loggingSettingsController;
    std::unique_ptr<iqtools::app::bridge::LogConsoleController> m_logConsoleController;
    std::unique_ptr<iqtools::app::bridge::SettingsController> m_settingsController;
    std::unique_ptr<iqtools::app::bridge::WindowController> m_windowController;
    std::unique_ptr<iqtools::app::bridge::UpdateController> m_updateController;
    std::unique_ptr<iqtools::core::SettingsManager> m_settingsManager;
    std::unique_ptr<iqtools::core::ShortcutManager> m_shortcutManager;
    std::unique_ptr<iqtools::core::I18nManager> m_i18nManager;
    std::unique_ptr<iqtools::core::UpdateChecker> m_updateChecker;
    std::unique_ptr<iqtools::app::bridge::AppFacade> m_appFacade;
};

}  // namespace iqtools::app
