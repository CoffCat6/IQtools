#pragma once

#ifndef IQTOOLS_IPLUGIN_H
#define IQTOOLS_IPLUGIN_H

#include <QtCore/QString>

namespace iqtools::pluginapi {

struct PluginInfo {
    QString id;
    QString name;
    QString version;
    QString vendor;
};

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual PluginInfo pluginInfo() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

}  // namespace iqtools::pluginapi

#endif  // IQTOOLS_IPLUGIN_H
