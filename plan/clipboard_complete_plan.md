# IQtools 剪贴板功能完整实施规划（总体版）

**版本**：v2.0（最终版，包含 SQLite 数据库）  
**日期**：2026-04-10  
**状态**：完整规划  

---

## 执行摘要

本规划将为 IQtools 构建一个**企业级剪贴板管理系统**，包括：

| 功能模块 | 特性 | 实施周期 |
|---------|------|---------|
| **基础读写** | 文本、图片、文件列表跨平台支持 | 第1-3周 |
| **实时监听** | 后台监听剪贴板变化，响应延迟 <1s | 第2-3周 |
| **历史记录** | SQLite 持久化，支持查找、标签、备注 | 第3-4周 |
| **智能管理** | 过期清理、自动备份、容量管理 | 第4-5周 |
| **用户界面** | 完整的 QML UI，搜索、过滤、预览 | 第4-5周 |
| **数据安全** | 备份导出、备份恢复、数据迁移 | 第5-6周 |

**总投入**：6 周，分 5 个阶段并行开发

---

## 分层架构总览

```
┌──────────────────────────────────────────────────────────────────┐
│ 表示层 (Presentation)                                            │
│ ClipboardPage.qml + ClipboardController                          │
│ - 历史记录列表（搜索、过滤、分页）                               │
│ - 配置面板（保留期、自动备份、容量限制）                        │
│ - 备份管理（创建、恢复、导出）                                  │
└──────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Qt 信号/槽 + Q_INVOKABLE
                              │
┌──────────────────────────────────────────────────────────────────┐
│ 业务逻辑层 (Business Logic)                                      │
│ ClipboardService                                                  │
│ - 剪贴板读写 API                                                 │
│ - 历史记录管理（ClipboardHistoryManager）                       │
│ - 配置管理（ClipboardConfigManager）                             │
│ - 备份管理（ClipboardBackupManager）                             │
│ - 实时监听（ClipboardMonitor）                                  │
└──────────────────────────────────────────────────────────────────┘
                              ▲
                              │ 抽象接口
                              │
┌──────────────────────────────────────────────────────────────────┐
│ 基础设施层 (Infrastructure)                                      │
│ 数据库 (ClipboardDatabase)      平台 (IPlatformClipboard)       │
│ - SQLite 连接                  - Windows 实现                    │
│ - 事务管理                     - Linux 实现                      │
│ - 迁移脚本                     - macOS 实现                      │
│ - 备份/恢复                    - 工厂函数                        │
└──────────────────────────────────────────────────────────────────┘
```

---

## 数据库设计核心

### 5 张表的职责

| 表名 | 行数级别 | 用途 | 主要查询 |
|------|---------|------|---------|
| `clipboard_items` | 高频 | 存储剪贴板项目 | 按时间/类型/标签查询 |
| `clipboard_history` | 高频 | 访问日志 | 追踪每个项目的读写历史 |
| `clipboard_settings` | 低频（1行） | 配置参数 | 获取/更新全局配置 |
| `clipboard_searches` | 低频 | 保存的搜索条件 | 快速搜索 |
| `clipboard_backups` | 低频 | 备份元数据 | 查看备份列表、恢复 |

### 关键索引优化

```sql
-- 最频繁查询的字段必有索引
CREATE INDEX idx_clipboard_items_created_at ON clipboard_items(created_at DESC);
CREATE INDEX idx_clipboard_items_content_type ON clipboard_items(content_type);
CREATE INDEX idx_clipboard_items_is_pinned ON clipboard_items(is_pinned);
CREATE UNIQUE INDEX idx_clipboard_items_content_hash ON clipboard_items(content_hash);
```

---

## 完整的实施时间表

### 第 1-2 周：基础设施搭建

#### 第 1 周

**任务 1：平台抽象层**
- 定义 `IPlatformClipboard` 接口（50 行）
- 定义数据结构 `ClipboardItem`、`ClipboardSnapshot` 等（100 行）
- 创建工厂函数 `createPlatformClipboard()`（30 行）

**任务 2：Windows 实现**
- `clipboard_windows.cpp`（200 行）
- 使用 `QClipboard` + 可选 Windows API
- 单元测试（100 行）

**任务 3：Linux 实现**
- `clipboard_linux.cpp`（150 行）
- 使用 `QClipboard`，可选 X11 支持
- 单元测试（80 行）

#### 第 2 周

**任务 4：macOS 实现**
- `clipboard_macos.cpp`（150 行）
- 单元测试（80 行）

**任务 5：数据库基础**
- `clipboard_database.h/cpp`（300 行）
- 5 张表的 SQL 定义
- 数据库初始化与迁移（200 行）
- 事务管理（RAII）（50 行）

---

### 第 2-3 周：Core 层实现

#### 第 2 周后半

**任务 6：ClipboardService**
- `clipboard_service.h/cpp`（400 行）
- 集成平台实现
- 监听启动/停止
- 历史记录初始化

**任务 7：ClipboardMonitor**
- `clipboard_monitor.h/cpp`（150 行）
- 后台定时轮询
- 变化检测逻辑
- 信号发送

#### 第 3 周

**任务 8：ClipboardHistoryManager**
- `clipboard_history_manager.h/cpp`（500 行）
- CRUD 操作
- 复杂查询（搜索、过滤、排序、分页）
- 过期清理逻辑

**任务 9：ClipboardConfigManager**
- `clipboard_config_manager.h/cpp`（200 行）
- 配置读写、持久化
- 默认值管理

**任务 10：ClipboardBackupManager**
- `clipboard_backup_manager.h/cpp`（300 行）
- 备份创建/恢复
- 自动备份定时器
- 导入/导出

---

### 第 3-4 周：集成与扩展

#### 第 3 周后半

**任务 11：AppContext 集成**
- 在 `app_context.cpp` 中注册 `ClipboardService`
- 在 `application_bootstrap.cpp` 中初始化数据库
- 配置加载

#### 第 4 周

**任务 12：ClipboardController**
- `clipboard_controller.h/cpp`（250 行）
- Q_INVOKABLE 方法暴露给 QML
- 信号适配

**任务 13：QML UI - ClipboardPage**
- `ClipboardPage.qml`（400 行）
- 历史记录列表
- 搜索、过滤、分页
- 配置面板
- 备份管理面板

**任务 14：主窗口导航集成**
- 在 `main.qml` 中添加 ClipboardPage
- 导航菜单添加入口
- AppFacade 暴露 `clipboardController`

---

### 第 5-6 周：测试、优化、文档

#### 第 5 周

**任务 15：单元测试**
- `test_clipboard_database.cpp`（200 行）
- `test_clipboard_history_manager.cpp`（300 行）
- `test_clipboard_config_manager.cpp`（150 行）
- `test_clipboard_backup_manager.cpp`（200 行）
- 覆盖率目标：>80%

**任务 16：集成测试**
- 端到端测试（UI → Core → Platform）
- 跨平台测试（Windows/Linux/macOS）
- 性能测试（读写速度、CPU、内存）

#### 第 6 周

**任务 17：性能优化**
- 查询优化（添加缺失索引）
- 缓存实现（最近访问项目）
- 内存泄漏检查

**任务 18：文档编写**
- 用户文档（功能说明、快捷键）
- 开发文档（API 参考、扩展指南）
- 架构文档更新

**任务 19：Bug 修复与用户体验改进**
- 修复测试中发现的问题
- UI 响应式设计调整
- 错误处理完善

---

## 关键数据模型

### ClipboardItem（剪贴板项目）

```cpp
struct ClipboardItem {
    int id;                              // 数据库 ID
    ClipboardContentType type;           // 文本/图片/文件
    QString mimeType;                    // MIME 类型
    QString contentHash;                 // SHA256 哈希
    
    // 内容
    QString textContent;                 // 文本
    QPixmap imageContent;                // 图片
    QStringList fileList;                // 文件列表
    
    // 元数据
    qint64 sizeBytes;                    // 大小
    QString thumbnailPath;               // 缩略图路径
    QDateTime createdAt;                 // 创建时间
    QDateTime accessedAt;                // 最后访问时间
    QDateTime expireAt;                  // 过期时间
    
    // 用户标记
    bool isPinned;                       // 是否固定
    QStringList tags;                    // 标签
    QString notes;                       // 备注
};
```

### ClipboardHistoryQuery（查询条件）

```cpp
struct ClipboardHistoryQuery {
    QString textContent;                 // 文本搜索（模糊匹配）
    QStringList contentTypes;            // 类型过滤
    QStringList tags;                    // 标签过滤
    QDateTime fromTime;                  // 开始时间
    QDateTime toTime;                    // 结束时间
    
    enum SortBy { ByNewest, ByOldest, BySize };
    SortBy sortBy = ByNewest;
    int offset = 0;                      // 分页偏移
    int limit = 50;                      // 分页大小
    
    bool pinnedOnly = false;
    bool excludeDeleted = true;
};
```

---

## 核心功能流程

### 流程 1：剪贴板内容变化 → 自动保存历史

```
1. ClipboardMonitor 定时检查 (500ms 间隔)
   ↓
2. 获取当前剪贴板内容 (IPlatformClipboard::getContent())
   ↓
3. 与上次快照对比 (ClipboardSnapshot 的哈希)
   ↓
4. 如果不同，发送 contentChanged 信号
   ↓
5. ClipboardService 接收信号
   ↓
6. ClipboardHistoryManager::addItem() 写入数据库
   ↓
7. ClipboardService 发送 historyItemAdded 信号
   ↓
8. UI 更新列表 (ListView model refresh)
```

### 流程 2：用户搜索历史

```
1. 用户输入搜索条件 (UI → ClipboardController)
   ↓
2. 构建 ClipboardHistoryQuery 对象
   ↓
3. ClipboardHistoryManager::queryHistory() 执行 SQL
   ↓
4. 返回分页结果 (ClipboardHistoryResult)
   ↓
5. UI 显示结果列表
   ↓
6. 用户可点击"复制"恢复到剪贴板
   ↓
7. ClipboardService::restoreFromHistory() 调用
   ↓
8. IPlatformClipboard::setTextToClipboard() 写入
```

### 流程 3：自动备份

```
1. 启用自动备份时，启动定时器 (间隔 N 天)
   ↓
2. 定时触发备份流程
   ↓
3. ClipboardBackupManager::createAutoBackup()
   ↓
4. 执行数据库 VACUUM + 复制到备份文件
   ↓
5. 在 clipboard_backups 表中记录
   ↓
6. 清理旧备份 (只保留最新 5 个)
```

---

## 关键设计决策

| 决策 | 理由 | 替代方案 | 何时切换 |
|------|------|---------|---------|
| **SQLite 数据库** | 本地存储、零依赖、跨平台 | PostgreSQL/MongoDB | 需要多用户同步 |
| **定时轮询监听** | 简单可靠、跨平台 | 平台原生事件 | 性能要求极高 |
| **双哈希变化检测** | 快速对比、低 CPU | 深度序列化对比 | 数据格式复杂 |
| **内存 + 磁盘缓存** | 平衡性能与持久性 | 纯内存 | 数据安全不重要 |
| **QML + Qt Widgets** | 现有架构一致 | 第三方 UI 库 | 需要 Web UI |

---

## 文件清单总表

### 新增文件（共 22 个）

```
Core 业务逻辑层 (5 个)
├── src/core/clipboard/i_clipboard_service.h
├── src/core/clipboard/clipboard_service.h/cpp
├── src/core/clipboard/clipboard_monitor.h/cpp
├── src/core/clipboard/clipboard_history_manager.h/cpp
├── src/core/clipboard/clipboard_config_manager.h/cpp
└── src/core/clipboard/clipboard_backup_manager.h/cpp

基础设施层 (8 个)
├── src/infra/clipboard/clipboard_data_types.h
├── src/infra/clipboard/clipboard_constants.h
├── src/infra/clipboard/clipboard_utils.h/cpp
├── src/infra/platform/i_platform_clipboard.h
├── src/infra/platform/clipboard_platform_factory.h/cpp
├── src/infra/platform/windows/clipboard_windows.h/cpp
├── src/infra/platform/linux/clipboard_linux.h/cpp
└── src/infra/platform/macos/clipboard_macos.h/cpp
└── src/infra/storage/clipboard_database.h/cpp

UI 层 (4 个)
├── src/app/shell/bridge/clipboard_controller.h/cpp
└── src/app/shell/qml/pages/ClipboardPage.qml

测试 (5 个)
├── src/tests/core/test_clipboard_service.cpp
├── src/tests/core/test_clipboard_history_manager.cpp
├── src/tests/core/test_clipboard_config_manager.cpp
├── src/tests/core/test_clipboard_backup_manager.cpp
└── src/tests/infra/test_clipboard_database.cpp
```

### 修改文件（共 6 个）

```
核心文件
├── CMakeLists.txt (添加新源文件、Qt6::Sql 依赖)
├── src/core/appcontext/app_context.h/cpp (添加 clipboard service)
├── src/app/bootstrap/application_bootstrap.cpp (初始化数据库)
├── src/app/shell/bridge/app_facade.h/cpp (添加 clipboard controller)
├── src/app/shell/qml/main.qml (添加导航入口)
```

---

## 验收标准清单

### 功能完整性

- ✅ Windows/Linux/macOS 上剪贴板读写正常
- ✅ 文本、图片、文件列表支持完整
- ✅ 实时监听响应延迟 <1 秒
- ✅ 历史查询支持文本搜索、类型过滤、标签过滤
- ✅ 可配置保留天数（1-365 天）
- ✅ 自动清理过期项目
- ✅ 手动/自动备份功能
- ✅ 备份恢复功能
- ✅ 用户可导出备份文件

### 质量指标

- ✅ 单元测试覆盖率 >80%
- ✅ 集成测试全通过
- ✅ 无内存泄漏（Valgrind/AddressSanitizer）
- ✅ 代码审查通过
- ✅ 性能指标达标：
  - 文本操作 <100ms
  - 图片操作 <500ms
  - 数据库查询 <200ms
  - 空闲时 CPU <5%
- ✅ 数据库大小 <500MB（可配置）
- ✅ 应用启动时间无明显增加（<500ms）

### 文档完整性

- ✅ API 参考文档
- ✅ 用户使用指南
- ✅ 开发者扩展指南
- ✅ 架构设计文档
- ✅ 数据库结构说明
- ✅ 代码注释完整（关键流程、复杂算法）

---

## 后续阶段规划（Phase 2+）

### Phase 2（可选，第 7-8 周）

- [ ] 云端同步支持（iCloud/OneDrive/Google Drive）
- [ ] 跨设备剪贴板共享
- [ ] 剪贴板内容加密存储
- [ ] 高级搜索语法（正则表达式、日期范围等）
- [ ] 浮窗快速访问小工具

### Phase 3（长期规划）

- [ ] 剪贴板插件扩展 API
- [ ] 第三方服务集成（如自动上传到图床）
- [ ] 剪贴板内容版本控制
- [ ] 团队协作模式（共享团队剪贴板）
- [ ] 性能分析与可视化

---

## 风险管理

### 高风险项

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| SQLite 性能瓶颈（大数据量） | 低 | 高 | 预留分片、迁移路径；定期性能测试 |
| 跨平台兼容性问题 | 中 | 中 | 早期在三平台测试；建立兼容性矩阵 |
| 数据库腐坏导致数据丢失 | 很低 | 极高 | 自动备份、定期 VACUUM、完整性检查 |

### 中等风险项

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| 查询性能不足（响应慢） | 中 | 中 | 索引优化、查询缓存、批量操作 |
| 监听占用 CPU 过高 | 低 | 中 | 可配置检查间隔、异步处理 |
| UI 响应延迟 | 低 | 低 | 后台线程、异步数据加载 |

---

## 资源投入与预期产出

### 人力投入

| 角色 | 周数 | 任务 |
|------|------|------|
| 核心开发 | 6 周 | 架构、核心实现、集成 |
| 平台适配 | 2 周 | 三平台实现 |
| 测试工程师 | 2 周 | 单元测试、集成测试 |
| UI/UX 设计 | 1 周 | 页面设计、交互优化 |

**总计**：6 周

### 预期产出

✅ 完整的企业级剪贴板管理系统  
✅ 500+ 行单元测试  
✅ 跨平台测试覆盖  
✅ 完整的 API 文档与用户指南  
✅ 生产就绪的代码质量

---

## 快速参考

### 常用查询 SQL

```sql
-- 获取最近 20 个项目
SELECT * FROM clipboard_items 
WHERE is_deleted = 0 
ORDER BY created_at DESC LIMIT 20;

-- 搜索文本
SELECT * FROM clipboard_items 
WHERE text_content LIKE '%keyword%' 
AND content_type = 'text' 
ORDER BY created_at DESC;

-- 获取已过期项目
SELECT * FROM clipboard_items 
WHERE expire_at IS NOT NULL 
AND expire_at < datetime('now');

-- 统计按类型分布
SELECT content_type, COUNT(*) as count 
FROM clipboard_items 
WHERE is_deleted = 0 
GROUP BY content_type;

-- 获取固定项目
SELECT * FROM clipboard_items 
WHERE is_pinned = 1 AND is_deleted = 0 
ORDER BY created_at DESC;
```

### 常用 C++ 代码片段

```cpp
// 获取最近历史
auto recent = historyManager->getRecentItems(20);

// 复杂查询
ClipboardHistoryQuery query;
query.textContent = "example";
query.contentTypes = {"text", "html"};
query.tags = {"important"};
query.fromTime = QDateTime::currentDateTime().addDays(-7);
query.limit = 10;
auto results = historyManager->queryHistory(query);

// 创建备份
bool success = backupManager->createBackup(
    "manual_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"),
    "Manual backup for data safety");

// 清理过期项目
int removed = historyManager->cleanupExpired();
qDebug() << "Removed" << removed << "expired items";
```

---

## 相关文档索引

| 文档 | 位置 | 内容 |
|------|------|------|
| 基础实施计划 | `plan/clipboard_implementation_plan.md` | 架构、接口、基础阶段 |
| 数据库设计 | `plan/clipboard_database_design.md` | 5 张表、索引、SQL |
| **本文档** | `plan/clipboard_complete_plan.md` | 完整实施总结 |
| 架构设计 | `src/docs/architecture.md` | 项目整体架构 |

---

## 检查清单（用于项目启动前）

项目启动前，确保：

- [ ] 已获得项目经理或 PO 的需求确认
- [ ] Qt 6 环境已配置（包括 Qt6::Sql）
- [ ] SQLite 开发库已安装
- [ ] 代码审查流程已准备好
- [ ] CI/CD 配置支持 CMake + 单元测试
- [ ] 三个开发平台已准备（或有 CI 覆盖）
- [ ] 备份策略已制定（特别是数据库备份）
- [ ] 性能基准已建立（用于后续对标）

---

**文档版本**：2.0（最终版，包含 SQLite 数据库）  
**最后更新**：2026-04-10  
**作者**：开发团队  
**审核状态**：待审核

---

## 联系与支持

如有疑问或需要补充：
- 查看详细的基础计划：`clipboard_implementation_plan.md`
- 查看数据库设计：`clipboard_database_design.md`
- 参考项目架构：`src/docs/architecture.md`
