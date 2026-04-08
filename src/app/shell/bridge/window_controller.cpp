#include "window_controller.h"

#include <QtCore/QEvent>
#include <QtGui/QIcon>
#include <QtGui/QAction>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

#include "core/services/log_service.h"
#include "core/settings/settings_manager.h"

namespace iqtools::app::bridge {

WindowController::WindowController(iqtools::core::SettingsManager* settingsManager,
                                   QObject* parent)
    : QObject(parent)
    , m_settingsManager(settingsManager)
{
    setupTrayIcon();
}

WindowController::~WindowController()
{
    delete m_trayMenu;
}

bool WindowController::trayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void WindowController::attachToWindow(QWindow* window)
{
    if (window == nullptr) {
        return;
    }

    // Remove from previous window if any
    if (m_window != nullptr) {
        m_window->removeEventFilter(this);
    }

    m_window = window;
    m_window->installEventFilter(this);

    iqtools::core::LogService::info(QStringLiteral("window"),
                                    QStringLiteral("WindowController attached to window"));
}

void WindowController::setTrayVisible(bool visible)
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->setVisible(visible);
    }
}

void WindowController::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        iqtools::core::LogService::info(QStringLiteral("window"),
                                        QStringLiteral("System tray not available"));
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(QStringLiteral(":/IQtools/src/resources/icons/app_icon.png")));
    m_trayIcon->setToolTip(QStringLiteral("IQtools"));

    // Build context menu
    m_trayMenu = new QMenu();
    auto* showAction = m_trayMenu->addAction(QStringLiteral("显示主窗口"));
    m_trayMenu->addSeparator();
    auto* quitAction = m_trayMenu->addAction(QStringLiteral("退出"));

    connect(showAction, &QAction::triggered, this, [this]() {
        if (m_window != nullptr) {
            m_window->show();
            m_window->raise();
            m_window->requestActivate();
        }
        emit showWindowRequested();
    });

    connect(quitAction, &QAction::triggered, this, &WindowController::quitRequested);

    // Double-click on tray icon shows the window
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick && m_window != nullptr) {
            m_window->show();
            m_window->raise();
            m_window->requestActivate();
        }
    });

    m_trayIcon->setContextMenu(m_trayMenu);

    // Show tray icon if minimizeToTray is currently enabled
    m_trayIcon->setVisible(m_settingsManager->minimizeToTray());

    iqtools::core::LogService::info(QStringLiteral("window"),
                                    QStringLiteral("System tray icon initialized"));
}

bool WindowController::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window && event->type() == QEvent::Close) {
        // Handle minimize-to-tray
        if (m_settingsManager->minimizeToTray() && m_trayIcon != nullptr && m_trayIcon->isVisible()) {
            event->ignore();
            m_window->hide();
            m_trayIcon->showMessage(QStringLiteral("IQtools"),
                                    QStringLiteral("程序已最小化到托盘"),
                                    QSystemTrayIcon::Information,
                                    2000);
            iqtools::core::LogService::info(QStringLiteral("window"),
                                            QStringLiteral("Window minimized to tray"));
            return true;
        }

        // Handle confirm-on-exit
        if (m_settingsManager->confirmOnExit()) {
            event->ignore();
            emit exitConfirmationNeeded();
            iqtools::core::LogService::info(QStringLiteral("window"),
                                            QStringLiteral("Exit confirmation requested"));
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}

}  // namespace iqtools::app::bridge
