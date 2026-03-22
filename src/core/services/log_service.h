#pragma once

#include <QtCore/QString>

namespace iqtools::core {

class LogService {
public:
    static void debug(const QString& category, const QString& message);
    static void info(const QString& category, const QString& message);
    static void warning(const QString& category, const QString& message);
    static void error(const QString& category, const QString& message);
};

}  // namespace iqtools::core
