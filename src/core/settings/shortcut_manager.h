#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QKeySequence>

class QAction;
class QShortcut;
class QWidget;

namespace iqtools::core {

class SettingsManager;

struct ShortcutItem {
    QString id;
    QString description;
    QString category;
    QKeySequence defaultKey;
    QKeySequence currentKey;
    QPointer<QAction> action;
    QPointer<QShortcut> shortcut;
};

/// Manages application and plugin shortcuts with conflict checking and persistence.
class ShortcutManager : public QObject {
    Q_OBJECT

public:
    explicit ShortcutManager(SettingsManager* settingsManager, QObject* parent = nullptr);

    void loadConfig();
    bool saveConfig();

    bool reserveShortcut(const QString& id,
                         const QString& category,
                         const QString& description,
                         const QKeySequence& defaultKey);

    bool registerAction(const QString& id,
                        const QString& category,
                        const QString& description,
                        const QKeySequence& defaultKey,
                        QAction* action);

    bool registerShortcut(const QString& id,
                          const QString& category,
                          const QString& description,
                          const QKeySequence& defaultKey,
                          QWidget* parent,
                          QObject* receiver,
                          const char* slot);

    bool registerPluginAction(const QString& pluginId,
                              const QString& actionId,
                              const QString& category,
                              const QString& description,
                              const QKeySequence& defaultKey,
                              QAction* action);

    bool registerPluginShortcut(const QString& pluginId,
                                const QString& actionId,
                                const QString& category,
                                const QString& description,
                                const QKeySequence& defaultKey,
                                QWidget* parent,
                                QObject* receiver,
                                const char* slot);

    void unregisterPluginShortcuts(const QString& pluginId);
    void unregisterPrefix(const QString& idPrefix);

    bool updateShortcut(const QString& id, const QKeySequence& newKey);
    QString checkConflict(const QKeySequence& key,
                          const QString& ignoreId = QString()) const;

    QVector<ShortcutItem> allShortcuts() const;

    void resetToDefault(const QString& id = QString());
    void resetAllToDefault();
    void registerDefaultAppShortcuts();

signals:
    void shortcutChanged(const QString& id, const QKeySequence& newKey);

private:
    bool registerItem(const QString& id,
                      const QString& category,
                      const QString& description,
                      const QKeySequence& defaultKey,
                      QAction* action,
                      QShortcut* shortcut);
    void applyBindings(ShortcutItem& item) const;
    QString composePluginShortcutId(const QString& pluginId,
                                    const QString& actionId) const;

    static QString normalizeId(const QString& id);
    static QString normalizePluginName(const QString& pluginId);
    static QString toPortableText(const QKeySequence& key);
    static QKeySequence fromPortableText(const QString& text);

private:
    SettingsManager* m_settingsManager {nullptr};
    QMap<QString, ShortcutItem> m_shortcuts;
    QMap<QString, QString> m_persistedShortcuts;
};

}  // namespace iqtools::core
