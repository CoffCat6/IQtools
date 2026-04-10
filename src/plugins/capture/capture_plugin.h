#pragma once

#ifndef IQTOOLS_CAPTURE_PLUGIN_H
#define IQTOOLS_CAPTURE_PLUGIN_H

#include <memory>

#include <plugin-api/itool_plugin.h>

namespace iqtools::core {
class ICaptureService;
}

namespace iqtools::plugins {

class CapturePlugin : public pluginapi::IToolPlugin {
public:
    ~CapturePlugin() override = default;

    pluginapi::PluginInfo pluginInfo() const override;
    bool initialize() override;
    void shutdown() override;

    QString toolId() const override;
    QString displayName() const override;

    iqtools::core::ICaptureService* captureService() const;

private:
    std::unique_ptr<iqtools::core::ICaptureService> m_captureService;
};

}  // namespace iqtools::plugins

#endif  // IQTOOLS_CAPTURE_PLUGIN_H
