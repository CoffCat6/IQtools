#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include "infra/logging/log_level.h"

namespace iqtools::core {

class LogService {
public:
    static void log(iqtools::infra::logging::LogLevel level,
                    const QString& category,
                    const QString& message,
                    const char* file,
                    int line,
                    const char* function);

    static void debug(const QString& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function);
    static void info(const QString& category,
                     const QString& message,
                     const char* file,
                     int line,
                     const char* function);
    static void warning(const QString& category,
                        const QString& message,
                        const char* file,
                        int line,
                        const char* function);
    static void error(const QString& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function);

    static void debug(const QLoggingCategory& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function);
    static void info(const QLoggingCategory& category,
                     const QString& message,
                     const char* file,
                     int line,
                     const char* function);
    static void warning(const QLoggingCategory& category,
                        const QString& message,
                        const char* file,
                        int line,
                        const char* function);
    static void error(const QLoggingCategory& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function);

    static void debug(const QString& category, const QString& message);
    static void info(const QString& category, const QString& message);
    static void warning(const QString& category, const QString& message);
    static void error(const QString& category, const QString& message);
};

}  // namespace iqtools::core

#define IQ_LOG_DEBUG(category, message)                                                   \
    ::iqtools::core::LogService::debug((category), (message), __FILE__, __LINE__, Q_FUNC_INFO)

#define IQ_LOG_INFO(category, message)                                                    \
    ::iqtools::core::LogService::info((category), (message), __FILE__, __LINE__, Q_FUNC_INFO)

#define IQ_LOG_WARNING(category, message)                                                 \
    ::iqtools::core::LogService::warning((category), (message), __FILE__, __LINE__, Q_FUNC_INFO)

#define IQ_LOG_ERROR(category, message)                                                   \
    ::iqtools::core::LogService::error((category), (message), __FILE__, __LINE__, Q_FUNC_INFO)
