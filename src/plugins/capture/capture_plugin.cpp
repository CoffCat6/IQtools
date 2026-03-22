#include "capture_plugin.h"

#include <QtCore/QString>

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
    infra::logging::Logger::info(
        QStringLiteral("plugins.capture"),
        QStringLiteral("CapturePlugin initialized"));
    return true;
}

void CapturePlugin::shutdown()
{
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

}  // namespace iqtools::plugins
