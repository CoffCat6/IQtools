#include "application_bootstrap.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtWidgets/QApplication>

#include "app/shell/bridge/app_facade.h"
#include "app/shell/bridge/log_console_controller.h"
#include "app/shell/bridge/logging_settings_controller.h"
#include "app/shell/bridge/navigation_controller.h"
#include "app/shell/bridge/theme_controller.h"
#include "app/shell/bridge/tool_list_model.h"
#include "app/shell/main_window.h"
#include "core/appcontext/app_context.h"
#include "core/services/log_service.h"
#include "core/services/theme_manager.h"
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
    initPresentation();
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
    qSetMessagePattern(QStringLiteral("[%{time yyyy-MM-dd HH:mm:ss.zzz}] [%{type}] [%{category}] %{message}"));

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

void ApplicationBootstrap::initPresentation()
{
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app != nullptr) {
        app->setStyleSheet(QString());
    }

    m_toolListModel = std::make_unique<iqtools::app::bridge::ToolListModel>(m_appContext->toolRegistry());
    m_themeController = std::make_unique<iqtools::app::bridge::ThemeController>();
    m_navigationController = std::make_unique<iqtools::app::bridge::NavigationController>();
    m_loggingSettingsController = std::make_unique<iqtools::app::bridge::LoggingSettingsController>();
    m_logConsoleController = std::make_unique<iqtools::app::bridge::LogConsoleController>();
    m_appFacade = std::make_unique<iqtools::app::bridge::AppFacade>(
        m_themeController.get(),
        m_navigationController.get(),
        m_toolListModel.get(),
        m_loggingSettingsController.get());

    m_qmlEngine = std::make_unique<QQmlApplicationEngine>();
    auto* rootContext = m_qmlEngine->rootContext();
    rootContext->setContextProperty(QStringLiteral("appFacade"), m_appFacade.get());
    rootContext->setContextProperty(QStringLiteral("themeController"), m_themeController.get());
    rootContext->setContextProperty(QStringLiteral("navigationController"), m_navigationController.get());
    rootContext->setContextProperty(QStringLiteral("toolListModel"), m_toolListModel.get());
    rootContext->setContextProperty(QStringLiteral("loggingSettings"), m_loggingSettingsController.get());
    rootContext->setContextProperty(QStringLiteral("logConsole"), m_logConsoleController.get());

    const QUrl mainQmlUrl(QStringLiteral("qrc:/IQtools/src/app/shell/qml/main.qml"));
    QObject::connect(m_qmlEngine.get(),
                     &QQmlApplicationEngine::objectCreated,
                     QCoreApplication::instance(),
                     [mainQmlUrl](QObject* object, const QUrl& objectUrl) {
                         if (object == nullptr && objectUrl == mainQmlUrl) {
                             QCoreApplication::exit(-1);
                         }
                     },
                     Qt::QueuedConnection);

    m_qmlEngine->load(mainQmlUrl);
    if (m_qmlEngine->rootObjects().isEmpty()) {
        iqtools::infra::logging::Logger::error(
            QStringLiteral("app.bootstrap"),
            QStringLiteral("Failed to load QML root object"));
        QCoreApplication::exit(-1);
        return;
    }

    iqtools::infra::logging::Logger::info(
        QStringLiteral("app.bootstrap"),
        QStringLiteral("QML presentation initialized"));
}

}  // namespace iqtools::app
