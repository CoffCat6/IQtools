#include "log_service.h"

#include <QtCore/QLoggingCategory>

#include "infra/logging/logger.h"

namespace iqtools::core {

void LogService::log(iqtools::infra::logging::LogLevel level,
                     const QString& category,
                     const QString& message,
                     const char* file,
                     int line,
                     const char* function)
{
    infra::logging::Logger::log(level, category, message, file, line, function);
}

void LogService::debug(const QString& category,
                       const QString& message,
                       const char* file,
                       int line,
                       const char* function)
{
    log(iqtools::infra::logging::LogLevel::Debug, category, message, file, line, function);
}

void LogService::info(const QString& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function)
{
    log(iqtools::infra::logging::LogLevel::Info, category, message, file, line, function);
}

void LogService::warning(const QString& category,
                         const QString& message,
                         const char* file,
                         int line,
                         const char* function)
{
    log(iqtools::infra::logging::LogLevel::Warning, category, message, file, line, function);
}

void LogService::error(const QString& category,
                       const QString& message,
                       const char* file,
                       int line,
                       const char* function)
{
    log(iqtools::infra::logging::LogLevel::Error, category, message, file, line, function);
}

void LogService::debug(const QLoggingCategory& category,
                       const QString& message,
                       const char* file,
                       int line,
                       const char* function)
{
    debug(QString::fromUtf8(category.categoryName()), message, file, line, function);
}

void LogService::info(const QLoggingCategory& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function)
{
    info(QString::fromUtf8(category.categoryName()), message, file, line, function);
}

void LogService::warning(const QLoggingCategory& category,
                         const QString& message,
                         const char* file,
                         int line,
                         const char* function)
{
    warning(QString::fromUtf8(category.categoryName()), message, file, line, function);
}

void LogService::error(const QLoggingCategory& category,
                       const QString& message,
                       const char* file,
                       int line,
                       const char* function)
{
    error(QString::fromUtf8(category.categoryName()), message, file, line, function);
}

void LogService::debug(const QString& category, const QString& message)
{
    debug(category, message, __FILE__, __LINE__, Q_FUNC_INFO);
}

void LogService::info(const QString& category, const QString& message)
{
    info(category, message, __FILE__, __LINE__, Q_FUNC_INFO);
}

void LogService::warning(const QString& category, const QString& message)
{
    warning(category, message, __FILE__, __LINE__, Q_FUNC_INFO);
}

void LogService::error(const QString& category, const QString& message)
{
    error(category, message, __FILE__, __LINE__, Q_FUNC_INFO);
}

}  // namespace iqtools::core
