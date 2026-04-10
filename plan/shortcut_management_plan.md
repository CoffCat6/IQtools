# 快捷键管理模块开发计划与设计文档

## 1. 目标与需求
为 IQtools 项目提供一套完整、可扩展的快捷键（Shortcut）管理机制。
- **动态配置**：支持用户自定义快捷键，支持保存和读取配置（持久化存储）。
- **统一管理**：所有的快捷键触发行为通过一个管理类（`ShortcutManager`）统一注册和管理。
- **冲突检测**：在设置新快捷键时，检测是否与其他功能冲突。
- **多组件支持**：支持绑定全局快捷键（`QShortcut`）和菜单栏动作（`QAction`）。
- **插件支持（高扩展性）**：为动态加载的插件提供快捷键注册、冲突规避和卸载反注册机制。

## 2. 核心架构设计

### 2.1 依赖与技术栈
- 框架：Qt C++ (QKeySequence, QShortcut, QAction, QSettings)
- 存储：本地配置文件（INI格式或JSON格式，建议使用 `QSettings` 默认格式，文件路径如 `config/shortcuts.ini`）

### 2.2 接口类定义 (`ShortcutManager`)
设计一个单例模式的管理类，负责维护 `ID -> 快捷键` 的映射关系。

#### 核心数据结构
```cpp
struct ShortcutItem {
    QString id;              // 唯一标识符，核心功能如 "file.open"，插件功能如 "plugin.name.action"
    QString description;     // 快捷键描述（用于在设置界面展示），如 "打开文件"
    QString category;        // 所属分类，如 "文件", "编辑", "插件：代码格式化"
    QKeySequence defaultKey; // 默认快捷键
    QKeySequence currentKey; // 当前生效的快捷键
    QAction* action;         // 绑定的 QAction (可选)
    QShortcut* shortcut;     // 绑定的 QShortcut (可选)
};
```

#### 接口文档 (API)
```cpp
class ShortcutManager : public QObject {
    Q_OBJECT
public:
    static ShortcutManager* instance();

    // 初始化/加载配置
    void loadConfig(const QString& configPath = "config/shortcuts.ini");
    // 保存配置
    void saveConfig();

    // ================= 快捷键注册 =================
    // 注册基于 QAction 的快捷键（通常用于菜单栏和工具栏）
    bool registerAction(const QString& id, const QString& category, const QString& description, const QKeySequence& defaultKey, QAction* action);
    
    // 注册基于 QShortcut 的独立快捷键（用于全局或特定窗口无界面的触发）
    bool registerShortcut(const QString& id, const QString& category, const QString& description, const QKeySequence& defaultKey, QWidget* parent, const char* slot);

    // ================= 插件支持 =================
    // 插件卸载时批量移除该插件注册的所有快捷键，避免内存泄漏和游离快捷键
    void unregisterPrefix(const QString& idPrefix);

    // ================= 配置与查询 =================
    // 修改快捷键
    bool updateShortcut(const QString& id, const QKeySequence& newKey);

    // 冲突检测：返回占用该快捷键的功能ID，如果没有冲突返回空字符串
    QString checkConflict(const QKeySequence& key, const QString& ignoreId = "") const;

    // 获取所有注册的快捷键列表（用于在UI设置界面展示）
    QList<ShortcutItem> getAllShortcuts() const;

    // 恢复默认设置
    void resetToDefault(const QString& id = "");
    void resetAllToDefault();

signals:
    // 快捷键修改时触发，UI界面可据此刷新
    void shortcutChanged(const QString& id, const QKeySequence& newKey);

private:
    QMap<QString, ShortcutItem> m_shortcuts;
};
```

## 3. 具体实现细节与步骤

### 第一阶段：基础管理类搭建 (1天)
1. **单例与数据结构**：创建 `shortcutmanager.h` 和 `shortcutmanager.cpp`，实现单例模式。
2. **注册逻辑**：实现 `registerAction` 和 `registerShortcut`。
   - 方法内部需先调用 `checkConflict` 判断 `defaultKey` / `currentKey` 是否被占用。
   - 若本地缓存中已有该ID的配置，优先使用本地配置；若无，则使用 `defaultKey`。
   - 动态更新 `action->setShortcut(currentKey)`。
3. **信号响应**：实现内部信号机制，使得修改快捷键时，自动更新对应的 `QAction` 或 `QShortcut` 实例。

### 第二阶段：持久化、分类与冲突检测 (1天)
1. **持久化 `QSettings`**：
   - 配置文件按层级存储，如 `[Shortcuts]` 组。
   - `Key` 存 `id`，`Value` 存 `QKeySequence::toString(QKeySequence::PortableText)`。
2. **冲突检测**：`checkConflict` 方法遍历 `m_shortcuts`。如果传入了 `ignoreId`，在比对时跳过该 ID（在更新自身快捷键时使用）。

### 第三阶段：插件快捷键生态支持 (1天)
1. **命名规范与预留机制**：
   - 规定主程序快捷键以 `app.`、`file.`、`edit.`、`view.` 等作为前缀。
   - 插件申请的快捷键必须以 `plugin.[PluginName].` 作为前缀。
2. **生命周期管理**：
   - 插件加载时 (`loadPlugin`)：调用 `ShortcutManager::instance()->registerAction(...)` 注册。
   - 插件卸载时 (`unloadPlugin`)：调用 `ShortcutManager::instance()->unregisterPrefix("plugin.myplugin.")` 清理资源，解除绑定。
3. **UI分类展示**：在配置UI中，通过读取 `ShortcutItem.category`，将插件快捷键和系统自带快捷键分 Tab 或分组（如 TreeView 树状视图）展示，降低用户查找负担。

### 第四阶段：UI 交互界面 (1-2天)
1. **界面构建**：创建快捷键设置对话框 `ShortcutSettingsDialog`。
2. **视图与模型**：使用 `QTreeWidget` 按 `category` 进行分组展示。包含三列：`功能描述`、`当前快捷键`、`默认快捷键`。
3. **快捷键捕获**：提供一个专门的输入框（重写 `keyPressEvent`），捕获按键并生成 `QKeySequence`。
4. **校验与保存**：验证新快捷键不冲突后，调用 `updateShortcut` 并 `saveConfig`。

## 4. 预留快捷键配置清单

在项目启动时，系统应预留并注册以下基础功能的快捷键，避免与后续插件冲突：

| 分类 (Category) | 唯一ID (ID) | 描述 (Description) | 默认快捷键 (Default Key) |
| --- | --- | --- | --- |
| **文件** | `file.new` | 新建项目 | `Ctrl+N` |
| | `file.open` | 打开文件/项目 | `Ctrl+O` |
| | `file.save` | 保存 | `Ctrl+S` |
| | `file.saveas` | 另存为 | `Ctrl+Shift+S` |
| | `file.quit` | 退出程序 | `Ctrl+Q` |
| **编辑** | `edit.undo` | 撤销 | `Ctrl+Z` |
| | `edit.redo` | 重做 | `Ctrl+Y` 或 `Ctrl+Shift+Z` |
| | `edit.copy` | 复制 | `Ctrl+C` |
| | `edit.paste` | 粘贴 | `Ctrl+V` |
| | `edit.cut` | 剪切 | `Ctrl+X` |
| **视图** | `view.zoomin` | 放大视图 | `Ctrl++` 或 `Ctrl+=` |
| | `view.zoomout` | 缩小视图 | `Ctrl+-` |
| | `view.reset` | 恢复默认视图 | `Ctrl+0` |
| **工具/执行** | `tool.run` | 运行当前工具/脚本 | `F5` |
| | `tool.stop` | 停止/终止运行 | `Shift+F5` |
| | `tool.build` | 编译/构建 | `Ctrl+B` |
| | `tool.clear_log` | 清空输出日志 | `Ctrl+L` |
| **帮助/系统** | `help.doc` | 查看帮助文档 | `F1` |
| | `app.settings` | 打开系统设置/偏好设置 | `Ctrl+,` (逗号) |
| | `app.search` | 全局搜索/查找 | `Ctrl+F` |
| | `app.find_next`| 查找下一个 | `F3` |

### 5. 插件集成示例代码

**插件中注册快捷键：**
```cpp
void MyCustomPlugin::initialize() {
    // 1. 创建插件特定的 QAction
    QAction* formatAction = new QAction(tr("格式化代码"), this);
    connect(formatAction, &QAction::triggered, this, &MyCustomPlugin::formatCode);

    // 2. 注册到全局快捷键管理器
    // 注意：ID 必须包含 "plugin.mycustomplugin." 前缀
    ShortcutManager::instance()->registerAction(
        "plugin.mycustomplugin.format", 
        tr("插件: 代码格式化"), // 分类名称
        tr("格式化当前文档"),   // 描述
        QKeySequence("Ctrl+Shift+F"), 
        formatAction
    );
}

void MyCustomPlugin::unload() {
    // 卸载插件时，一键注销本插件关联的所有快捷键
    ShortcutManager::instance()->unregisterPrefix("plugin.mycustomplugin.");
}
```