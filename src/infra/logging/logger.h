#pragma once

#ifndef IQTOOLS_LOGGER_H
#define IQTOOLS_LOGGER_H

#include <QtCore/QFile>
#include <QtCore/QMessageLogContext>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

class QDateTime;

#include "log_entry.h"

namespace iqtools::infra::logging {

struct LoggingConfig {
    QString logDirectory;
    QString logFileName {"iqtools.log"};
    bool enableConsoleOutput {true};
    bool enableFileOutput {true};
    LogLevel minimumLevel {LogLevel::Debug};
};

class Logger {
public:
    static void initialize(const LoggingConfig& config);
    static void shutdown();

    static void debug(const QString& category, const QString& message);
    static void info(const QString& category, const QString& message);
    static void warning(const QString& category, const QString& message);
    static void error(const QString& category, const QString& message);

private:
    static void write(LogLevel level, const QString& category, const QString& message);
    static QString format(const LogEntry& entry);
    static const char* levelToString(LogLevel level);
    static void installQtMessageHandler();
    static void uninstallQtMessageHandler();
    static void qtMessageHandler(QtMsgType type,
                                 const QMessageLogContext& context,
                                 const QString& message);

private:
    static LoggingConfig s_config;
    static QFile s_logFile;
    static QMutex s_mutex;
    static bool s_initialized;
    static QtMessageHandler s_previousHandler;
};

}  // namespace iqtools::infra::logging

#endif  // IQTOOLS_LOGGER_H
