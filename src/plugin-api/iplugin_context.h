#pragma once

#ifndef IQTOOLS_IPLUGIN_CONTEXT_H
#define IQTOOLS_IPLUGIN_CONTEXT_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QKeySequence>

class QAction;
class QWidget;

namespace iqtools::pluginapi {

class IPluginContext {
public:
    virtual ~IPluginContext() = default;

    /// Register a QAction shortcut owned by a plugin.
    virtual bool registerPluginActionShortcut(const QString& pluginId,
                                              const QString& actionId,
                                              const QString& category,
                                              const QString& description,
                                              const QKeySequence& defaultKey,
                                              QAction* action) = 0;

    /// Register a QShortcut-owned key trigger in plugin window/widget context.
    virtual bool registerPluginWindowShortcut(const QString& pluginId,
                                              const QString& actionId,
                                              const QString& category,
                                              const QString& description,
                                              const QKeySequence& defaultKey,
                                              QWidget* parent,
                                              QObject* receiver,
                                              const char* slot) = 0;

    /// Remove all shortcuts belonging to a plugin when plugin unloads.
    virtual void unregisterPluginShortcuts(const QString& pluginId) = 0;

    /// Returns conflicting shortcut id or empty string when no conflict exists.
    virtual QString checkShortcutConflict(const QKeySequence& key,
                                          const QString& ignoreId = QString()) const = 0;
};

}  // namespace iqtools::pluginapi

#endif  // IQTOOLS_IPLUGIN_CONTEXT_H
