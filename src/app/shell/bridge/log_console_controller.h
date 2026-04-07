#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace iqtools::app::bridge {

class LogConsoleController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(int maxEntries READ maxEntries WRITE setMaxEntries NOTIFY maxEntriesChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int filterLevel READ filterLevel WRITE setFilterLevel NOTIFY filterLevelChanged)

public:
    explicit LogConsoleController(QObject* parent = nullptr);

    QStringList entries() const;
    int maxEntries() const;
    bool paused() const;
    int filterLevel() const;

    void setMaxEntries(int max);
    void setPaused(bool paused);
    void setFilterLevel(int level);

    Q_INVOKABLE void clear();

signals:
    void entriesChanged();
    void maxEntriesChanged();
    void pausedChanged();
    void filterLevelChanged();
    void logAdded(const QString& line, int severity);

private slots:
    void onLogEmitted(const QString& formattedLine, int severity);

private:
    void trimEntries();

private:
    QStringList m_entries;
    QStringList m_buffered;  // entries received while paused
    int m_maxEntries {2000};
    bool m_paused {false};
    int m_filterLevel {0};  // 0=Debug, show all
};

}  // namespace iqtools::app::bridge
