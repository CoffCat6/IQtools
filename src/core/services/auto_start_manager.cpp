#include "auto_start_manager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>

#include "core/services/log_service.h"

namespace iqtools::core {

bool AutoStartManager::isRegistered()
{
    QSettings reg(QLatin1String(kRegistryKey), QSettings::NativeFormat);
    return reg.contains(QLatin1String(kAppName));
}

bool AutoStartManager::registerAutoStart()
{
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QSettings reg(QLatin1String(kRegistryKey), QSettings::NativeFormat);
    reg.setValue(QLatin1String(kAppName), QStringLiteral("\"%1\"").arg(appPath));
    reg.sync();

    const bool ok = reg.status() == QSettings::NoError;
    if (ok) {
        LogService::info(QStringLiteral("autostart"),
                         QStringLiteral("Registered auto-start: %1").arg(appPath));
    } else {
        LogService::error(QStringLiteral("autostart"),
                          QStringLiteral("Failed to register auto-start"));
    }
    return ok;
}

bool AutoStartManager::unregisterAutoStart()
{
    QSettings reg(QLatin1String(kRegistryKey), QSettings::NativeFormat);
    reg.remove(QLatin1String(kAppName));
    reg.sync();

    const bool ok = reg.status() == QSettings::NoError;
    if (ok) {
        LogService::info(QStringLiteral("autostart"),
                         QStringLiteral("Unregistered auto-start"));
    } else {
        LogService::error(QStringLiteral("autostart"),
                          QStringLiteral("Failed to unregister auto-start"));
    }
    return ok;
}

bool AutoStartManager::setEnabled(bool enabled)
{
    return enabled ? registerAutoStart() : unregisterAutoStart();
}

}  // namespace iqtools::core
