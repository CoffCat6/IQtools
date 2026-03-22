#pragma once

#ifndef IQTOOLS_ITOOL_PLUGIN_H
#define IQTOOLS_ITOOL_PLUGIN_H

#include "iplugin.h"

namespace iqtools::pluginapi {

class IToolPlugin : public IPlugin {
public:
    ~IToolPlugin() override = default;

    virtual QString toolId() const = 0;
    virtual QString displayName() const = 0;
};

}  // namespace iqtools::pluginapi

#endif  // IQTOOLS_ITOOL_PLUGIN_H
