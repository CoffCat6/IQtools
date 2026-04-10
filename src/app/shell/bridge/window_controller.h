#pragma once

#include <QtCore/QObject>
#include <QtGui/QWindow>

class QSystemTrayIcon;
class QMenu;
class QAction;

namespace iqtools::core {
class SettingsManager;
}

namespace iqtools::app::bridge {

/// Controls window-level behaviors: system tray, minimizeToTray, confirmOnExit.
/// Installed as an event filter on the QML ApplicationWindow.
class WindowController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)

public:
    explicit WindowController(iqtools::core::SettingsManager* settingsManager,
                              QObject* parent = nullptr);
    ~WindowController() override;

    bool trayAvailable() const;

    /// Attach this controller to the given QML window. Must be called after
    /// the QML engine has created the root window.
    Q_INVOKABLE void attachToWindow(QWindow* window);

    /// Set whether the tray icon is visible.
    Q_INVOKABLE void setTrayVisible(bool visible);

signals:
    /// Emitted when user selects tray menu action for instant full-screen capture.
    void quickCaptureRequested();

    /// Emitted when user selects tray menu action for instant region capture.
    void quickRegionCaptureRequested();

    /// Emitted when the user selects "Show" from the tray context menu.
    void showWindowRequested();

    /// Emitted when the user selects "Quit" from the tray context menu
    /// or confirms an exit. The QML side should call Qt.quit().
    void quitRequested();

    /// Emitted when a close event is intercepted and confirmOnExit is true.
    void exitConfirmationNeeded();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupTrayIcon();

private:
    iqtools::core::SettingsManager* m_settingsManager {nullptr};
    QWindow* m_window {nullptr};
    QSystemTrayIcon* m_trayIcon {nullptr};
    QMenu* m_trayMenu {nullptr};
};

}  // namespace iqtools::app::bridge
