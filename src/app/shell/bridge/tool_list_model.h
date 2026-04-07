#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariantMap>

namespace iqtools::core {
class ToolRegistry;
}

namespace iqtools::app::bridge {

class ToolListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum ToolRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        CategoryRole,
        DescriptionRole,
        RouteRole,
        HighlightedRole,
        AvailableRole
    };
    Q_ENUM(ToolRoles)

    explicit ToolListModel(iqtools::core::ToolRegistry& registry, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void reload();

private:
    iqtools::core::ToolRegistry& m_registry;
};

}  // namespace iqtools::app::bridge
