#pragma once

#ifndef IQTOOLS_LOG_ENTRY_H
#define IQTOOLS_LOG_ENTRY_H

#include <QtCore/QDateTime>
#include <QtCore/QString>

#include "log_level.h"

namespace iqtools::infra::logging {

struct LogEntry {
    QDateTime timestamp;
    LogLevel level {LogLevel::Info};
    QString category;
    QString message;
};

}  // namespace iqtools::infra::logging

#endif  // IQTOOLS_LOG_ENTRY_H
