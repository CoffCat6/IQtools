#pragma once

#include "core/plugin/plugin_manager.h"
#include "core/tools/tool_registry.h"

namespace iqtools::core {

class AppContext {
public:
    AppContext();

    ToolRegistry& toolRegistry();
    const ToolRegistry& toolRegistry() const;

    PluginManager& pluginManager();
    const PluginManager& pluginManager() const;

private:
    ToolRegistry m_toolRegistry;
    PluginManager m_pluginManager;
};

}  // namespace iqtools::core
