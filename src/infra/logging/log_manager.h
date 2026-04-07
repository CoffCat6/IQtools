#pragma once

#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QMessageLogContext>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace iqtools::infra::logging {

enum class LogSeverity {
    Debug = 0,
    Info,
    Warning,
    Critical,
    Fatal
};

struct LogManagerConfig {
    // Empty means: QStandardPaths::AppDataLocation + "/logs"
    QString logDirectory;
    QString filePrefix {QStringLiteral("iqtools")};
    bool enableConsoleOutput {true};
    bool enableFileOutput {true};
    LogSeverity minimumLevel {LogSeverity::Debug};
    bool forwardToPreviousHandler {false};
};

class LogManager {
public:
    static LogManager& instance();

    void initialize(const LogManagerConfig& config = {});
    void shutdown();
    bool isInitialized() const;
    LogManagerConfig config() const;
    void updateConfig(const LogManagerConfig& config);
    static bool isPathWritable(const QString& directoryPath);

    static void qtMessageHandler(QtMsgType type,
                                 const QMessageLogContext& context,
                                 const QString& message);

    void logMessage(LogSeverity level,
                    const char* category,
                    const QString& message,
                    const char* file,
                    int line,
                    const char* function);

private:
    LogManager() = default;
    ~LogManager() = default;
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    void logInternal(QtMsgType type,
                     const QMessageLogContext& context,
                     const QString& message);

    void openLogFileForDateLocked(const QDate& date);
    QString resolveLogDirectory() const;
    QString buildLogFileName(const QDate& date) const;

    QString formatLine(QtMsgType type,
                       const QMessageLogContext& context,
                       const QString& message,
                       const QDateTime& now) const;

    static LogSeverity toSeverity(QtMsgType type);
    static QtMsgType toQtType(LogSeverity level);
    static const char* severityToText(LogSeverity level);

    bool shouldLog(QtMsgType type) const;

private:
    mutable QMutex m_mutex;
    LogManagerConfig m_config;
    QFile m_logFile;
    QDate m_openedDate;
    bool m_initialized {false};
    QtMessageHandler m_previousHandler {nullptr};
};

}  // namespace iqtools::infra::logging
