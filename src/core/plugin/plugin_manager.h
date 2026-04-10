#pragma once

#include <memory>
#include <vector>

#include <QtCore/QString>
#include <QtCore/QVector>

#include "plugin-api/iplugin.h"
#include "plugin-api/iplugin_context.h"

namespace iqtools::core {

class ShortcutManager;

}  // namespace iqtools::core

class QPluginLoader;

namespace iqtools::core {

class PluginManager {
public:
    explicit PluginManager(ShortcutManager* shortcutManager = nullptr);
    ~PluginManager();

    void setShortcutManager(ShortcutManager* shortcutManager);

    void initialize();
    void shutdown();

    bool registerBuiltinPlugin(std::unique_ptr<iqtools::pluginapi::IPlugin> plugin);
    bool loadExternalPlugins(const QString& directoryPath = QString());

    iqtools::pluginapi::IPlugin* plugin(const QString& pluginId) const;
    QVector<iqtools::pluginapi::PluginInfo> loadedPlugins() const;

    iqtools::pluginapi::IPluginContext* context();
    const iqtools::pluginapi::IPluginContext* context() const;

private:
    struct PluginRecord {
        QString sourcePath;
        iqtools::pluginapi::IPlugin* instance {nullptr};
        std::unique_ptr<iqtools::pluginapi::IPlugin> ownedInstance;
        std::unique_ptr<QPluginLoader> dynamicLoader;
        bool initialized {false};
    };

    class HostPluginContext;
    bool activatePlugin(PluginRecord& record);
    void deactivatePlugin(PluginRecord& record);
    QString defaultPluginDirectory() const;

    std::unique_ptr<HostPluginContext> m_context;
    std::vector<PluginRecord> m_plugins;
    bool m_initialized {false};
};

}  // namespace iqtools::core
