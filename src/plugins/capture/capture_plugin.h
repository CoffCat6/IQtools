#pragma once

#ifndef IQTOOLS_CAPTURE_PLUGIN_H
#define IQTOOLS_CAPTURE_PLUGIN_H

#include <plugin-api/itool_plugin.h>

namespace iqtools::plugins {

class CapturePlugin : public pluginapi::IToolPlugin {
public:
    ~CapturePlugin() override = default;

    pluginapi::PluginInfo pluginInfo() const override;
    bool initialize() override;
    void shutdown() override;

    QString toolId() const override;
    QString displayName() const override;
};

}  // namespace iqtools::plugins

#endif  // IQTOOLS_CAPTURE_PLUGIN_H
