#include "log_manager.h"

#include <cstdlib>
#include <cstdio>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMutexLocker>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryFile>

namespace iqtools::infra::logging {

LogManager& LogManager::instance()
{
    static LogManager manager;
    return manager;
}

void LogManager::initialize(const LogManagerConfig& config)
{
    QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        return;
    }

    m_config = config;

    if (m_config.filePrefix.trimmed().isEmpty()) {
        m_config.filePrefix = QStringLiteral("iqtools");
    }

    if (m_config.enableFileOutput) {
        openLogFileForDateLocked(QDate::currentDate());
    }

    m_previousHandler = qInstallMessageHandler(&LogManager::qtMessageHandler);
    m_initialized = true;
}

void LogManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return;
    }

    qInstallMessageHandler(m_previousHandler);
    m_previousHandler = nullptr;

    if (m_logFile.isOpen()) {
        m_logFile.flush();
        m_logFile.close();
    }

    m_initialized = false;
}

bool LogManager::isInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

LogManagerConfig LogManager::config() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
}

void LogManager::updateConfig(const LogManagerConfig& config)
{
    QMutexLocker locker(&m_mutex);

    if (config.filePrefix.trimmed().isEmpty()) {
        m_config.filePrefix = QStringLiteral("iqtools");
    } else {
        m_config.filePrefix = config.filePrefix;
    }

    m_config.logDirectory = config.logDirectory;
    m_config.enableConsoleOutput = config.enableConsoleOutput;
    m_config.enableFileOutput = config.enableFileOutput;
    m_config.minimumLevel = config.minimumLevel;
    m_config.forwardToPreviousHandler = config.forwardToPreviousHandler;

    if (!m_config.enableFileOutput && m_logFile.isOpen()) {
        m_logFile.flush();
        m_logFile.close();
        return;
    }

    if (m_config.enableFileOutput) {
        openLogFileForDateLocked(QDate::currentDate());
    }
}

void LogManager::qtMessageHandler(QtMsgType type,
                                  const QMessageLogContext& context,
                                  const QString& message)
{
    LogManager::instance().logInternal(type, context, message);
}

void LogManager::logMessage(LogSeverity level,
                            const char* category,
                            const QString& message,
                            const char* file,
                            int line,
                            const char* function)
{
    const char* safeCategory = (category != nullptr && *category != '\0') ? category : "app";
    const QMessageLogContext context(file, line, function, safeCategory);
    logInternal(toQtType(level), context, message);
}

void LogManager::logInternal(QtMsgType type,
                             const QMessageLogContext& context,
                             const QString& message)
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString line = formatLine(type, context, message, now);
    const QByteArray utf8 = line.toUtf8();

    QtMessageHandler previousHandler = nullptr;
    bool shouldForward = false;

    {
        QMutexLocker locker(&m_mutex);

        if (!m_initialized) {
            std::fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stderr);
            std::fwrite("\n", 1, 1, stderr);
            std::fflush(stderr);

            if (type == QtFatalMsg) {
                std::abort();
            }
            return;
        }

        if (!shouldLog(type)) {
            return;
        }

        if (m_config.enableFileOutput) {
            openLogFileForDateLocked(now.date());
        }

        if (m_config.enableConsoleOutput) {
            std::fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stderr);
            std::fwrite("\n", 1, 1, stderr);
            std::fflush(stderr);
        }

        if (m_config.enableFileOutput && m_logFile.isOpen()) {
            m_logFile.write(utf8);
            m_logFile.write("\n");
            m_logFile.flush();
        }

        previousHandler = m_previousHandler;
        shouldForward = m_config.forwardToPreviousHandler;
    }

    if (shouldForward && previousHandler != nullptr) {
        previousHandler(type, context, message);
    }

    if (type == QtFatalMsg) {
        std::abort();
    }
}

void LogManager::openLogFileForDateLocked(const QDate& date)
{
    if (m_openedDate == date && m_logFile.isOpen()) {
        return;
    }

    if (m_logFile.isOpen()) {
        m_logFile.flush();
        m_logFile.close();
    }

    const QString logDir = resolveLogDirectory();
    QDir dir(logDir);
    dir.mkpath(QStringLiteral("."));

    const QString logFilePath = dir.filePath(buildLogFileName(date));
    m_logFile.setFileName(logFilePath);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

    m_openedDate = date;
}

QString LogManager::resolveLogDirectory() const
{
    if (!m_config.logDirectory.isEmpty()) {
        return m_config.logDirectory;
    }

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        appDataPath = QDir::homePath() + QStringLiteral("/.iqtools");
    }

    return QDir(appDataPath).filePath(QStringLiteral("logs"));
}

QString LogManager::buildLogFileName(const QDate& date) const
{
    return QStringLiteral("%1_%2.log")
        .arg(m_config.filePrefix)
        .arg(date.toString(QStringLiteral("yyyy-MM-dd")));
}

QString LogManager::formatLine(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& message,
                               const QDateTime& now) const
{
    const LogSeverity severity = toSeverity(type);
    const QString levelText = QString::fromLatin1(severityToText(severity));

    const QString category = (context.category != nullptr && *context.category != '\0')
                                 ? QString::fromUtf8(context.category)
                                 : QStringLiteral("default");

    QString fileLine = QStringLiteral("unknown:0");
    if (context.file != nullptr && *context.file != '\0') {
        const QString fileName = QFileInfo(QString::fromUtf8(context.file)).fileName();
        fileLine = QStringLiteral("%1:%2").arg(fileName).arg(context.line);
    }

    const QString formattedContext = QStringLiteral("%1|%2").arg(category, fileLine);

    return QStringLiteral("[%1] [%2] [%3] %4")
        .arg(now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
             levelText,
             formattedContext,
             message);
}

LogSeverity LogManager::toSeverity(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return LogSeverity::Debug;
    case QtInfoMsg:
        return LogSeverity::Info;
    case QtWarningMsg:
        return LogSeverity::Warning;
    case QtCriticalMsg:
        return LogSeverity::Critical;
    case QtFatalMsg:
        return LogSeverity::Fatal;
    }
    return LogSeverity::Debug;
}

QtMsgType LogManager::toQtType(LogSeverity level)
{
    switch (level) {
    case LogSeverity::Debug:
        return QtDebugMsg;
    case LogSeverity::Info:
        return QtInfoMsg;
    case LogSeverity::Warning:
        return QtWarningMsg;
    case LogSeverity::Critical:
        return QtCriticalMsg;
    case LogSeverity::Fatal:
        return QtFatalMsg;
    }
    return QtDebugMsg;
}

const char* LogManager::severityToText(LogSeverity level)
{
    switch (level) {
    case LogSeverity::Debug:
        return "DEBUG";
    case LogSeverity::Info:
        return "INFO";
    case LogSeverity::Warning:
        return "WARNING";
    case LogSeverity::Critical:
        return "CRITICAL";
    case LogSeverity::Fatal:
        return "FATAL";
    }
    return "DEBUG";
}

bool LogManager::shouldLog(QtMsgType type) const
{
    return static_cast<int>(toSeverity(type)) >= static_cast<int>(m_config.minimumLevel);
}

bool LogManager::isPathWritable(const QString& directoryPath)
{
    if (directoryPath.trimmed().isEmpty()) {
        return false;
    }

    QDir dir(directoryPath);
    if (!dir.exists() && !QDir().mkpath(directoryPath)) {
        return false;
    }

    QTemporaryFile tmpFile(dir.filePath(QStringLiteral("iqtools_write_test_XXXXXX.tmp")));
    const bool ok = tmpFile.open();
    if (ok) {
        tmpFile.close();
    }
    return ok;
}

}  // namespace iqtools::infra::logging
