#pragma once

#ifndef IQTOOLS_LOG_LEVEL_H
#define IQTOOLS_LOG_LEVEL_H

namespace iqtools::infra::logging {

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Critical,
    Error,
    Fatal
};

}  // namespace iqtools::infra::logging

#endif  // IQTOOLS_LOG_LEVEL_H
