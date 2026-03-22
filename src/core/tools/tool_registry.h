#pragma once

#include <QtCore/QVector>

#include "tool_metadata.h"

namespace iqtools::core {

class ToolRegistry {
public:
    ToolRegistry();

    void registerTool(const ToolMetadata& metadata);
    const QVector<ToolMetadata>& tools() const;

private:
    QVector<ToolMetadata> m_tools;
};

}  // namespace iqtools::core
