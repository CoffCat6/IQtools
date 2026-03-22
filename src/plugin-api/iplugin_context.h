#pragma once

#ifndef IQTOOLS_IPLUGIN_CONTEXT_H
#define IQTOOLS_IPLUGIN_CONTEXT_H

namespace iqtools::pluginapi {

class IPluginContext {
public:
    virtual ~IPluginContext() = default;
};

}  // namespace iqtools::pluginapi

#endif  // IQTOOLS_IPLUGIN_CONTEXT_H
