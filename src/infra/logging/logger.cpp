#include "logger.h"

#include <cstdlib>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QIODevice>
#include <QtCore/QMutexLocker>
#include <QtCore/QTextStream>

namespace iqtools::infra::logging {

LoggingConfig Logger::s_config {};
QFile Logger::s_logFile {};
QMutex Logger::s_mutex {};
bool Logger::s_initialized = false;
QtMessageHandler Logger::s_previousHandler = nullptr;

void Logger::initialize(const LoggingConfig& config)
{
    QMutexLocker locker(&s_mutex);

    if (s_initialized) {
        return;
    }

    s_config = config;

    if (s_config.enableFileOutput && !s_config.logDirectory.isEmpty()) {
        QDir().mkpath(s_config.logDirectory);
        s_logFile.setFileName(QDir(s_config.logDirectory).filePath(s_config.logFileName));
        s_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }

    installQtMessageHandler();
    s_initialized = true;
    locker.unlock();

    info(QStringLiteral("infra.logging"), QStringLiteral("Logger initialized"));
}

void Logger::shutdown()
{
    QMutexLocker locker(&s_mutex);

    if (!s_initialized) {
        return;
    }

    if (s_logFile.isOpen()) {
        QTextStream stream(&s_logFile);
        stream << "Logger shutdown" << Qt::endl;
        s_logFile.flush();
        s_logFile.close();
    }

    uninstallQtMessageHandler();
    s_initialized = false;
}

void Logger::debug(const QString& category, const QString& message)
{
    write(LogLevel::Debug, category, message);
}

void Logger::info(const QString& category, const QString& message)
{
    write(LogLevel::Info, category, message);
}

void Logger::warning(const QString& category, const QString& message)
{
    write(LogLevel::Warning, category, message);
}

void Logger::error(const QString& category, const QString& message)
{
    write(LogLevel::Error, category, message);
}

void Logger::write(LogLevel level, const QString& category, const QString& message)
{
    if (level < s_config.minimumLevel) {
        return;
    }

    const LogEntry entry {
        QDateTime::currentDateTime(),
        level,
        category,
        message,
    };

    const QString line = format(entry);

    QMutexLocker locker(&s_mutex);

    if (s_config.enableConsoleOutput) {
        QTextStream console(stderr);
        console << line << Qt::endl;
    }

    if (s_config.enableFileOutput && s_logFile.isOpen()) {
        QTextStream fileStream(&s_logFile);
        fileStream << line << Qt::endl;
        s_logFile.flush();
    }
}

QString Logger::format(const LogEntry& entry)
{
    return QStringLiteral("[%1] [%2] [%3] %4")
        .arg(entry.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")))
        .arg(QString::fromUtf8(levelToString(entry.level)))
        .arg(entry.category)
        .arg(entry.message);
}

const char* Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void Logger::installQtMessageHandler()
{
    s_previousHandler = qInstallMessageHandler(&Logger::qtMessageHandler);
}

void Logger::uninstallQtMessageHandler()
{
    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;
}

void Logger::qtMessageHandler(QtMsgType type,
                              const QMessageLogContext& context,
                              const QString& message)
{
    QString category = context.category ? QString::fromUtf8(context.category)
                                        : QStringLiteral("qt");

    switch (type) {
    case QtDebugMsg:
        write(LogLevel::Debug, category, message);
        break;
    case QtInfoMsg:
        write(LogLevel::Info, category, message);
        break;
    case QtWarningMsg:
        write(LogLevel::Warning, category, message);
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        write(LogLevel::Error, category, message);
        break;
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

}  // namespace iqtools::infra::logging
