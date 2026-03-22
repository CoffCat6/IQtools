#pragma once

#ifndef IQTOOLS_TRANSLATE_PLUGIN_H
#define IQTOOLS_TRANSLATE_PLUGIN_H

#include <plugin-api/itool_plugin.h>

namespace iqtools::plugins {

class TranslatePlugin : public pluginapi::IToolPlugin {
public:
    ~TranslatePlugin() override = default;

    pluginapi::PluginInfo pluginInfo() const override;
    bool initialize() override;
    void shutdown() override;

    QString toolId() const override;
    QString displayName() const override;
};

}  // namespace iqtools::plugins

#endif  // IQTOOLS_TRANSLATE_PLUGIN_H
