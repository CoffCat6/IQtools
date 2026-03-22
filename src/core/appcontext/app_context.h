#pragma once

#include "core/tools/tool_registry.h"

namespace iqtools::core {

class AppContext {
public:
    AppContext();

    ToolRegistry& toolRegistry();
    const ToolRegistry& toolRegistry() const;

private:
    ToolRegistry m_toolRegistry;
};

}  // namespace iqtools::core
