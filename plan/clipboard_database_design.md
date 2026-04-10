# IQtools 剪贴板历史记录 SQLite 数据库设计补充

**版本**：v2.0（包含数据库存储）  
**日期**：2026-04-10  
**状态**：更新  

---

## 一、概述

本文档补充 `clipboard_implementation_plan.md` 中的历史记录存储设计，将内存存储升级为 **SQLite 数据库持久化方案**，支持：

- ✅ 完整的持久化历史记录
- ✅ 快速查找与过滤
- ✅ 可配置的保存周期（按天数设置过期策略）
- ✅ 手动备份与恢复
- ✅ 数据库清理与维护
- ✅ 线程安全的并发访问

---

## 二、架构设计

### 2.1 存储层架构

```
┌─────────────────────────────────────────────────────────┐
│ ClipboardService (业务逻辑)                             │
└─────────────────────────────────────────────────────────┘
                        │
        ┌───────────────┴───────────────┐
        │                               │
┌───────▼───────────────────┐  ┌────────▼────────────────────┐
│ ClipboardHistoryManager   │  │ ClipboardMonitor            │
│ (历史记录管理)             │  │ (监听与同步)                │
│ - 查询                     │  │                             │
│ - 插入                     │  │                             │
│ - 删除                     │  │                             │
│ - 过期清理                 │  │                             │
└───────┬───────────────────┘  └────────┬────────────────────┘
        │                               │
        └───────────────┬───────────────┘
                        │
        ┌───────────────▼─────────────────┐
        │  ClipboardDatabase              │
        │  (数据库抽象层)                 │
        │  - 事务管理                     │
        │  - 连接池                       │
        │  - 迁移                         │
        └───────────────┬─────────────────┘
                        │
        ┌───────────────▼─────────────────┐
        │  SQLite Database                │
        │  - clipboard_history.db         │
        │  - 5 张表（见 2.2 章节）        │
        └─────────────────────────────────┘
```

### 2.2 数据库表设计

#### 表 1：clipboard_items（剪贴板项目）

```sql
CREATE TABLE clipboard_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    -- 基本信息
    content_type TEXT NOT NULL,          -- 'text' / 'image' / 'files' / 'html'
    mime_type TEXT,                       -- MIME 类型，如 'text/plain'
    content_hash TEXT UNIQUE NOT NULL,   -- 内容唯一标识（SHA256）
    
    -- 内容存储
    text_content TEXT,                    -- 文本内容（最大 1MB）
    binary_data BLOB,                     -- 二进制数据（图片等，最大 50MB）
    file_list TEXT,                       -- 文件列表（JSON 数组，逗号分隔）
    
    -- 元数据
    size_bytes INTEGER,                   -- 内容大小（字节）
    thumbnail_path TEXT,                  -- 图片缩略图路径（可选）
    
    -- 时间戳
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,  -- 创建时间
    accessed_at DATETIME DEFAULT CURRENT_TIMESTAMP, -- 最后访问时间
    
    -- 标记与分类
    is_pinned BOOLEAN DEFAULT 0,          -- 是否固定
    tags TEXT,                            -- 标签（JSON 数组）
    notes TEXT,                           -- 用户备注
    
    -- 索引字段
    expire_at DATETIME,                   -- 过期时间（基于配置的天数）
    is_deleted BOOLEAN DEFAULT 0          -- 软删除标记
);

-- 索引优化查询
CREATE INDEX idx_clipboard_items_created_at ON clipboard_items(created_at DESC);
CREATE INDEX idx_clipboard_items_content_type ON clipboard_items(content_type);
CREATE INDEX idx_clipboard_items_tags ON clipboard_items(tags);
CREATE INDEX idx_clipboard_items_is_pinned ON clipboard_items(is_pinned);
CREATE INDEX idx_clipboard_items_expire_at ON clipboard_items(expire_at);
CREATE UNIQUE INDEX idx_clipboard_items_content_hash ON clipboard_items(content_hash);
```

#### 表 2：clipboard_history（访问历史）

```sql
CREATE TABLE clipboard_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    item_id INTEGER NOT NULL,
    
    -- 访问信息
    action TEXT NOT NULL,                -- 'read' / 'write' / 'delete' / 'restore'
    accessed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    -- 上下文
    context TEXT,                         -- 访问上下文（如 '截图工具' 等）
    source_app TEXT,                      -- 源应用（可选）
    
    FOREIGN KEY (item_id) REFERENCES clipboard_items(id) ON DELETE CASCADE
);

CREATE INDEX idx_clipboard_history_item_id ON clipboard_history(item_id);
CREATE INDEX idx_clipboard_history_accessed_at ON clipboard_history(accessed_at DESC);
CREATE INDEX idx_clipboard_history_action ON clipboard_history(action);
```

#### 表 3：clipboard_settings（设置）

```sql
CREATE TABLE clipboard_settings (
    id INTEGER PRIMARY KEY CHECK (id = 1),  -- 单行表
    
    -- 历史记录设置
    retention_days INTEGER DEFAULT 30,     -- 保留天数
    max_items INTEGER DEFAULT 1000,        -- 最大项目数
    max_memory_mb INTEGER DEFAULT 500,     -- 最大数据库大小（MB）
    
    -- 功能开关
    enable_history BOOLEAN DEFAULT 1,      -- 启用历史记录
    enable_auto_cleanup BOOLEAN DEFAULT 1, -- 启用自动清理
    enable_image_cache BOOLEAN DEFAULT 1,  -- 启用图片缓存
    
    -- 备份设置
    last_backup_at DATETIME,               -- 最后备份时间
    backup_directory TEXT,                 -- 备份目录
    auto_backup_interval_days INTEGER DEFAULT 7,  -- 自动备份间隔
    
    -- 统计信息
    total_items_count INTEGER DEFAULT 0,
    database_size_bytes INTEGER DEFAULT 0,
    
    -- 更新时间戳
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

#### 表 4：clipboard_searches（保存的搜索）

```sql
CREATE TABLE clipboard_searches (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    -- 搜索信息
    name TEXT NOT NULL,                    -- 搜索名称
    description TEXT,                      -- 描述
    query TEXT NOT NULL,                   -- 搜索条件（JSON）
    
    -- 时间戳
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    -- 使用统计
    usage_count INTEGER DEFAULT 0,         -- 使用次数
    last_used_at DATETIME
);

CREATE INDEX idx_clipboard_searches_name ON clipboard_searches(name);
CREATE INDEX idx_clipboard_searches_created_at ON clipboard_searches(created_at DESC);
```

#### 表 5：clipboard_backups（备份记录）

```sql
CREATE TABLE clipboard_backups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    -- 备份信息
    backup_name TEXT NOT NULL,             -- 备份名称
    backup_file_path TEXT NOT NULL UNIQUE, -- 备份文件路径
    
    -- 元数据
    item_count INTEGER,                    -- 包含的项目数
    database_size_bytes INTEGER,           -- 备份时数据库大小
    description TEXT,                      -- 备份描述
    
    -- 时间戳
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    restored_at DATETIME,                  -- 最后一次恢复的时间
    
    -- 标记
    is_auto_backup BOOLEAN DEFAULT 0,      -- 是否自动备份
    tags TEXT                              -- 备份标签（JSON）
);

CREATE INDEX idx_clipboard_backups_created_at ON clipboard_backups(created_at DESC);
CREATE INDEX idx_clipboard_backups_is_auto_backup ON clipboard_backups(is_auto_backup);
```

---

## 三、核心接口设计

### 3.1 数据库连接管理

**文件**：`src/infra/storage/clipboard_database.h/cpp`

```cpp
namespace iqtools::infra::storage {

class ClipboardDatabase {
public:
    ClipboardDatabase(const QString& dbPath, QObject* parent = nullptr);
    ~ClipboardDatabase();

    // 连接管理
    bool open();
    void close();
    bool isOpen() const;
    QString lastError() const;

    // 数据库初始化与迁移
    bool initialize();
    bool migrate();
    int currentSchemaVersion() const;

    // 事务管理
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    class Transaction {  // RAII 事务管理
    public:
        Transaction(ClipboardDatabase* db);
        ~Transaction();
        bool commit();
    private:
        ClipboardDatabase* m_db;
        bool m_committed;
    };

    // 低级查询接口
    QSqlQueryModel* executeSelect(const QString& query);
    bool executeInsert(const QString& query, const QVariantMap& bindings);
    bool executeUpdate(const QString& query, const QVariantMap& bindings);
    bool executeDelete(const QString& query, const QVariantMap& bindings);

    // 获取数据库对象
    QSqlDatabase& database();
    const QSqlDatabase& database() const;

    // 维护
    bool vacuum();
    qint64 getDatabaseSize() const;
    bool optimize();

private:
    QString m_dbPath;
    QSqlDatabase m_database;
    QString m_lastError;
    QMutex m_dbMutex;  // 线程安全
};

}  // namespace iqtools::infra::storage
```

### 3.2 历史记录管理器

**文件**：`src/core/clipboard/clipboard_history_manager.h/cpp`

```cpp
namespace iqtools::core {

struct ClipboardHistoryQuery {
    // 搜索条件
    QString textContent;              // 文本内容搜索（模糊匹配）
    QStringList contentTypes;         // 内容类型过滤
    QStringList tags;                 // 标签过滤
    
    // 时间范围
    QDateTime fromTime;
    QDateTime toTime;
    
    // 排序与分页
    enum SortBy { ByNewest, ByOldest, BySize, ByAccessCount };
    SortBy sortBy = ByNewest;
    int offset = 0;
    int limit = 50;
    
    // 其他
    bool pinnedOnly = false;
    bool excludeDeleted = true;
};

struct ClipboardHistoryResult {
    QVector<ClipboardItem> items;
    int totalCount;
    int returnedCount;
};

class ClipboardHistoryManager : public QObject {
    Q_OBJECT

public:
    ClipboardHistoryManager(infra::storage::ClipboardDatabase* db,
                            LogService* logService = nullptr,
                            QObject* parent = nullptr);

    // 添加项目到历史
    bool addItem(const ClipboardItem& item);

    // 查询历史
    ClipboardHistoryResult queryHistory(const ClipboardHistoryQuery& query) const;
    
    // 快速查询
    QVector<ClipboardItem> getRecentItems(int limit = 20) const;
    QVector<ClipboardItem> getItemsByTag(const QString& tag, int limit = 50) const;
    ClipboardItem getItemById(int itemId) const;
    ClipboardItem getItemByHash(const QString& hash) const;

    // 修改历史
    bool pinItem(int itemId, bool pinned);
    bool addTags(int itemId, const QStringList& tags);
    bool removeTags(int itemId, const QStringList& tags);
    bool setNotes(int itemId, const QString& notes);
    bool deleteItem(int itemId, bool hardDelete = false);

    // 过期清理
    bool cleanupExpired();
    int getExpiredItemCount() const;

    // 访问记录
    bool recordAccess(int itemId, const QString& action, const QString& context = "");
    QVector<QPair<QDateTime, QString>> getAccessHistory(int itemId) const;

    // 统计信息
    int getTotalItemCount() const;
    qint64 getTotalSize() const;
    QMap<QString, int> getContentTypeStats() const;

signals:
    void itemAdded(const ClipboardItem& item);
    void itemDeleted(int itemId);
    void itemUpdated(int itemId);
    void cleanupStarted();
    void cleanupFinished(int itemsRemoved);

private:
    QString buildQueryString(const ClipboardHistoryQuery& query) const;
    QString createSearchPredicate(const QString& textContent) const;

    infra::storage::ClipboardDatabase* m_db;
    LogService* m_logService;
    mutable QMutex m_accessMutex;  // 线程安全
};

}  // namespace iqtools::core
```

### 3.3 配置管理器

**文件**：`src/core/clipboard/clipboard_config_manager.h/cpp`

```cpp
namespace iqtools::core {

struct ClipboardHistoryConfig {
    // 保留策略
    int retentionDays {30};           // 保留天数
    int maxItems {1000};              // 最大项目数
    int maxMemoryMb {500};            // 最大数据库大小

    // 功能开关
    bool enableHistory {true};
    bool enableAutoCleanup {true};
    bool enableImageCache {true};

    // 备份设置
    QString backupDirectory;
    int autoBackupIntervalDays {7};
    bool enableAutoBackup {false};

    // 性能设置
    int queryTimeoutMs {5000};
    int batchInsertSize {100};
};

class ClipboardConfigManager : public QObject {
    Q_OBJECT

public:
    ClipboardConfigManager(infra::storage::ClipboardDatabase* db,
                           QObject* parent = nullptr);

    // 读取配置
    ClipboardHistoryConfig loadConfig() const;
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    // 保存配置
    bool saveConfig(const ClipboardHistoryConfig& config);
    bool setValue(const QString& key, const QVariant& value);

    // 便利方法
    int getRetentionDays() const;
    void setRetentionDays(int days);
    
    int getMaxItems() const;
    void setMaxItems(int count);

    QString getBackupDirectory() const;
    void setBackupDirectory(const QString& path);

    bool isAutoBackupEnabled() const;
    void setAutoBackupEnabled(bool enabled);

signals:
    void configChanged(const ClipboardHistoryConfig& config);

private:
    infra::storage::ClipboardDatabase* m_db;
    mutable QMutex m_configMutex;
};

}  // namespace iqtools::core
```

### 3.4 备份管理器

**文件**：`src/core/clipboard/clipboard_backup_manager.h/cpp`

```cpp
namespace iqtools::core {

struct BackupMetadata {
    QString backupName;
    QString backupFilePath;
    int itemCount;
    qint64 databaseSizeBytes;
    QString description;
    QDateTime createdAt;
    bool isAutoBackup;
    QStringList tags;
};

class ClipboardBackupManager : public QObject {
    Q_OBJECT

public:
    ClipboardBackupManager(infra::storage::ClipboardDatabase* db,
                          LogService* logService = nullptr,
                          QObject* parent = nullptr);

    // 备份操作
    bool createBackup(const QString& name, const QString& description = "",
                     const QStringList& tags = {});
    
    bool createAutoBackup();

    // 恢复操作
    bool restoreBackup(const QString& backupFilePath);
    bool restoreBackupById(int backupId);

    // 查询备份
    QVector<BackupMetadata> getBackups() const;
    QVector<BackupMetadata> getAutoBackups() const;
    BackupMetadata getBackupInfo(int backupId) const;

    // 备份管理
    bool deleteBackup(int backupId);
    bool deleteOldBackups(int keepCount = 5);
    
    // 备份导出
    bool exportBackup(int backupId, const QString& exportPath);
    bool importBackup(const QString& importPath, const QString& backupName);

    // 统计
    int getBackupCount() const;
    qint64 getTotalBackupSize() const;

signals:
    void backupStarted(const QString& name);
    void backupFinished(bool success, const QString& backupFilePath);
    void backupProgress(int percentage);
    void restoreStarted(const QString& backupName);
    void restoreFinished(bool success);

private:
    bool checkBackupDirectory();
    QString generateBackupPath(const QString& name) const;

    infra::storage::ClipboardDatabase* m_db;
    LogService* m_logService;
    QString m_backupDirectory;
    mutable QMutex m_backupMutex;
};

}  // namespace iqtools::core
```

---

## 四、集成到 ClipboardService

### 4.1 修改 ClipboardService 构造函数与初始化

```cpp
namespace iqtools::core {

class ClipboardService : public IClipboardService {
    Q_OBJECT

public:
    // 新的构造函数，包含数据库支持
    ClipboardService(
        std::unique_ptr<infra::platform::IPlatformClipboard> platformImpl,
        std::unique_ptr<infra::storage::ClipboardDatabase> database,
        LogService* logService = nullptr,
        QObject* parent = nullptr);

    // ... 原有接口方法 ...

    // 新增历史记录相关方法
    ClipboardHistoryResult searchHistory(const ClipboardHistoryQuery& query) const;
    QVector<ClipboardItem> getRecentHistory(int limit = 20) const;
    bool restoreFromHistory(const ClipboardItem& item);
    bool pinHistoryItem(int itemId, bool pinned);
    bool deleteHistoryItem(int itemId, bool hardDelete = false);

    // 配置管理
    void setHistoryConfig(const ClipboardHistoryConfig& config);
    ClipboardHistoryConfig getHistoryConfig() const;

    // 备份管理
    bool createBackup(const QString& name, const QString& description = "");
    bool restoreBackup(const QString& backupPath);
    QVector<BackupMetadata> getBackupList() const;

signals:
    // 新增信号
    void historyItemAdded(const ClipboardItem& item);
    void historyItemDeleted(int itemId);
    void backupCreated(const QString& backupPath);

private slots:
    void onHistoryCleanupNeeded();
    void onAutoBackupNeeded();

private:
    std::unique_ptr<ClipboardHistoryManager> m_historyManager;
    std::unique_ptr<ClipboardConfigManager> m_configManager;
    std::unique_ptr<ClipboardBackupManager> m_backupManager;
    QTimer* m_cleanupTimer;
    QTimer* m_backupTimer;
};

}
```

---

## 五、UI 层设计补充

### 5.1 ClipboardPage 页面结构

**文件**：`src/app/shell/qml/pages/ClipboardPage.qml`

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root
    title: qsTr("Clipboard Manager")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // 顶部工具栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search history...")
                onEditingFinished: performSearch()
            }

            ComboBox {
                id: typeFilter
                model: ["All", "Text", "Image", "Files", "Html"]
                onCurrentValueChanged: performSearch()
            }

            Button {
                text: qsTr("Search")
                onClicked: performSearch()
            }

            Button {
                text: qsTr("Clear History")
                onClicked: clearHistoryDialog.open()
            }
        }

        // 标签页：历史记录 / 配置 / 备份
        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton { text: qsTr("History") }
            TabButton { text: qsTr("Settings") }
            TabButton { text: qsTr("Backups") }
        }

        // 标签页内容
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // 标签 1：历史记录
            Rectangle {
                ListView {
                    id: historyList
                    anchors.fill: parent
                    model: ClipboardHistoryModel { }
                    spacing: 4

                    delegate: Rectangle {
                        width: historyList.width
                        height: 60
                        border.color: "#ddd"
                        border.width: 1
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            // 内容类型图标
                            Rectangle {
                                width: 40
                                height: 40
                                color: colorForType(model.contentType)
                                radius: 4

                                Text {
                                    anchors.centerIn: parent
                                    text: iconForType(model.contentType)
                                    color: "white"
                                    font.bold: true
                                }
                            }

                            // 内容预览
                            Column {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: model.preview
                                    elide: Text.ElideRight
                                    font.weight: Font.Medium
                                }

                                Text {
                                    text: model.timestamp
                                    font.pixelSize: 10
                                    color: "#666"
                                }
                            }

                            // 操作按钮
                            RowLayout {
                                spacing: 4

                                Button {
                                    text: "📌"
                                    flat: true
                                    onClicked: controller.pinItem(model.id, !model.isPinned)
                                }

                                Button {
                                    text: "复制"
                                    onClicked: controller.copyToClipboard(model.id)
                                }

                                Button {
                                    text: "删除"
                                    onClicked: controller.deleteHistoryItem(model.id)
                                }
                            }
                        }

                        // 长按显示详情
                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: itemContextMenu.popup()

                            Menu {
                                id: itemContextMenu
                                MenuItem {
                                    text: qsTr("View Details")
                                    onTriggered: showItemDetails(model)
                                }
                                MenuItem {
                                    text: qsTr("Add Tags...")
                                    onTriggered: tagDialog.itemId = model.id; tagDialog.open()
                                }
                                MenuItem {
                                    text: qsTr("Add Notes...")
                                    onTriggered: notesDialog.itemId = model.id; notesDialog.open()
                                }
                                MenuSeparator { }
                                MenuItem {
                                    text: qsTr("Delete Permanently")
                                    onTriggered: controller.deleteHistoryItemPermanently(model.id)
                                }
                            }
                        }
                    }
                }

                // 分页按钮
                RowLayout {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.margins: 8

                    Button {
                        text: qsTr("Previous")
                        onClicked: historyList.model.previousPage()
                    }

                    Text {
                        text: qsTr("Page %1 of %2").arg(
                            historyList.model.currentPage).arg(
                            historyList.model.totalPages)
                    }

                    Button {
                        text: qsTr("Next")
                        onClicked: historyList.model.nextPage()
                    }
                }
            }

            // 标签 2：设置
            ColumnLayout {
                spacing: 12
                anchors.margins: 16

                GroupBox {
                    title: qsTr("Retention Policy")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Label { text: qsTr("Retention Days:") }
                            SpinBox {
                                from: 1
                                to: 365
                                value: controller.retentionDays
                                onValueChanged: controller.setRetentionDays(value)
                            }
                        }

                        RowLayout {
                            Label { text: qsTr("Max Items:") }
                            SpinBox {
                                from: 10
                                to: 10000
                                value: controller.maxItems
                                onValueChanged: controller.setMaxItems(value)
                            }
                        }

                        RowLayout {
                            Label { text: qsTr("Max Database Size (MB):") }
                            SpinBox {
                                from: 10
                                to: 5000
                                value: controller.maxMemoryMb
                                onValueChanged: controller.setMaxMemoryMb(value)
                            }
                        }
                    }
                }

                GroupBox {
                    title: qsTr("Features")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        CheckBox {
                            text: qsTr("Enable History")
                            checked: controller.enableHistory
                            onCheckedChanged: controller.setEnableHistory(checked)
                        }

                        CheckBox {
                            text: qsTr("Enable Auto Cleanup")
                            checked: controller.enableAutoCleanup
                            onCheckedChanged: controller.setEnableAutoCleanup(checked)
                        }

                        CheckBox {
                            text: qsTr("Enable Image Cache")
                            checked: controller.enableImageCache
                            onCheckedChanged: controller.setEnableImageCache(checked)
                        }
                    }
                }

                GroupBox {
                    title: qsTr("Backup Settings")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Label { text: qsTr("Backup Directory:") }
                            TextField {
                                Layout.fillWidth: true
                                text: controller.backupDirectory
                                readOnly: true
                            }
                            Button {
                                text: qsTr("Browse...")
                                onClicked: folderDialog.open()
                            }
                        }

                        CheckBox {
                            text: qsTr("Auto Backup Every")
                            checked: controller.enableAutoBackup
                            onCheckedChanged: controller.setEnableAutoBackup(checked)
                        }

                        RowLayout {
                            enabled: controller.enableAutoBackup
                            SpinBox {
                                from: 1
                                to: 30
                                value: controller.autoBackupIntervalDays
                                onValueChanged: controller.setAutoBackupIntervalDays(value)
                            }
                            Label { text: qsTr("days") }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // 标签 3：备份
            ColumnLayout {
                spacing: 12
                anchors.margins: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: backupNameField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Backup name...")
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Description (optional)...")
                    }

                    Button {
                        text: qsTr("Create Backup")
                        onClicked: controller.createBackup(
                            backupNameField.text,
                            descriptionField.text)
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: BackupListModel { }
                    spacing: 4

                    delegate: Rectangle {
                        width: parent.width
                        height: 80
                        border.color: "#ddd"
                        border.width: 1
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            RowLayout {
                                Text {
                                    text: model.backupName
                                    font.weight: Font.Bold
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: model.createdAt
                                    font.pixelSize: 10
                                    color: "#666"
                                }
                            }

                            Text {
                                text: qsTr("Items: %1 | Size: %2 MB").arg(
                                    model.itemCount).arg(
                                    (model.databaseSizeBytes / 1024 / 1024).toFixed(2))
                                font.pixelSize: 10
                                color: "#999"
                            }

                            RowLayout {
                                spacing: 4

                                Button {
                                    text: qsTr("Restore")
                                    onClicked: controller.restoreBackup(model.id)
                                }

                                Button {
                                    text: qsTr("Export")
                                    onClicked: controller.exportBackup(model.id)
                                }

                                Button {
                                    text: qsTr("Delete")
                                    onClicked: deleteBackupDialog.backupId = model.id; deleteBackupDialog.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 对话框
    Dialog {
        id: clearHistoryDialog
        title: qsTr("Clear History")
        Button { text: qsTr("Cancel") }
        Button { text: qsTr("Clear"); onClicked: controller.clearHistory() }
    }

    Dialog {
        id: tagDialog
        property int itemId: -1
        title: qsTr("Add Tags")
        TextField { id: tagInput; placeholderText: qsTr("Tags (comma-separated)...") }
        Button { text: qsTr("Add"); onClicked: controller.addTags(tagDialog.itemId, tagInput.text) }
    }

    Dialog {
        id: notesDialog
        property int itemId: -1
        title: qsTr("Add Notes")
        TextField { id: noteInput; placeholderText: qsTr("Notes...") }
        Button { text: qsTr("Save"); onClicked: controller.setNotes(notesDialog.itemId, noteInput.text) }
    }
}
```

### 5.2 ClipboardController 补充方法

**文件**：`src/app/shell/bridge/clipboard_controller.h/cpp`

```cpp
namespace iqtools::app::bridge {

class ClipboardController : public QObject {
    Q_OBJECT

    // 属性
    Q_PROPERTY(int retentionDays READ retentionDays WRITE setRetentionDays NOTIFY configChanged)
    Q_PROPERTY(int maxItems READ maxItems WRITE setMaxItems NOTIFY configChanged)
    Q_PROPERTY(bool enableHistory READ enableHistory WRITE setEnableHistory NOTIFY configChanged)
    Q_PROPERTY(QString backupDirectory READ backupDirectory WRITE setBackupDirectory NOTIFY configChanged)
    Q_PROPERTY(bool enableAutoBackup READ enableAutoBackup WRITE setEnableAutoBackup NOTIFY configChanged)

public:
    // ... 现有方法 ...

    // 历史记录查询
    Q_INVOKABLE QStringList searchHistory(const QString& text, const QString& contentType = "");
    Q_INVOKABLE QString getHistoryItemContent(int itemId);
    Q_INVOKABLE bool copyHistoryItemToClipboard(int itemId);

    // 历史记录管理
    Q_INVOKABLE bool pinHistoryItem(int itemId, bool pinned);
    Q_INVOKABLE bool addTagsToItem(int itemId, const QString& tags);
    Q_INVOKABLE bool deleteHistoryItem(int itemId, bool hardDelete = false);
    Q_INVOKABLE bool clearHistory();

    // 配置管理
    Q_INVOKABLE void setRetentionDays(int days);
    Q_INVOKABLE int retentionDays() const;
    Q_INVOKABLE void setMaxItems(int count);
    Q_INVOKABLE int maxItems() const;
    Q_INVOKABLE void setEnableHistory(bool enabled);
    Q_INVOKABLE bool enableHistory() const;
    Q_INVOKABLE void setBackupDirectory(const QString& path);
    Q_INVOKABLE QString backupDirectory() const;

    // 备份管理
    Q_INVOKABLE bool createBackup(const QString& name, const QString& description = "");
    Q_INVOKABLE bool restoreBackup(int backupId);
    Q_INVOKABLE bool exportBackup(int backupId, const QString& exportPath);
    Q_INVOKABLE QStringList getBackupList();
    Q_INVOKABLE bool deleteBackup(int backupId);

signals:
    void configChanged();
    void backupCompleted(bool success);
    void restoreCompleted(bool success);

private:
    iqtools::core::ClipboardService* m_service;
};

}  // namespace iqtools::app::bridge
```

---

## 六、实施步骤补充

### 阶段 1 补充：数据库基础设施（第1-2周内）

#### 步骤 1.4：实现数据库连接与迁移
**文件**：
- `src/infra/storage/clipboard_database.h/cpp`

**工作内容**：
- 创建 SQLite 数据库连接
- 实现 5 张表的 SQL 定义
- 实现数据库迁移机制
- 实现事务管理（RAII）
- 错误处理与日志

**验收标准**：
- 数据库初始化成功
- 表结构正确
- 单元测试覆盖迁移场景

---

### 阶段 2 补充：历史记录管理（第3-4周内）

#### 步骤 3.4：实现 ClipboardHistoryManager
**文件**：
- `src/core/clipboard/clipboard_history_manager.h/cpp`

**工作内容**：
- 实现完整的 CRUD 操作
- 实现复杂查询（搜索、过滤、排序、分页）
- 实现过期清理
- 实现访问记录
- 实现统计功能

**验收标准**：
- 所有查询方法正常工作
- 单元测试覆盖 >80%
- 性能测试：查询 <500ms、插入 <100ms

---

#### 步骤 3.5：实现 ClipboardConfigManager
**文件**：
- `src/core/clipboard/clipboard_config_manager.h/cpp`

**工作内容**：
- 实现配置的读写
- 实现配置持久化
- 实现配置变更通知
- 集成 ClipboardDatabase

**验收标准**：
- 配置能正确保存与读取
- 配置变更信号正确发送

---

#### 步骤 3.6：实现 ClipboardBackupManager
**文件**：
- `src/core/clipboard/clipboard_backup_manager.h/cpp`

**工作内容**：
- 实现数据库备份
- 实现备份恢复
- 实现备份导入导出
- 实现自动备份策略
- 实现备份清理

**验收标准**：
- 备份与恢复功能正常
- 导入导出兼容性验证
- 单元测试通过

---

### 阶段 4 补充：UI 扩展（第4-5周内）

#### 步骤 4.4：创建 ClipboardPage 完整实现
**文件**：
- `src/app/shell/qml/pages/ClipboardPage.qml`
- 相关数据模型

**工作内容**：
- 实现历史记录列表视图
- 实现搜索与过滤
- 实现配置面板
- 实现备份面板
- 实现各类对话框

**验收标准**：
- UI 完整可用
- 与 Controller 完全集成
- 响应式设计验证

---

#### 步骤 4.5：扩展 ClipboardController
**文件**：
- `src/app/shell/bridge/clipboard_controller.h/cpp`

**工作内容**：
- 添加历史查询方法
- 添加配置管理方法
- 添加备份管理方法
- 实现信号与槽

**验收标准**：
- 所有 Q_INVOKABLE 方法正常工作
- 集成测试通过

---

## 七、CMakeLists.txt 修改

```cmake
# 在现有 IQTOOLS_SOURCES 中添加：

set(IQTOOLS_SOURCES
    # ... 现有源文件 ...
    
    # Clipboard 数据库层
    src/infra/storage/clipboard_database.cpp
    
    # Clipboard Core 层
    src/core/clipboard/clipboard_service.cpp
    src/core/clipboard/clipboard_monitor.cpp
    src/core/clipboard/clipboard_history_manager.cpp
    src/core/clipboard/clipboard_config_manager.cpp
    src/core/clipboard/clipboard_backup_manager.cpp
    
    # Clipboard Bridge 层
    src/app/shell/bridge/clipboard_controller.cpp
)

set(IQTOOLS_HEADERS
    # ... 现有头文件 ...
    
    # Clipboard 数据库层
    src/infra/storage/clipboard_database.h
    
    # Clipboard Core 层
    src/core/clipboard/clipboard_history_manager.h
    src/core/clipboard/clipboard_config_manager.h
    src/core/clipboard/clipboard_backup_manager.h
    
    # Clipboard Bridge 层
    src/app/shell/bridge/clipboard_controller.h
)

# Qt 依赖中添加
find_package(Qt6 REQUIRED COMPONENTS 
    # ... 现有组件 ...
    Sql          # SQLite 支持
)

target_link_libraries(IQtools PRIVATE
    # ... 现有库 ...
    Qt6::Sql
)

# QML 模块中添加
qt_add_qml_module(IQtools
    # ... 现有 QML 文件 ...
    src/app/shell/qml/pages/ClipboardPage.qml
)
```

---

## 八、数据库性能优化

### 8.1 查询优化

```cpp
// 使用参数化查询避免 SQL 注入
QString query = "SELECT * FROM clipboard_items WHERE text_content LIKE ? "
                "AND content_type = ? ORDER BY created_at DESC LIMIT ?";
QSqlQuery q;
q.prepare(query);
q.addBindValue("%" + searchText + "%");
q.addBindValue(contentType);
q.addBindValue(limit);
q.exec();

// 使用事务批量插入以提高性能
ClipboardDatabase::Transaction txn(&db);
for (const auto& item : items) {
    historyManager->addItem(item);  // 在事务内
}
txn.commit();
```

### 8.2 缓存策略

```cpp
// 最近访问的项目缓存
static const int CACHE_SIZE = 50;
QList<ClipboardItem> m_recentCache;

// 定期清理过期缓存
void ClipboardHistoryManager::updateCache() {
    if (m_recentCache.size() > CACHE_SIZE) {
        m_recentCache.erase(m_recentCache.begin(),
                           m_recentCache.begin() + (CACHE_SIZE / 2));
    }
}
```

### 8.3 数据库维护

```cpp
// 定期 VACUUM 整理数据库文件
bool ClipboardDatabase::optimize() {
    return executeInsert("VACUUM", {});
}

// 定期 ANALYZE 更新统计信息
bool ClipboardDatabase::analyze() {
    return executeInsert("ANALYZE", {});
}

// 监听定期触发（如周一清晨）
void ClipboardService::onMaintenanceTimer() {
    m_database->vacuum();
    m_database->analyze();
}
```

---

## 九、完整文件清单（数据库补充）

### 新增文件

```
src/
├── infra/
│   └── storage/
│       └── clipboard_database.h/cpp
├── core/
│   └── clipboard/
│       ├── clipboard_history_manager.h/cpp
│       ├── clipboard_config_manager.h/cpp
│       └── clipboard_backup_manager.h/cpp
└── app/
    └── shell/
        ├── bridge/
        │   └── clipboard_controller.h/cpp （扩展）
        └── qml/
            └── pages/
                └── ClipboardPage.qml （完整）

tests/
├── core/
│   ├── test_clipboard_history_manager.cpp
│   ├── test_clipboard_config_manager.cpp
│   └── test_clipboard_backup_manager.cpp
└── infra/
    └── test_clipboard_database.cpp
```

---

## 十、测试用例补充

### 单元测试场景

```cpp
// 历史记录测试
TEST(ClipboardHistoryManager, AddAndRetrieve) {
    // 添加项目
    ClipboardItem item = createTestItem();
    ASSERT_TRUE(manager.addItem(item));
    
    // 查询
    auto retrieved = manager.getItemByHash(item.hash);
    ASSERT_EQ(retrieved.data, item.data);
}

TEST(ClipboardHistoryManager, SearchWithFilter) {
    // 添加多个项目
    addTestItems(50);
    
    // 搜索
    ClipboardHistoryQuery query;
    query.textContent = "test";
    query.limit = 10;
    auto result = manager.queryHistory(query);
    ASSERT_LE(result.returnedCount, 10);
}

TEST(ClipboardHistoryManager, CleanupExpired) {
    // 添加已过期项目
    addExpiredItems(20);
    
    // 清理
    int removed = manager.cleanupExpired();
    ASSERT_EQ(removed, 20);
}

// 备份测试
TEST(ClipboardBackupManager, CreateAndRestore) {
    // 创建备份
    ASSERT_TRUE(manager.createBackup("test_backup"));
    
    // 修改数据
    addTestItems(50);
    
    // 恢复备份
    ASSERT_TRUE(manager.restoreBackup("test_backup.qdb"));
    
    // 验证恢复
    ASSERT_EQ(manager.getTotalItemCount(), 0);
}

// 性能测试
PERFORMANCE_TEST(ClipboardHistoryManager, BulkInsert) {
    auto items = generateTestItems(1000);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    ClipboardDatabase::Transaction txn(&db);
    for (const auto& item : items) {
        manager.addItem(item);
    }
    txn.commit();
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    ASSERT_LT(duration.count(), 5000);  // 5 秒内完成
}
```

---

## 十一、风险与缓解补充

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 数据库腐坏 | 高 | 低 | 自动备份、定期 VACUUM、数据库完整性检查 |
| 查询性能退化 | 中 | 中 | 索引优化、缓存、定期 ANALYZE |
| 备份文件丢失 | 高 | 低 | 多备份位置、加密、定期导出 |
| 并发写入冲突 | 中 | 中 | 事务管理、互斥锁、连接池 |
| 数据库磁盘满 | 中 | 低 | 自动清理、磁盘检查、用户提示 |

---

## 十二、配置示例

### 默认配置文件

**位置**：`~/.config/IQtools/clipboard.conf`

```yaml
# 历史记录保留设置
clipboard:
  retention_days: 30           # 保留 30 天
  max_items: 1000             # 最多 1000 项
  max_memory_mb: 500          # 数据库最大 500MB
  
  # 功能开关
  enable_history: true
  enable_auto_cleanup: true
  enable_image_cache: true
  
  # 备份设置
  backup_directory: ~/.config/IQtools/backups
  enable_auto_backup: true
  auto_backup_interval_days: 7
  
  # 性能设置
  query_timeout_ms: 5000
  batch_insert_size: 100
```

---

## 十三、集成示例代码

```cpp
// application_bootstrap.cpp 中初始化数据库

void initializeClipboard(AppContext& context) {
    // 1. 创建数据库
    QString dbPath = getClipboardDbPath();
    auto database = std::make_unique<ClipboardDatabase>(dbPath);
    if (!database->open()) {
        qWarning() << "Failed to open clipboard database:" << database->lastError();
        return;
    }
    if (!database->initialize()) {
        qWarning() << "Failed to initialize clipboard database";
        return;
    }

    // 2. 创建各类管理器
    auto historyMgr = std::make_unique<ClipboardHistoryManager>(
        database.get(), logService);
    auto configMgr = std::make_unique<ClipboardConfigManager>(
        database.get());
    auto backupMgr = std::make_unique<ClipboardBackupManager>(
        database.get(), logService);

    // 3. 创建 ClipboardService
    auto platformImpl = createPlatformClipboard();
    auto clipboardService = std::make_unique<ClipboardService>(
        std::move(platformImpl),
        std::move(database),
        logService);

    // 4. 注册到 AppContext
    context.registerService("clipboard", clipboardService.get());
    
    // 5. 配置初始化
    auto config = configMgr->loadConfig();
    clipboardService->setHistoryConfig(config);
    
    // 6. 启动监听
    clipboardService->startMonitoring();
}
```

---

**编写人**：开发团队  
**最后更新**：2026-04-10  
**审核状态**：待审核  
**版本**：v2.0（包含 SQLite 数据库设计）
