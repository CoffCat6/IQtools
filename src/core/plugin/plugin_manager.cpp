#include "plugin_manager.h"

#include <utility>

#include "core/services/log_service.h"
#include "core/settings/shortcut_manager.h"

namespace iqtools::core {

class PluginManager::HostPluginContext final : public iqtools::pluginapi::IPluginContext {
public:
	explicit HostPluginContext(ShortcutManager* shortcutManager)
		: m_shortcutManager(shortcutManager)
	{
	}

	void setShortcutManager(ShortcutManager* shortcutManager)
	{
		m_shortcutManager = shortcutManager;
	}

	bool registerPluginActionShortcut(const QString& pluginId,
									  const QString& actionId,
									  const QString& category,
									  const QString& description,
									  const QKeySequence& defaultKey,
									  QAction* action) override
	{
		if (m_shortcutManager == nullptr) {
			return false;
		}
		return m_shortcutManager->registerPluginAction(pluginId, actionId, category,
													   description, defaultKey, action);
	}

	bool registerPluginWindowShortcut(const QString& pluginId,
									  const QString& actionId,
									  const QString& category,
									  const QString& description,
									  const QKeySequence& defaultKey,
									  QWidget* parent,
									  QObject* receiver,
									  const char* slot) override
	{
		if (m_shortcutManager == nullptr) {
			return false;
		}
		return m_shortcutManager->registerPluginShortcut(pluginId, actionId, category,
														 description, defaultKey,
														 parent, receiver, slot);
	}

	void unregisterPluginShortcuts(const QString& pluginId) override
	{
		if (m_shortcutManager == nullptr) {
			return;
		}
		m_shortcutManager->unregisterPluginShortcuts(pluginId);
	}

	QString checkShortcutConflict(const QKeySequence& key,
								  const QString& ignoreId) const override
	{
		if (m_shortcutManager == nullptr) {
			return QString();
		}
		return m_shortcutManager->checkConflict(key, ignoreId);
	}

private:
	ShortcutManager* m_shortcutManager {nullptr};
};

PluginManager::PluginManager(ShortcutManager* shortcutManager)
	: m_context(std::make_unique<HostPluginContext>(shortcutManager))
{
}

PluginManager::~PluginManager() = default;

void PluginManager::setShortcutManager(ShortcutManager* shortcutManager)
{
	m_context->setShortcutManager(shortcutManager);
}

void PluginManager::initialize()
{
	IQ_LOG_INFO(QStringLiteral("core.plugin"),
				QStringLiteral("Plugin manager initialized"));
}

iqtools::pluginapi::IPluginContext* PluginManager::context()
{
	return m_context.get();
}

const iqtools::pluginapi::IPluginContext* PluginManager::context() const
{
	return m_context.get();
}

}  // namespace iqtools::core
