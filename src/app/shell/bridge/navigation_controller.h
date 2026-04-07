#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>

namespace iqtools::app::bridge {

class NavigationController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentRoute READ currentRoute NOTIFY currentRouteChanged)
    Q_PROPERTY(QVariantList navigationItems READ navigationItems CONSTANT)

public:
    explicit NavigationController(QObject* parent = nullptr);

    QString currentRoute() const;
    QVariantList navigationItems() const;

    Q_INVOKABLE void navigateTo(const QString& route);
    Q_INVOKABLE bool isCurrentRoute(const QString& route) const;

signals:
    void currentRouteChanged();

private:
    QString m_currentRoute;
};

}  // namespace iqtools::app::bridge
