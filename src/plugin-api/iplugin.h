#pragma once

#ifndef IQTOOLS_IPLUGIN_H
#define IQTOOLS_IPLUGIN_H

#include <QtCore/QObject>
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

#define IQTOOLS_IPLUGIN_IID "com.iqtools.pluginapi.IPlugin/1.0"
Q_DECLARE_INTERFACE(iqtools::pluginapi::IPlugin, IQTOOLS_IPLUGIN_IID)

#endif  // IQTOOLS_IPLUGIN_H
