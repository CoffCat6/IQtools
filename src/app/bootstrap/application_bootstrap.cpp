#include "application_bootstrap.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "app/shell/main_window.h"
#include "core/appcontext/app_context.h"
#include "core/services/log_service.h"
#include "infra/logging/logger.h"

namespace iqtools::app {

ApplicationBootstrap::ApplicationBootstrap() = default;

ApplicationBootstrap::~ApplicationBootstrap()
{
    iqtools::infra::logging::Logger::shutdown();
}

void ApplicationBootstrap::initialize()
{
    initLogging();
    initCore();
    initUi();
}

void ApplicationBootstrap::initLogging()
{
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString logDir = QDir(appDataDir).filePath(QStringLiteral("logs"));

    iqtools::infra::logging::LoggingConfig config;
    config.logDirectory = logDir;
    config.enableFileOutput = true;
    config.enableConsoleOutput = true;

    iqtools::infra::logging::Logger::initialize(config);

    // Direct infra call here to avoid circular dependency at bootstrap stage
    iqtools::infra::logging::Logger::info(
        QStringLiteral("app.bootstrap"),
        QStringLiteral("IQtools starting up"));
}

void ApplicationBootstrap::initCore()
{
    m_appContext = std::make_unique<iqtools::core::AppContext>();
    iqtools::infra::logging::Logger::info(
        QStringLiteral("app.bootstrap"),
        QStringLiteral("Core initialized"));
}

void ApplicationBootstrap::initUi()
{
    m_mainWindow = std::make_unique<MainWindow>(m_appContext.get());
    m_mainWindow->show();
    iqtools::infra::logging::Logger::info(
        QStringLiteral("app.bootstrap"),
        QStringLiteral("UI initialized"));
}

}  // namespace iqtools::app
