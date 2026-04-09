#pragma once

#include <memory>

#include "plugin-api/iplugin_context.h"

namespace iqtools::core {

class ShortcutManager;

class PluginManager {
public:
    explicit PluginManager(ShortcutManager* shortcutManager = nullptr);
    ~PluginManager();

    void setShortcutManager(ShortcutManager* shortcutManager);

    void initialize();

    iqtools::pluginapi::IPluginContext* context();
    const iqtools::pluginapi::IPluginContext* context() const;

private:
    class HostPluginContext;
    std::unique_ptr<HostPluginContext> m_context;
};

}  // namespace iqtools::core
