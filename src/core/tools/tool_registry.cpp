#include "tool_registry.h"

namespace iqtools::core {

ToolRegistry::ToolRegistry()
{
    registerTool({QStringLiteral("home"), QStringLiteral("首页"), QStringLiteral("system"), QStringLiteral("应用首页")});
    registerTool({QStringLiteral("capture"), QStringLiteral("截图工具"), QStringLiteral("tools"), QStringLiteral("屏幕截图能力")});
    registerTool({QStringLiteral("translate"), QStringLiteral("翻译工具"), QStringLiteral("tools"), QStringLiteral("文本翻译能力")});
}

void ToolRegistry::registerTool(const ToolMetadata& metadata)
{
    m_tools.push_back(metadata);
}

const QVector<ToolMetadata>& ToolRegistry::tools() const
{
    return m_tools;
}

}  // namespace iqtools::core
