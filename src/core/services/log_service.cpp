#include "log_service.h"

#include "infra/logging/logger.h"

namespace iqtools::core {

void LogService::debug(const QString& category, const QString& message)
{
    infra::logging::Logger::debug(category, message);
}

void LogService::info(const QString& category, const QString& message)
{
    infra::logging::Logger::info(category, message);
}

void LogService::warning(const QString& category, const QString& message)
{
    infra::logging::Logger::warning(category, message);
}

void LogService::error(const QString& category, const QString& message)
{
    infra::logging::Logger::error(category, message);
}

}  // namespace iqtools::core
