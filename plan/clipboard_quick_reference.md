# 剪贴板功能实施快速参考卡

**打印版** | 快速查阅 | 2026-04-10

---

## 📋 实施阶段时间表

```
第 1-2 周    第 2-3 周      第 3-4 周       第 4-5 周      第 5-6 周
├─ 平台实现  ├─ Core 层     ├─ 集成        ├─ UI 开发      ├─ 测试优化
├─ 数据库    │ ├─ Service   │ ├─ AppContext ├─ ClipboardPage├─ 单元测试
└─ 工具函数  │ ├─ Monitor   │ └─ 初始化     └─ Controller   └─ 文档
            │ └─ Manager    
            └─ Config/Backup
```

---

## 🗂️ 关键文件速查

| 模块 | 文件 | 行数 | 优先级 |
|------|------|------|-------|
| **平台抽象** | `i_platform_clipboard.h` | 50 | 🔴 高 |
| | `clipboard_windows.cpp` | 200 | 🔴 高 |
| | `clipboard_linux.cpp` | 150 | 🔴 高 |
| | `clipboard_macos.cpp` | 150 | 🟡 中 |
| **数据库** | `clipboard_database.h/cpp` | 300 | 🔴 高 |
| **业务逻辑** | `clipboard_service.h/cpp` | 400 | 🔴 高 |
| | `clipboard_history_manager.h/cpp` | 500 | 🔴 高 |
| | `clipboard_backup_manager.h/cpp` | 300 | 🟡 中 |
| **UI 层** | `clipboard_controller.h/cpp` | 250 | 🔴 高 |
| | `ClipboardPage.qml` | 400 | 🟡 中 |

---

## 🗄️ 数据库 5 张表

```sql
clipboard_items          -- 主表，存储剪贴板项目
├── 字段：id, content_type, mime_type, text_content, 
│        binary_data, file_list, created_at, expire_at...
├── 索引：created_at, content_type, is_pinned, content_hash
└── 关键：content_hash UNIQUE（防重复）

clipboard_history       -- 访问日志
├── 字段：id, item_id, action, accessed_at
└── 索引：item_id, accessed_at, action

clipboard_settings      -- 配置（单行表）
├── 字段：retention_days, max_items, enable_history...
└── 用途：全局配置

clipboard_searches      -- 保存的搜索条件
├── 字段：id, name, query(JSON), usage_count...
└── 用途：快速搜索

clipboard_backups       -- 备份记录
├── 字段：id, backup_name, backup_file_path, item_count...
└── 用途：备份管理
```

---

## 🔧 核心类快速查阅

### ClipboardService（业务逻辑入口）

```cpp
// 核心方法
ClipboardItem getContent();              // 获取当前剪贴板
bool setText(const QString& text);       // 写入文本
bool setImage(const QPixmap& pixmap);    // 写入图片
QVector<ClipboardItem> getHistory(10);   // 获取历史

// 信号
void contentChanged(const ClipboardItem&);
void historyItemAdded(const ClipboardItem&);
void backupCreated(const QString&);
```

### ClipboardHistoryManager（历史查询）

```cpp
// 查询
ClipboardHistoryResult queryHistory(const ClipboardHistoryQuery&);
QVector<ClipboardItem> getRecentItems(int limit);
ClipboardItem getItemByHash(const QString& hash);

// 修改
bool addItem(const ClipboardItem&);
bool deleteItem(int id, bool hardDelete);
bool pinItem(int id, bool pinned);
bool addTags(int id, const QStringList&);
```

### ClipboardBackupManager（备份管理）

```cpp
// 操作
bool createBackup(const QString& name, const QString& desc);
bool restoreBackup(const QString& path);
bool exportBackup(int id, const QString& path);
QVector<BackupMetadata> getBackups();
```

---

## 📊 性能目标

| 操作 | 目标 | 备注 |
|------|------|------|
| 文本读写 | <100ms | 包括数据库操作 |
| 图片操作 | <500ms | 可接受的用户体验 |
| 数据库查询 | <200ms | 50 项以内 |
| 监听延迟 | <1s | 定时轮询 500ms 间隔 |
| 空闲 CPU | <5% | 监听不占用资源 |
| DB 大小 | <500MB | 可配置限制 |

---

## 🎯 验收标准速查

**功能**  
✓ 文本/图片/文件列表支持  
✓ 跨平台（Win/Linux/Mac）  
✓ 实时监听  
✓ 搜索/过滤/标签  
✓ 自动清理（按天数）  
✓ 备份恢复  

**质量**  
✓ 测试覆盖 >80%  
✓ 无内存泄漏  
✓ 代码审查通过  
✓ 性能指标达标  

**文档**  
✓ API 参考  
✓ 用户指南  
✓ 开发指南  

---

## 💡 关键决策速记

| 问题 | 决定 | 理由 |
|------|------|------|
| 数据库选型 | **SQLite** | 本地、零依赖、跨平台 |
| 监听方式 | **定时轮询** | 简单可靠、跨平台 |
| 变化检测 | **双哈希** | 快速、低 CPU |
| 历史存储 | **磁盘 + 内存** | 持久化 + 性能 |
| 备份策略 | **定期 + 手动** | 安全 + 灵活 |

---

## ⚡ 快速启动检查

启动前确认：

- [ ] Qt 6 + Qt6::Sql 配置完成
- [ ] CMakeLists.txt 已更新
- [ ] 三平台开发环境就绪
- [ ] 代码审查流程准备好
- [ ] 数据库备份策略制定好
- [ ] 性能基准已建立

---

## 📝 常用 SQL 模板

```sql
-- 最近 N 个项目
SELECT * FROM clipboard_items 
WHERE is_deleted = 0 ORDER BY created_at DESC LIMIT ?;

-- 文本搜索
SELECT * FROM clipboard_items 
WHERE text_content LIKE '%' || ? || '%' 
AND content_type = 'text';

-- 按标签过滤
SELECT * FROM clipboard_items 
WHERE json_extract(tags, '$[*]') LIKE '%' || ? || '%';

-- 过期清理
DELETE FROM clipboard_items 
WHERE is_deleted = 0 AND expire_at < datetime('now');

-- 统计信息
SELECT content_type, COUNT(*) FROM clipboard_items 
GROUP BY content_type;
```

---

## 🔗 文档导航

**详细计划**  
→ `plan/clipboard_implementation_plan.md` 基础架构与接口  
→ `plan/clipboard_database_design.md` 数据库与存储  
→ `plan/clipboard_complete_plan.md` 完整实施总结  

**项目文档**  
→ `src/docs/architecture.md` 项目整体架构  
→ `src/docs/plugin-system.md` 插件系统  
→ `README.md` 项目概述  

---

## 📞 遇到问题？

| 问题 | 查看 |
|------|------|
| 数据库设计问题 | `clipboard_database_design.md` §2 |
| 接口定义问题 | `clipboard_implementation_plan.md` §2 |
| 实施步骤问题 | `clipboard_complete_plan.md` §2 |
| 性能优化 | `clipboard_database_design.md` §8 |
| 跨平台适配 | `clipboard_implementation_plan.md` §6 |

---

**版本**：2.0 | **日期**：2026-04-10 | **状态**：即用型参考卡
