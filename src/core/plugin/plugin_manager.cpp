#include "plugin_manager.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QStringList>

#include "plugin-api/iplugin.h"
#include "core/services/log_service.h"
#include "core/settings/shortcut_manager.h"

namespace iqtools::core {

namespace {

QStringList pluginFileNameFilters()
{
#if defined(Q_OS_WIN)
	return {QStringLiteral("*.dll")};
#elif defined(Q_OS_MACOS)
	return {QStringLiteral("*.dylib"), QStringLiteral("*.bundle")};
#else
	return {QStringLiteral("*.so")};
#endif
}

}  // namespace

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

PluginManager::~PluginManager()
{
	shutdown();
}

void PluginManager::setShortcutManager(ShortcutManager* shortcutManager)
{
	m_context->setShortcutManager(shortcutManager);
}

void PluginManager::initialize()
{
	if (m_initialized) {
		return;
	}

	m_initialized = true;
	for (auto& record : m_plugins) {
		activatePlugin(record);
	}

	loadExternalPlugins();

	IQ_LOG_INFO(QStringLiteral("core.plugin"),
				QStringLiteral("Plugin manager initialized (%1 active)")
					.arg(loadedPlugins().size()));
}

void PluginManager::shutdown()
{
	if (!m_initialized && m_plugins.empty()) {
		return;
	}

	for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
		deactivatePlugin(*it);

		if (it->dynamicLoader != nullptr) {
			const QString path = it->sourcePath;
			if (!it->dynamicLoader->unload()) {
				IQ_LOG_WARNING(
					QStringLiteral("core.plugin"),
					QStringLiteral("Unload plugin failed: %1 (%2)")
						.arg(path, it->dynamicLoader->errorString()));
			}
		}
	}

	m_plugins.clear();
	m_initialized = false;
}

bool PluginManager::registerBuiltinPlugin(
	std::unique_ptr<iqtools::pluginapi::IPlugin> plugin)
{
	if (plugin == nullptr) {
		return false;
	}

	const iqtools::pluginapi::PluginInfo info = plugin->pluginInfo();
	const QString pluginId = info.id.trimmed();
	if (pluginId.isEmpty()) {
		IQ_LOG_WARNING(
			QStringLiteral("core.plugin"),
			QStringLiteral("Skip builtin plugin with empty id"));
		return false;
	}

	for (const auto& record : m_plugins) {
		if (record.instance == nullptr) {
			continue;
		}
		if (record.instance->pluginInfo().id == pluginId) {
			IQ_LOG_WARNING(
				QStringLiteral("core.plugin"),
				QStringLiteral("Duplicate plugin id ignored: %1").arg(pluginId));
			return false;
		}
	}

	PluginRecord record;
	record.sourcePath = QStringLiteral("builtin:%1").arg(pluginId);
	record.instance = plugin.get();
	record.ownedInstance = std::move(plugin);

	if (m_initialized && !activatePlugin(record)) {
		return false;
	}

	m_plugins.push_back(std::move(record));
	return true;
}

bool PluginManager::loadExternalPlugins(const QString& directoryPath)
{
	QString pluginDirectory = directoryPath.trimmed();
	if (pluginDirectory.isEmpty()) {
		pluginDirectory = defaultPluginDirectory();
	}

	QDir dir(pluginDirectory);
	if (!dir.exists()) {
		IQ_LOG_INFO(
			QStringLiteral("core.plugin"),
			QStringLiteral("Plugin directory not found: %1").arg(pluginDirectory));
		return false;
	}

	bool loadedAny = false;
	const QStringList entries =
		dir.entryList(pluginFileNameFilters(), QDir::Files | QDir::Readable);
	for (const QString& name : entries) {
		const QString path = dir.absoluteFilePath(name);

		bool alreadyLoaded = false;
		for (const auto& record : m_plugins) {
			if (record.sourcePath == path) {
				alreadyLoaded = true;
				break;
			}
		}
		if (alreadyLoaded) {
			continue;
		}

		auto loader = std::make_unique<QPluginLoader>(path);
		QObject* object = loader->instance();
		if (object == nullptr) {
			IQ_LOG_WARNING(
				QStringLiteral("core.plugin"),
				QStringLiteral("Load plugin failed: %1 (%2)")
					.arg(path, loader->errorString()));
			continue;
		}

		auto* instance = qobject_cast<iqtools::pluginapi::IPlugin*>(object);
		if (instance == nullptr) {
			IQ_LOG_WARNING(
				QStringLiteral("core.plugin"),
				QStringLiteral("Plugin does not implement IPlugin: %1").arg(path));
			loader->unload();
			continue;
		}

		const QString pluginId = instance->pluginInfo().id.trimmed();
		if (pluginId.isEmpty()) {
			IQ_LOG_WARNING(
				QStringLiteral("core.plugin"),
				QStringLiteral("Skip plugin with empty id: %1").arg(path));
			loader->unload();
			continue;
		}

		bool duplicated = false;
		for (const auto& record : m_plugins) {
			if (record.instance == nullptr) {
				continue;
			}
			if (record.instance->pluginInfo().id == pluginId) {
				duplicated = true;
				break;
			}
		}
		if (duplicated) {
			IQ_LOG_WARNING(
				QStringLiteral("core.plugin"),
				QStringLiteral("Duplicate plugin id ignored: %1 (%2)")
					.arg(pluginId, path));
			loader->unload();
			continue;
		}

		PluginRecord record;
		record.sourcePath = path;
		record.instance = instance;
		record.dynamicLoader = std::move(loader);

		if (m_initialized && !activatePlugin(record)) {
			record.dynamicLoader->unload();
			continue;
		}

		m_plugins.push_back(std::move(record));
		loadedAny = true;
	}

	return loadedAny;
}

iqtools::pluginapi::IPlugin* PluginManager::plugin(const QString& pluginId) const
{
	const QString normalized = pluginId.trimmed();
	if (normalized.isEmpty()) {
		return nullptr;
	}

	for (const auto& record : m_plugins) {
		if (!record.initialized || record.instance == nullptr) {
			continue;
		}
		if (record.instance->pluginInfo().id == normalized) {
			return record.instance;
		}
	}

	return nullptr;
}

QVector<iqtools::pluginapi::PluginInfo> PluginManager::loadedPlugins() const
{
	QVector<iqtools::pluginapi::PluginInfo> plugins;
	for (const auto& record : m_plugins) {
		if (!record.initialized || record.instance == nullptr) {
			continue;
		}
		plugins.push_back(record.instance->pluginInfo());
	}
	return plugins;
}

iqtools::pluginapi::IPluginContext* PluginManager::context()
{
	return m_context.get();
}

const iqtools::pluginapi::IPluginContext* PluginManager::context() const
{
	return m_context.get();
}

bool PluginManager::activatePlugin(PluginRecord& record)
{
	if (record.instance == nullptr) {
		return false;
	}

	if (record.initialized) {
		return true;
	}

	const iqtools::pluginapi::PluginInfo info = record.instance->pluginInfo();
	if (info.id.trimmed().isEmpty()) {
		IQ_LOG_WARNING(
			QStringLiteral("core.plugin"),
			QStringLiteral("Plugin id is empty, source: %1").arg(record.sourcePath));
		return false;
	}

	if (!record.instance->initialize()) {
		IQ_LOG_WARNING(
			QStringLiteral("core.plugin"),
			QStringLiteral("Plugin initialize failed: %1 (%2)")
				.arg(info.id, record.sourcePath));
		return false;
	}

	record.initialized = true;
	IQ_LOG_INFO(
		QStringLiteral("core.plugin"),
		QStringLiteral("Plugin activated: %1 (%2)")
			.arg(info.id, record.sourcePath));
	return true;
}

void PluginManager::deactivatePlugin(PluginRecord& record)
{
	if (!record.initialized || record.instance == nullptr) {
		return;
	}

	const iqtools::pluginapi::PluginInfo info = record.instance->pluginInfo();
	m_context->unregisterPluginShortcuts(info.id);
	record.instance->shutdown();
	record.initialized = false;

	IQ_LOG_INFO(
		QStringLiteral("core.plugin"),
		QStringLiteral("Plugin deactivated: %1 (%2)")
			.arg(info.id, record.sourcePath));
}

QString PluginManager::defaultPluginDirectory() const
{
	const QString appDir = QCoreApplication::applicationDirPath();
	return QDir(appDir).filePath(QStringLiteral("plugins"));
}

}  // namespace iqtools::core
