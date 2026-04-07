#include "logging_settings_controller.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "core/services/log_service.h"

namespace iqtools::app::bridge {

namespace {

QString defaultLogDirectory()
{
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) {
        return QDir::homePath() + QStringLiteral("/.iqtools/logs");
    }
    return QDir(appDataDir).filePath(QStringLiteral("logs"));
}

}  // namespace

LoggingSettingsController::LoggingSettingsController(QObject* parent)
    : QObject(parent)
{
    m_applied = iqtools::infra::logging::Logger::config();
    if (m_applied.logDirectory.isEmpty()) {
        m_applied.logDirectory = defaultLogDirectory();
    }
    m_pending = m_applied;
}

bool LoggingSettingsController::consoleEnabled() const
{
    return m_pending.enableConsoleOutput;
}

bool LoggingSettingsController::fileEnabled() const
{
    return m_pending.enableFileOutput;
}

QString LoggingSettingsController::logDirectory() const
{
    return m_pending.logDirectory;
}

QString LoggingSettingsController::minimumLevel() const
{
    return levelToString(m_pending.minimumLevel);
}

QStringList LoggingSettingsController::levelOptions() const
{
    return {
        QStringLiteral("Debug"),
        QStringLiteral("Info"),
        QStringLiteral("Warning"),
        QStringLiteral("Error"),
        QStringLiteral("Fatal"),
    };
}

bool LoggingSettingsController::dirty() const
{
    return m_pending.logDirectory != m_applied.logDirectory
        || m_pending.enableConsoleOutput != m_applied.enableConsoleOutput
        || m_pending.enableFileOutput != m_applied.enableFileOutput
        || m_pending.minimumLevel != m_applied.minimumLevel;
}

QString LoggingSettingsController::statusMessage() const
{
    return m_statusMessage;
}

void LoggingSettingsController::setConsoleEnabled(bool enabled)
{
    if (m_pending.enableConsoleOutput == enabled) {
        return;
    }
    m_pending.enableConsoleOutput = enabled;
    emit settingsChanged();
}

void LoggingSettingsController::setFileEnabled(bool enabled)
{
    if (m_pending.enableFileOutput == enabled) {
        return;
    }
    m_pending.enableFileOutput = enabled;
    emit settingsChanged();
}

void LoggingSettingsController::setLogDirectory(const QString& directory)
{
    if (m_pending.logDirectory == directory) {
        return;
    }
    m_pending.logDirectory = directory;
    emit settingsChanged();
}

void LoggingSettingsController::setMinimumLevel(const QString& level)
{
    const auto mapped = levelFromString(level);
    if (m_pending.minimumLevel == mapped) {
        return;
    }
    m_pending.minimumLevel = mapped;
    emit settingsChanged();
}

bool LoggingSettingsController::apply()
{
    if (m_pending.enableFileOutput && !iqtools::infra::logging::Logger::isLogDirectoryWritable(m_pending.logDirectory)) {
        m_statusMessage = QStringLiteral("日志目录不可写，请检查路径或权限");
        emit statusMessageChanged();
        return false;
    }

    if (!iqtools::infra::logging::Logger::applyConfig(m_pending)) {
        m_statusMessage = QStringLiteral("日志设置应用失败");
        emit statusMessageChanged();
        return false;
    }

    m_applied = m_pending;
    m_statusMessage = QStringLiteral("日志设置已应用");
    emit settingsChanged();
    emit statusMessageChanged();

    iqtools::core::LogService::info(QStringLiteral("settings.logging"),
                                    QStringLiteral("Logging settings updated"));
    return true;
}

void LoggingSettingsController::resetPending()
{
    m_pending = m_applied;
    m_statusMessage.clear();
    emit settingsChanged();
    emit statusMessageChanged();
}

void LoggingSettingsController::useDefaultDirectory()
{
    setLogDirectory(defaultLogDirectory());
}

QString LoggingSettingsController::levelToString(iqtools::infra::logging::LogLevel level)
{
    switch (level) {
    case iqtools::infra::logging::LogLevel::Debug:
        return QStringLiteral("Debug");
    case iqtools::infra::logging::LogLevel::Info:
        return QStringLiteral("Info");
    case iqtools::infra::logging::LogLevel::Warning:
        return QStringLiteral("Warning");
    case iqtools::infra::logging::LogLevel::Error:
    case iqtools::infra::logging::LogLevel::Critical:
        return QStringLiteral("Error");
    case iqtools::infra::logging::LogLevel::Fatal:
        return QStringLiteral("Fatal");
    }
    return QStringLiteral("Debug");
}

iqtools::infra::logging::LogLevel LoggingSettingsController::levelFromString(const QString& levelText)
{
    const QString value = levelText.trimmed().toLower();
    if (value == QStringLiteral("fatal")) {
        return iqtools::infra::logging::LogLevel::Fatal;
    }
    if (value == QStringLiteral("error") || value == QStringLiteral("critical")) {
        return iqtools::infra::logging::LogLevel::Error;
    }
    if (value == QStringLiteral("warning")) {
        return iqtools::infra::logging::LogLevel::Warning;
    }
    if (value == QStringLiteral("info")) {
        return iqtools::infra::logging::LogLevel::Info;
    }
    return iqtools::infra::logging::LogLevel::Debug;
}

}  // namespace iqtools::app::bridge
