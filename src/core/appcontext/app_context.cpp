#include "app_context.h"

namespace iqtools::core {

AppContext::AppContext() = default;

ToolRegistry& AppContext::toolRegistry()
{
    return m_toolRegistry;
}

const ToolRegistry& AppContext::toolRegistry() const
{
    return m_toolRegistry;
}

}  // namespace iqtools::core
