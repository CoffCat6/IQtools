#include "navigation_controller.h"

#include "core/services/log_service.h"

namespace iqtools::app::bridge {

NavigationController::NavigationController(QObject* parent)
    : QObject(parent)
    , m_currentRoute(QStringLiteral("home"))
{
}

QString NavigationController::currentRoute() const
{
    return m_currentRoute;
}

QVariantList NavigationController::navigationItems() const
{
    return {
        QVariantMap{{QStringLiteral("label"), QStringLiteral("首页")}, {QStringLiteral("route"), QStringLiteral("home")}},
        QVariantMap{{QStringLiteral("label"), QStringLiteral("截图工具")}, {QStringLiteral("route"), QStringLiteral("capture")}},
        QVariantMap{{QStringLiteral("label"), QStringLiteral("翻译工具")}, {QStringLiteral("route"), QStringLiteral("translate")}},
        QVariantMap{{QStringLiteral("label"), QStringLiteral("插件管理")}, {QStringLiteral("route"), QStringLiteral("plugins")}},
        QVariantMap{{QStringLiteral("label"), QStringLiteral("设置")}, {QStringLiteral("route"), QStringLiteral("settings")}},
    };
}

void NavigationController::navigateTo(const QString& route)
{
    const QString normalized = route.trimmed();
    if (normalized.isEmpty() || normalized == m_currentRoute) {
        return;
    }

    m_currentRoute = normalized;
    iqtools::core::LogService::info(QStringLiteral("app.shell.qml"),
                                    QStringLiteral("Navigation switched: %1").arg(m_currentRoute));
    emit currentRouteChanged();
}

bool NavigationController::isCurrentRoute(const QString& route) const
{
    return m_currentRoute == route;
}

}  // namespace iqtools::app::bridge
