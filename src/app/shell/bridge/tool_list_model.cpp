#include "tool_list_model.h"

#include "core/tools/tool_metadata.h"
#include "core/tools/tool_registry.h"

namespace iqtools::app::bridge {

namespace {

QString routeForTool(const QString& id)
{
    if (id == QStringLiteral("home") || id == QStringLiteral("capture") || id == QStringLiteral("translate")) {
        return id;
    }
    return QStringLiteral("home");
}

bool highlightedForTool(const QString& id)
{
    return id == QStringLiteral("home") || id == QStringLiteral("translate");
}

QVariantMap toVariantMap(const iqtools::core::ToolMetadata& tool)
{
    return {
        {QStringLiteral("id"), tool.id},
        {QStringLiteral("name"), tool.name},
        {QStringLiteral("category"), tool.category},
        {QStringLiteral("description"), tool.description},
        {QStringLiteral("route"), routeForTool(tool.id)},
        {QStringLiteral("highlighted"), highlightedForTool(tool.id)},
        {QStringLiteral("available"), true},
    };
}

}  // namespace

ToolListModel::ToolListModel(iqtools::core::ToolRegistry& registry, QObject* parent)
    : QAbstractListModel(parent)
    , m_registry(registry)
{
}

int ToolListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_registry.tools().size();
}

QVariant ToolListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_registry.tools().size()) {
        return {};
    }

    const auto& tool = m_registry.tools().at(index.row());
    switch (role) {
    case IdRole:
        return tool.id;
    case NameRole:
        return tool.name;
    case CategoryRole:
        return tool.category;
    case DescriptionRole:
        return tool.description;
    case RouteRole:
        return routeForTool(tool.id);
    case HighlightedRole:
        return highlightedForTool(tool.id);
    case AvailableRole:
        return true;
    default:
        return {};
    }
}

QHash<int, QByteArray> ToolListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {CategoryRole, "category"},
        {DescriptionRole, "description"},
        {RouteRole, "route"},
        {HighlightedRole, "highlighted"},
        {AvailableRole, "available"},
    };
}

QVariantMap ToolListModel::get(int row) const
{
    if (row < 0 || row >= m_registry.tools().size()) {
        return {};
    }
    return toVariantMap(m_registry.tools().at(row));
}

void ToolListModel::reload()
{
    beginResetModel();
    endResetModel();
}

}  // namespace iqtools::app::bridge
