#include "log_console_controller.h"

#include "infra/logging/log_manager.h"

namespace iqtools::app::bridge {

LogConsoleController::LogConsoleController(QObject* parent)
    : QObject(parent)
{
    auto& manager = iqtools::infra::logging::LogManager::instance();
    connect(&manager,
            &iqtools::infra::logging::LogManager::logEmitted,
            this,
            &LogConsoleController::onLogEmitted,
            Qt::QueuedConnection);
}

QStringList LogConsoleController::entries() const
{
    return m_entries;
}

int LogConsoleController::maxEntries() const
{
    return m_maxEntries;
}

bool LogConsoleController::paused() const
{
    return m_paused;
}

int LogConsoleController::filterLevel() const
{
    return m_filterLevel;
}

void LogConsoleController::setMaxEntries(int max)
{
    if (max < 100) {
        max = 100;
    }
    if (m_maxEntries == max) {
        return;
    }
    m_maxEntries = max;
    trimEntries();
    emit maxEntriesChanged();
}

void LogConsoleController::setPaused(bool paused)
{
    if (m_paused == paused) {
        return;
    }
    m_paused = paused;

    if (!m_paused && !m_buffered.isEmpty()) {
        m_entries.append(m_buffered);
        m_buffered.clear();
        trimEntries();
        emit entriesChanged();
    }

    emit pausedChanged();
}

void LogConsoleController::setFilterLevel(int level)
{
    if (m_filterLevel == level) {
        return;
    }
    m_filterLevel = level;
    emit filterLevelChanged();
}

void LogConsoleController::clear()
{
    m_entries.clear();
    m_buffered.clear();
    emit entriesChanged();
}

void LogConsoleController::onLogEmitted(const QString& formattedLine, int severity)
{
    if (severity < m_filterLevel) {
        return;
    }

    if (m_paused) {
        m_buffered.append(formattedLine);
        // Cap buffer while paused to avoid unbounded growth
        while (m_buffered.size() > m_maxEntries) {
            m_buffered.removeFirst();
        }
        return;
    }

    m_entries.append(formattedLine);
    trimEntries();
    emit logAdded(formattedLine, severity);
    emit entriesChanged();
}

void LogConsoleController::trimEntries()
{
    while (m_entries.size() > m_maxEntries) {
        m_entries.removeFirst();
    }
}

}  // namespace iqtools::app::bridge
