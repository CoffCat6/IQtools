#pragma once

#ifndef IQTOOLS_LOGGER_H
#define IQTOOLS_LOGGER_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>

#include "log_manager.h"

#include "log_entry.h"

namespace iqtools::infra::logging {

struct LoggingConfig {
    QString logDirectory;
    QString logFileName {QStringLiteral("iqtools.log")};
    bool enableConsoleOutput {true};
    bool enableFileOutput {true};
    LogLevel minimumLevel {LogLevel::Debug};
    bool forwardToPreviousHandler {false};
};

class Logger {
public:
    static void initialize(const LoggingConfig& config);
    static void shutdown();
    static LoggingConfig config();
    static bool applyConfig(const LoggingConfig& config);
    static bool isLogDirectoryWritable(const QString& directoryPath);

    static void log(LogLevel level,
                    const QString& category,
                    const QString& message,
                    const char* file = __FILE__,
                    int line = __LINE__,
                    const char* function = Q_FUNC_INFO);

    static void debug(const QString& category,
                      const QString& message,
                      const char* file = __FILE__,
                      int line = __LINE__,
                      const char* function = Q_FUNC_INFO);
    static void info(const QString& category,
                     const QString& message,
                     const char* file = __FILE__,
                     int line = __LINE__,
                     const char* function = Q_FUNC_INFO);
    static void warning(const QString& category,
                        const QString& message,
                        const char* file = __FILE__,
                        int line = __LINE__,
                        const char* function = Q_FUNC_INFO);
    static void error(const QString& category,
                      const QString& message,
                      const char* file = __FILE__,
                      int line = __LINE__,
                      const char* function = Q_FUNC_INFO);
    static void critical(const QString& category,
                         const QString& message,
                         const char* file = __FILE__,
                         int line = __LINE__,
                         const char* function = Q_FUNC_INFO);

private:
    static LogSeverity toSeverity(LogLevel level);

private:
    static LoggingConfig s_config;
};

}  // namespace iqtools::infra::logging

#endif  // IQTOOLS_LOGGER_H
