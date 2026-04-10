#include "capture_plugin.h"

#include <QtCore/QString>

#include <core/capture/screen_capture_service.h>
#include <infra/logging/logger.h>

namespace iqtools::plugins {

pluginapi::PluginInfo CapturePlugin::pluginInfo() const
{
    return {
        QStringLiteral("com.iqtools.plugin.capture"),
        QStringLiteral("截图工具"),
        QStringLiteral("1.0.0"),
        QStringLiteral("IQtools")
    };
}

bool CapturePlugin::initialize()
{
    if (m_captureService == nullptr) {
        m_captureService = std::make_unique<iqtools::core::ScreenCaptureService>();
    }

    infra::logging::Logger::info(
        QStringLiteral("plugins.capture"),
        QStringLiteral("CapturePlugin initialized"));
    return true;
}

void CapturePlugin::shutdown()
{
    m_captureService.reset();

    infra::logging::Logger::info(
        QStringLiteral("plugins.capture"),
        QStringLiteral("CapturePlugin shutdown"));
}

QString CapturePlugin::toolId() const
{
    return QStringLiteral("capture");
}

QString CapturePlugin::displayName() const
{
    return QStringLiteral("截图工具");
}

iqtools::core::ICaptureService* CapturePlugin::captureService() const
{
    return m_captureService.get();
}

}  // namespace iqtools::plugins
