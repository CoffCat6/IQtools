#pragma once

#include <QtCore/QString>

namespace iqtools::core {

/// Manages OS-level auto-start registration (Windows: registry Run key).
class AutoStartManager {
public:
    /// Check whether the application is currently registered for auto-start.
    static bool isRegistered();

    /// Register the application for auto-start at login.
    /// Returns true on success.
    static bool registerAutoStart();

    /// Unregister the application from auto-start.
    /// Returns true on success.
    static bool unregisterAutoStart();

    /// Synchronize the registry to match the desired state.
    /// Returns true on success.
    static bool setEnabled(bool enabled);

private:
    static constexpr auto kRegistryKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    static constexpr auto kAppName     = "IQtools";
};

}  // namespace iqtools::core
