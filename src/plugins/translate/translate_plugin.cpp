#include "translate_plugin.h"

#include <QtCore/QString>

#include <infra/logging/logger.h>

namespace iqtools::plugins {

pluginapi::PluginInfo TranslatePlugin::pluginInfo() const
{
    return {
        QStringLiteral("com.iqtools.plugin.translate"),
        QStringLiteral("翻译工具"),
        QStringLiteral("1.0.0"),
        QStringLiteral("IQtools")
    };
}

bool TranslatePlugin::initialize()
{
    infra::logging::Logger::info(
        QStringLiteral("plugins.translate"),
        QStringLiteral("TranslatePlugin initialized - Translation service ready"));
    return true;
}

void TranslatePlugin::shutdown()
{
    infra::logging::Logger::info(
        QStringLiteral("plugins.translate"),
        QStringLiteral("TranslatePlugin shutdown"));
}

QString TranslatePlugin::toolId() const
{
    return QStringLiteral("translate");
}

QString TranslatePlugin::displayName() const
{
    return QStringLiteral("翻译工具");
}

}  // namespace iqtools::plugins
