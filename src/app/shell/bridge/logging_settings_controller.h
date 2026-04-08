#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "infra/logging/logger.h"

namespace iqtools::core {
class SettingsManager;
}

namespace iqtools::app::bridge {

class LoggingSettingsController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool consoleEnabled READ consoleEnabled WRITE setConsoleEnabled
                 NOTIFY settingsChanged)
  Q_PROPERTY(bool fileEnabled READ fileEnabled WRITE setFileEnabled NOTIFY
                 settingsChanged)
  Q_PROPERTY(QString logDirectory READ logDirectory WRITE setLogDirectory NOTIFY
                 settingsChanged)
  Q_PROPERTY(QString minimumLevel READ minimumLevel WRITE setMinimumLevel NOTIFY
                 settingsChanged)
  Q_PROPERTY(QStringList levelOptions READ levelOptions CONSTANT)
  Q_PROPERTY(bool dirty READ dirty NOTIFY settingsChanged)
  Q_PROPERTY(
      QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
  explicit LoggingSettingsController(iqtools::core::SettingsManager* settingsManager,
                                     QObject *parent = nullptr);

  bool consoleEnabled() const;
  bool fileEnabled() const;
  QString logDirectory() const;
  QString minimumLevel() const;
  QStringList levelOptions() const;
  bool dirty() const;
  QString statusMessage() const;

  void setConsoleEnabled(bool enabled);
  void setFileEnabled(bool enabled);
  void setLogDirectory(const QString &directory);
  void setMinimumLevel(const QString &level);

  Q_INVOKABLE bool apply();
  Q_INVOKABLE void resetPending();
  Q_INVOKABLE void useDefaultDirectory();

signals:
  void settingsChanged();
  void statusMessageChanged();

private:
  void loadFromSettings();
  static QString levelToString(iqtools::infra::logging::LogLevel level);
  static iqtools::infra::logging::LogLevel
  levelFromString(const QString &levelText);

private:
  iqtools::core::SettingsManager* m_settingsManager{nullptr};
  iqtools::infra::logging::LoggingConfig m_applied;
  iqtools::infra::logging::LoggingConfig m_pending;
  QString m_statusMessage;
};

} // namespace iqtools::app::bridge
