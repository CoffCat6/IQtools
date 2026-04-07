#include "logger.h"

namespace iqtools::infra::logging {

LoggingConfig Logger::s_config {};

void Logger::initialize(const LoggingConfig& config)
{
    s_config = config;

    LogManagerConfig managerConfig;
    managerConfig.logDirectory = config.logDirectory;
    managerConfig.enableConsoleOutput = config.enableConsoleOutput;
    managerConfig.enableFileOutput = config.enableFileOutput;
    managerConfig.minimumLevel = toSeverity(config.minimumLevel);
    managerConfig.forwardToPreviousHandler = config.forwardToPreviousHandler;

    const int dotIndex = config.logFileName.lastIndexOf(QLatin1Char('.'));
    if (dotIndex > 0) {
        managerConfig.filePrefix = config.logFileName.left(dotIndex);
    } else if (!config.logFileName.isEmpty()) {
        managerConfig.filePrefix = config.logFileName;
    }

    LogManager::instance().initialize(managerConfig);
}

void Logger::shutdown()
{
    LogManager::instance().shutdown();
}

LoggingConfig Logger::config()
{
    return s_config;
}

bool Logger::applyConfig(const LoggingConfig& config)
{
    if (config.enableFileOutput) {
        const QString directory = config.logDirectory.isEmpty()
                                      ? LogManager::instance().config().logDirectory
                                      : config.logDirectory;
        if (!directory.isEmpty() && !LogManager::isPathWritable(directory)) {
            return false;
        }
    }

    s_config = config;

    LogManagerConfig managerConfig;
    managerConfig.logDirectory = config.logDirectory;
    managerConfig.enableConsoleOutput = config.enableConsoleOutput;
    managerConfig.enableFileOutput = config.enableFileOutput;
    managerConfig.minimumLevel = toSeverity(config.minimumLevel);
    managerConfig.forwardToPreviousHandler = config.forwardToPreviousHandler;

    const int dotIndex = config.logFileName.lastIndexOf(QLatin1Char('.'));
    if (dotIndex > 0) {
        managerConfig.filePrefix = config.logFileName.left(dotIndex);
    } else if (!config.logFileName.isEmpty()) {
        managerConfig.filePrefix = config.logFileName;
    }

    LogManager::instance().updateConfig(managerConfig);
    return true;
}

bool Logger::isLogDirectoryWritable(const QString& directoryPath)
{
    return LogManager::isPathWritable(directoryPath);
}

void Logger::log(LogLevel level,
                 const QString& category,
                 const QString& message,
                 const char* file,
                 int line,
                 const char* function)
{
    const QByteArray utf8Category = category.toUtf8();
    LogManager::instance().logMessage(toSeverity(level),
                                      utf8Category.constData(),
                                      message,
                                      file,
                                      line,
                                      function);
}

void Logger::debug(const QString& category,
                   const QString& message,
                   const char* file,
                   int line,
                   const char* function)
{
    log(LogLevel::Debug, category, message, file, line, function);
}

void Logger::info(const QString& category,
                  const QString& message,
                  const char* file,
                  int line,
                  const char* function)
{
    log(LogLevel::Info, category, message, file, line, function);
}

void Logger::warning(const QString& category,
                     const QString& message,
                     const char* file,
                     int line,
                     const char* function)
{
    log(LogLevel::Warning, category, message, file, line, function);
}

void Logger::error(const QString& category,
                   const QString& message,
                   const char* file,
                   int line,
                   const char* function)
{
    log(LogLevel::Error, category, message, file, line, function);
}

void Logger::critical(const QString& category,
                      const QString& message,
                      const char* file,
                      int line,
                      const char* function)
{
    log(LogLevel::Critical, category, message, file, line, function);
}

LogSeverity Logger::toSeverity(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return LogSeverity::Debug;
    case LogLevel::Info:
        return LogSeverity::Info;
    case LogLevel::Warning:
        return LogSeverity::Warning;
    case LogLevel::Critical:
    case LogLevel::Error:
        return LogSeverity::Critical;
    case LogLevel::Fatal:
        return LogSeverity::Fatal;
    }

    return LogSeverity::Debug;
}

}  // namespace iqtools::infra::logging
