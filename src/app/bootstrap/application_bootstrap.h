#pragma once

#include <memory>

namespace iqtools::core {
class AppContext;
class LogService;
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
    void initUi();

private:
    std::unique_ptr<iqtools::core::AppContext> m_appContext;
    std::unique_ptr<MainWindow> m_mainWindow;
};

}  // namespace iqtools::app
