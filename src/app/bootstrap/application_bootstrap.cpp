#include "application_bootstrap.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtWidgets/QApplication>

#include "app/shell/bridge/app_facade.h"
#include "app/shell/bridge/log_console_controller.h"
#include "app/shell/bridge/logging_settings_controller.h"
#include "app/shell/bridge/navigation_controller.h"
#include "app/shell/bridge/settings_controller.h"
#include "app/shell/bridge/theme_controller.h"
#include "app/shell/bridge/tool_list_model.h"
#include "app/shell/bridge/update_controller.h"
#include "app/shell/bridge/window_controller.h"
#include "app/shell/main_window.h"
#include "core/appcontext/app_context.h"
#include "core/services/i18n_manager.h"
#include "core/services/log_service.h"
#include "core/services/theme_manager.h"
#include "core/services/update_checker.h"
#include "core/settings/settings_manager.h"
#include "infra/logging/logger.h"

namespace iqtools::app {

ApplicationBootstrap::ApplicationBootstrap() = default;

ApplicationBootstrap::~ApplicationBootstrap() {
  iqtools::infra::logging::Logger::shutdown();
}

void ApplicationBootstrap::initialize() {
  initLogging();
  initCore();
  initPresentation();
}

void ApplicationBootstrap::initLogging() {
  const QString appDataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  const QString logDir = QDir(appDataDir).filePath(QStringLiteral("logs"));

  iqtools::infra::logging::LoggingConfig config;
  config.logDirectory = logDir;
  config.enableFileOutput = true;
  config.enableConsoleOutput = true;

  iqtools::infra::logging::Logger::initialize(config);
  qSetMessagePattern(QStringLiteral(
      "[%{time yyyy-MM-dd HH:mm:ss.zzz}] [%{type}] [%{category}] %{message}"));

  // Direct infra call here to avoid circular dependency at bootstrap stage
  iqtools::infra::logging::Logger::info(QStringLiteral("app.bootstrap"),
                                        QStringLiteral("IQtools starting up"));
}

void ApplicationBootstrap::initCore() {
  m_appContext = std::make_unique<iqtools::core::AppContext>();

  // Initialize settings manager and load from setting.yaml
  m_settingsManager = std::make_unique<iqtools::core::SettingsManager>();
  m_settingsManager->load();

  // Initialize i18n manager with saved language preference
  m_i18nManager = std::make_unique<iqtools::core::I18nManager>();
  m_i18nManager->initialize(m_settingsManager->language());

  // Initialize update checker
  m_updateChecker = std::make_unique<iqtools::core::UpdateChecker>();

  iqtools::infra::logging::Logger::info(
      QStringLiteral("app.bootstrap"),
      QStringLiteral("Core initialized, settings loaded from: %1")
          .arg(m_settingsManager->settingsFilePath()));
}

void ApplicationBootstrap::initPresentation() {
  auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
  if (app != nullptr) {
    app->setStyleSheet(QString());
  }

  m_toolListModel = std::make_unique<iqtools::app::bridge::ToolListModel>(
      m_appContext->toolRegistry());
  m_themeController = std::make_unique<iqtools::app::bridge::ThemeController>();
  m_navigationController =
      std::make_unique<iqtools::app::bridge::NavigationController>();
  m_loggingSettingsController =
      std::make_unique<iqtools::app::bridge::LoggingSettingsController>(
          m_settingsManager.get());
  m_logConsoleController =
      std::make_unique<iqtools::app::bridge::LogConsoleController>();
  m_settingsController =
      std::make_unique<iqtools::app::bridge::SettingsController>(
          m_settingsManager.get(), m_themeController.get());
  m_windowController = std::make_unique<iqtools::app::bridge::WindowController>(
      m_settingsManager.get());
  m_updateController = std::make_unique<iqtools::app::bridge::UpdateController>(
      m_updateChecker.get());
  m_appFacade = std::make_unique<iqtools::app::bridge::AppFacade>(
      m_themeController.get(), m_navigationController.get(),
      m_toolListModel.get(), m_loggingSettingsController.get());

  // Connect language change signal to i18n manager
  QObject::connect(m_settingsController.get(),
                   &iqtools::app::bridge::SettingsController::languageRequested,
                   m_i18nManager.get(), &iqtools::core::I18nManager::setLanguage);

  m_qmlEngine = std::make_unique<QQmlApplicationEngine>();

  // Connect retranslate signal to QML engine
  QObject::connect(m_i18nManager.get(),
                   &iqtools::core::I18nManager::retranslateRequested,
                   m_qmlEngine.get(), &QQmlApplicationEngine::retranslate);

  auto *rootContext = m_qmlEngine->rootContext();
  rootContext->setContextProperty(QStringLiteral("appFacade"),
                                  m_appFacade.get());
  rootContext->setContextProperty(QStringLiteral("themeController"),
                                  m_themeController.get());
  rootContext->setContextProperty(QStringLiteral("navigationController"),
                                  m_navigationController.get());
  rootContext->setContextProperty(QStringLiteral("toolListModel"),
                                  m_toolListModel.get());
  rootContext->setContextProperty(QStringLiteral("loggingSettings"),
                                  m_loggingSettingsController.get());
  rootContext->setContextProperty(QStringLiteral("logConsole"),
                                  m_logConsoleController.get());
  rootContext->setContextProperty(QStringLiteral("settingsCtrl"),
                                  m_settingsController.get());
  rootContext->setContextProperty(QStringLiteral("windowController"),
                                  m_windowController.get());
  rootContext->setContextProperty(QStringLiteral("updateController"),
                                  m_updateController.get());

  const QUrl mainQmlUrl(
      QStringLiteral("qrc:/IQtools/src/app/shell/qml/main.qml"));
  QObject::connect(
      m_qmlEngine.get(), &QQmlApplicationEngine::objectCreated,
      QCoreApplication::instance(),
      [mainQmlUrl](QObject *object, const QUrl &objectUrl) {
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

  // Auto-check for updates if enabled
  if (m_settingsManager->checkUpdateEnabled()) {
    m_updateChecker->checkForUpdates();
  }

  iqtools::infra::logging::Logger::info(
      QStringLiteral("app.bootstrap"),
      QStringLiteral("QML presentation initialized"));
}

} // namespace iqtools::app
