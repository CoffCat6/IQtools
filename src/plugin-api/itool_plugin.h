#pragma once

#ifndef IQTOOLS_ITOOL_PLUGIN_H
#define IQTOOLS_ITOOL_PLUGIN_H

#include <QtCore/QObject>

#include "iplugin.h"

namespace iqtools::pluginapi {

class IToolPlugin : public IPlugin {
public:
    ~IToolPlugin() override = default;

    virtual QString toolId() const = 0;
    virtual QString displayName() const = 0;
};

}  // namespace iqtools::pluginapi

#define IQTOOLS_ITOOL_PLUGIN_IID "com.iqtools.pluginapi.IToolPlugin/1.0"
Q_DECLARE_INTERFACE(iqtools::pluginapi::IToolPlugin, IQTOOLS_ITOOL_PLUGIN_IID)

#endif  // IQTOOLS_ITOOL_PLUGIN_H
