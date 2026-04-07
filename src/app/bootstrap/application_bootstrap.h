#pragma once

#include <memory>

namespace iqtools::core {
class AppContext;
class LogService;
}

class QQmlApplicationEngine;

namespace iqtools::app::bridge {
class ToolListModel;
class ThemeController;
class NavigationController;
class LoggingSettingsController;
class LogConsoleController;
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
    std::unique_ptr<iqtools::app::bridge::AppFacade> m_appFacade;
};

}  // namespace iqtools::app
