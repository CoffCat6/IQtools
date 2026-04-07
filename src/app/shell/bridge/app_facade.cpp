#include "app_facade.h"

#include "core/services/log_service.h"
#include "logging_settings_controller.h"

namespace iqtools::app::bridge {

AppFacade::AppFacade(ThemeController* theme,
                     NavigationController* navigation,
                     ToolListModel* tools,
                     LoggingSettingsController* loggingSettings,
                     QObject* parent)
    : QObject(parent)
    , m_theme(theme)
    , m_navigation(navigation)
    , m_tools(tools)
    , m_loggingSettings(loggingSettings)
{
}

void AppFacade::logInfo(const QString& category, const QString& message) const
{
    iqtools::core::LogService::info(category, message);
}

QObject* AppFacade::loggingSettings() const
{
    return m_loggingSettings;
}

}  // namespace iqtools::app::bridge
