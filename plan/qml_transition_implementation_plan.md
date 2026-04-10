# IQtools QML 迁移实施细化方案

## 1. 文档目标

本文档基于 `plan/qml_transition_plan.md` 进一步细化 QML / Qt Quick 迁移的实施方案，目标是：

1. 在**不破坏既有后端模块边界**的前提下，将 UI 层从 Qt Widgets 平滑迁移到 QML。
2. 将迁移工作拆分为**可执行、可验证、可回滚**的阶段。
3. 明确 C++ 与 QML 的桥接方式、资源组织、主题机制、导航结构、模型暴露方式与验收标准。

---

## 2. 当前状态复盘

根据现有代码，当前项目的实际状态如下：

### 2.1 构建层

- `CMakeLists.txt` 当前只引入：`Qt6::Core`、`Qt6::Gui`、`Qt6::Widgets`
- 目前是典型 Widgets 应用，使用 `qt_add_executable(IQtools WIN32 ...)`
- 尚未引入：
  - `Qt6::Qml`
  - `Qt6::Quick`
  - `qt_add_qml_module(...)`
- 尚未建立 QML 资源入口与模块命名空间

### 2.2 应用入口与启动流程

- `src/app/main.cpp` 使用 `QApplication`
- `ApplicationBootstrap::initialize()` 内部分三步：
  1. `initLogging()`
  2. `initCore()`
  3. `initUi()`
- `initUi()` 当前行为：
  - 通过 `ThemeManager::applyTheme(...)` 加载 QSS
  - 创建 `MainWindow`
  - 显示 `QMainWindow`

### 2.3 主题系统

- `ThemeManager` 是**静态工具类**，不是 `QObject`
- 当前职责是读取 `src/resources/qss/*.qss` 并直接调用 `QApplication::setStyleSheet(...)`
- 这意味着它服务于 Widgets 样式机制，而不是 QML 响应式主题机制

### 2.4 工具数据结构

- `ToolRegistry` 是普通 C++ 类，内部持有 `QVector<ToolMetadata>`
- `ToolMetadata` 是简单结构体：
  - `id`
  - `name`
  - `category`
  - `description`
- 当前结构非常适合被封装为 `QAbstractListModel`

### 2.5 现有 Shell UI 结构

- `MainWindow` 采用固定左导航 + 右内容区的布局
- 内容区通过 `QStackedWidget` 切页
- 首页使用手工 `QGridLayout` 实现 Bento 风格卡片
- 页面包括：
  - 首页
  - 截图工具
  - 翻译工具
  - 插件管理
  - 设置

### 2.6 核心结论

当前项目的**业务后端基础已具备**，问题主要集中在：

1. 启动流程仍然绑定 Widgets
2. 主题系统仍然绑定 QSS
3. 工具数据未以 QML 友好的模型形式暴露
4. Shell 层全部是 QWidget 结构，无法直接复用到 QML

因此，本次迁移应当遵循：**保留后端、替换壳层、重建表现层桥接**。

---

## 3. 迁移原则

### 3.1 总体原则

1. **后端不重写**：保留 `core`、`infra`、`plugin-api`、`plugins` 的分层结构。
2. **UI 单独迁移**：优先替换 `app/shell` 与展示层，不在第一阶段触碰业务逻辑。
3. **桥接层隔离**：不要让 QML 直接依赖复杂核心对象，增加 ViewModel / Model / Facade 层进行隔离。
4. **先跑通，再美化**：先建立 QML 基础运行链路，再逐步迁移 Bento 视觉与交互细节。
5. **保留回退能力**：在迁移中短期内保留 Widgets 入口或旧代码，避免一次性替换导致无法运行。

### 3.2 技术原则

1. **QML 面向声明式 UI，C++ 面向状态与服务**。
2. **所有供 QML 使用的数据必须具备稳定接口**：`Q_PROPERTY`、`Q_INVOKABLE`、`QAbstractListModel`。
3. **主题必须改为 token 驱动**，而不是继续依赖 QSS。
4. **页面切换优先使用路由状态驱动**，避免在 QML 中散落硬编码切换逻辑。
5. **资源统一纳入 Qt Resource / QML Module**，避免继续依赖磁盘相对路径。

---

## 4. 目标架构

## 4.1 迁移后的分层结构

建议迁移后的结构如下：

```text
src/
├─ app/
│  ├─ main.cpp
│  ├─ bootstrap/
│  │  └─ application_bootstrap.*
│  ├─ shell/
│  │  ├─ qml/
│  │  │  ├─ main.qml
│  │  │  ├─ AppWindow.qml
│  │  │  ├─ components/
│  │  │  ├─ pages/
│  │  │  └─ theme/
│  │  └─ bridge/
│  │     ├─ tool_list_model.*
│  │     ├─ theme_controller.*
│  │     ├─ navigation_controller.*
│  │     └─ app_facade.*
├─ core/
├─ infra/
├─ plugin-api/
└─ resources/
```

说明：

- `core/` 保持偏业务和基础服务
- `app/shell/bridge/` 放 QML 专用桥接对象
- `app/shell/qml/` 放所有 QML 页面与组件
- 这样可以避免把 QML 适配逻辑塞进 `core/`，污染后端层

### 4.2 对象职责划分

#### 保留在 `core/` 的对象

- `AppContext`
- `ToolRegistry`
- `LogService`
- 插件相关对象

#### 新增在 `app/shell/bridge/` 的对象

- `ToolListModel`
  - 从 `ToolRegistry` 读取数据
  - 对 QML 暴露角色字段
- `ThemeController`
  - 管理当前主题、主题切换、主题 token
- `NavigationController`
  - 管理当前页面路由、导航项状态
- `AppFacade`
  - 统一向 QML 暴露应用级服务入口，减少 QML 上下文碎片化

---

## 5. 启动链路实施方案

### 5.1 目标链路

Widgets 时代：

```text
main.cpp
  -> QApplication
  -> ApplicationBootstrap::initialize()
  -> ThemeManager::applyTheme()
  -> MainWindow.show()
```

QML 时代建议改为：

```text
main.cpp
  -> QGuiApplication / QApplication
  -> ApplicationBootstrap::initialize()
      -> initLogging()
      -> initCore()
      -> initPresentation()
          -> 创建桥接对象
          -> 注册 QML 类型 / 单例
          -> 启动 QQmlApplicationEngine
          -> load main.qml
```

### 5.2 `QGuiApplication` 还是 `QApplication`

建议：**短期继续使用 `QApplication`，中期再视情况切换为 `QGuiApplication`**。

原因：

1. 当前代码仍然链接 Widgets，迁移初期保留 `QApplication` 兼容性最高。
2. Qt Quick 应用使用 `QApplication` 也完全可行。
3. 等 Widgets 代码完全下线后，再判断是否改回 `QGuiApplication`。

这样可以降低迁移第一阶段的风险。

### 5.3 Bootstrap 重构建议

将 `initUi()` 重命名为 `initPresentation()`，职责变为：

1. 创建 QML 所需桥接对象
2. 注册 QML 类型或设置上下文属性
3. 初始化主题默认值
4. 创建 `QQmlApplicationEngine`
5. 加载 QML 根组件
6. 校验根对象是否成功加载

建议新增成员：

- `std::unique_ptr<QQmlApplicationEngine> m_qmlEngine;`
- `std::unique_ptr<ToolListModel> m_toolListModel;`
- `std::unique_ptr<ThemeController> m_themeController;`
- `std::unique_ptr<NavigationController> m_navigationController;`
- `std::unique_ptr<AppFacade> m_appFacade;`

---

## 6. C++ / QML 桥接实施方案

## 6.1 桥接总体策略

不建议让 QML 直接访问 `AppContext` 里的全部对象，而应通过“桥接层”暴露稳定、受控、便于演进的接口。

推荐桥接方式如下：

### 方式 A：`setContextProperty`

适合：

- 应用级单例对象
- 启动时创建一次，全局使用

可用于：

- `appFacade`
- `themeController`
- `navigationController`
- `toolListModel`

优点：实现快，适合当前项目阶段。

缺点：模块化程度稍弱，测试隔离略差。

### 方式 B：`qmlRegisterSingletonType / qmlRegisterSingletonInstance`

适合：

- 长期稳定的全局对象
- 需要在 QML 模块体系内清晰声明 API

建议中期演进到该方式，特别是：

- `ThemeController`
- `NavigationController`

### 方式 C：`QAbstractListModel`

适合：

- 列表数据
- 需要给 `ListView`、`Repeater`、`GridView` 使用的数据集

用于：

- `ToolListModel`

---

## 6.2 ToolListModel 设计

### 6.2.1 目标

把 `ToolRegistry` 的 `QVector<ToolMetadata>` 暴露给 QML，使首页卡片区、导航区、工具页等都能共享同一份数据源。

### 6.2.2 建议角色定义

`ToolListModel` 建议提供如下 role：

- `IdRole`
- `NameRole`
- `CategoryRole`
- `DescriptionRole`
- `RouteRole`
- `HighlightedRole`
- `AvailableRole`

其中后 3 个字段当前后端没有，可先在桥接层派生：

- `route`：如 `home` / `capture` / `translate`
- `highlighted`：用于首页 Bento 卡片强调样式
- `available`：用于未来插件启用/禁用状态显示

### 6.2.3 实施建议

阶段一不要急着改动 `ToolRegistry` 为 `QObject`，而是：

1. 保持 `ToolRegistry` 现状
2. 新建 `ToolListModel` 从 `ToolRegistry` 读取数据
3. 若未来工具注册支持动态变化，再增加刷新接口：
   - `reload()`
   - `appendTool(...)`
   - `removeTool(...)`

这样风险最小。

---

## 6.3 ThemeController 设计

### 6.3.1 为什么不直接复用 `ThemeManager`

当前 `ThemeManager`：

- 是静态类
- 面向 QSS
- 没有通知信号
- 没有可绑定属性

这不适合 QML。

因此建议：

- **保留旧 `ThemeManager` 作为 Widgets 兼容层**
- **新增 `ThemeController` 作为 QML 主题控制器**

### 6.3.2 ThemeController 最低能力

建议提供：

- `Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY currentThemeChanged)`
- `Q_PROPERTY(QVariantMap palette READ palette NOTIFY themeChanged)`
- `Q_PROPERTY(bool dark READ dark NOTIFY themeChanged)`
- `Q_INVOKABLE void setTheme(const QString& themeName)`
- `Q_INVOKABLE void toggleTheme()`

### 6.3.3 palette / token 设计

不要让 QML 页面直接写死颜色，统一通过 token 暴露，例如：

- `bg.app`
- `bg.panel`
- `bg.card`
- `bg.cardHighlight`
- `text.primary`
- `text.secondary`
- `text.muted`
- `border.default`
- `accent.primary`
- `accent.secondary`
- `shadow.color`
- `radius.card`
- `spacing.md`
- `spacing.lg`

QML 页面只引用 token，不直接引用硬编码十六进制颜色。这样以后：

1. Dark / Light 切换更简单
2. 自定义主题更容易接入
3. UI 风格更统一

### 6.3.4 是否保留 QSS

建议：

- **短期保留**：作为旧 Widgets 界面的 fallback
- **新 QML 界面不依赖 QSS**
- 后期若 Widgets 完全移除，可将 `ThemeManager` 缩减为仅负责主题配置持久化

---

## 6.4 NavigationController 设计

### 6.4.1 目标

统一管理左侧导航、当前页状态、页面切换日志，而不是在 QML 中每个按钮自己维护 selected 状态。

### 6.4.2 建议接口

- `Q_PROPERTY(QString currentRoute READ currentRoute NOTIFY currentRouteChanged)`
- `Q_PROPERTY(QVariantList navigationItems READ navigationItems NOTIFY navigationChanged)`
- `Q_INVOKABLE void navigateTo(const QString& route)`
- `Q_INVOKABLE bool isCurrentRoute(const QString& route) const`

### 6.4.3 路由值建议

- `home`
- `capture`
- `translate`
- `plugins`
- `settings`

### 6.4.4 页面切换策略

QML 层建议：

- 第一阶段使用 `Loader` + `currentRoute`
- 第二阶段视交互复杂度引入 `StackView`

原因：

- 当前页面层级浅，`Loader` 更简单直接
- 若后续工具页需要多级跳转，再升级为 `StackView`

---

## 6.5 AppFacade 设计

### 6.5.1 目标

为 QML 提供一个总入口对象，避免页面到处引用多个 context property。

### 6.5.2 建议内容

`AppFacade` 可暴露：

- `themeController`
- `navigationController`
- `toolListModel`
- 日志接口（如 `logInfo`）
- 全局设置接口（未来）

### 6.5.3 价值

1. QML 代码更整洁
2. 上下文注入更集中
3. 将来更适合测试或替换实现

---

## 7. QML 目录与组件规划

## 7.1 推荐目录结构

```text
src/app/shell/qml/
├─ main.qml
├─ AppWindow.qml
├─ components/
│  ├─ AppSidebar.qml
│  ├─ NavItem.qml
│  ├─ BentoCard.qml
│  ├─ BentoSectionHeader.qml
│  ├─ ThemedButton.qml
│  └─ ThemeBadge.qml
├─ pages/
│  ├─ HomePage.qml
│  ├─ CapturePage.qml
│  ├─ TranslatePage.qml
│  ├─ PluginPage.qml
│  └─ SettingsPage.qml
└─ theme/
   ├─ Theme.js
   └─ metrics.js
```

---

## 7.2 根组件职责

### `main.qml`

职责：

- 作为 QML 模块入口
- 创建 `AppWindow`
- 处理应用级窗口属性

### `AppWindow.qml`

职责：

- 定义主布局
- 左侧 Sidebar
- 右侧内容区
- 顶部标题栏（如果需要）
- 统一背景、阴影、圆角、内边距规则

---

## 7.3 页面组件职责

### `HomePage.qml`

展示：

- 主页欢迎信息
- Bento 卡片矩阵
- 核心工具摘要
- 系统能力概览

### `CapturePage.qml`

第一阶段先做占位页，展示：

- 工具名称
- 功能说明
- 待实现状态

### `TranslatePage.qml`

第一阶段先做占位页

### `PluginPage.qml`

第一阶段先做占位页，后续对接插件列表

### `SettingsPage.qml`

第一阶段必须包含：

- Dark / Light 切换
- 当前主题状态展示

---

## 8. Bento Grid 视觉实施细化

## 8.1 视觉目标

目标不是机械地复制当前 QGridLayout，而是把现有信息结构升级为更现代的 Qt Quick 表达方式。

关键视觉特征：

1. 大圆角卡片
2. 柔和阴影 / 层次感
3. 深浅主题下都清晰可读
4. 卡片大小不一致，形成节奏感
5. 留白明显、布局呼吸感强

## 8.2 实施方式

QML 中不建议一开始就追求复杂自动布局算法，建议分两步：

### 第一阶段：静态响应式网格

使用：

- `GridLayout`
- `Layout.columnSpan`
- `Layout.rowSpan`

优点：

- 容易复刻当前 Bento 样式
- 开发快
- 可控性高

### 第二阶段：自适应卡片编排

在有明确数据驱动需求后，再考虑：

- 自定义 Flow / Masonry 风格布局
- 基于宽度断点动态改变跨度

## 8.3 断点建议

- `>= 1440`：4 列
- `>= 1100 && < 1440`：3 列
- `>= 760 && < 1100`：2 列
- `< 760`：1 列

## 8.4 卡片规范

每个 Bento 卡片应统一具备：

- 标题
- 描述
- meta 信息
- hover / pressed 状态
- 支持强调 variant（如 highlight / accent）

建议属性：

- `title`
- `description`
- `meta`
- `variant`
- `clickable`
- `onClicked`

---

## 9. 资源组织方案

## 9.1 资源管理目标

停止依赖 `src/resources/qss/dark.qss` 这种相对路径磁盘读取方式，统一改为 Qt 资源系统或 QML Module 打包。

## 9.2 推荐方案

### 方案一：`qt_add_qml_module()`

推荐作为主方案，管理：

- `.qml`
- `.js`
- 图片图标
- 字体

### 方案二：`.qrc`

用于：

- 非 QML 类资源
- 兼容旧 QSS / 图标 / 翻译文件

## 9.3 资源迁移建议

### 第一阶段

- QML 文件通过 `qt_add_qml_module(...)` 打包
- 旧 `qss/` 保留，不再作为新 UI 主依赖
- `icons/` 纳入资源系统

### 第二阶段

- 若存在可复用配色，可将旧 QSS 中的颜色整理成主题 token 文档
- 将 QSS 完全降级为兼容遗留层

---

## 10. CMake 改造方案

## 10.1 最低改造目标

`CMakeLists.txt` 需要完成以下调整：

1. `find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick)`
2. target 链接加入：
   - `Qt6::Qml`
   - `Qt6::Quick`
3. 新增 QML 模块定义：
   - `qt_add_qml_module(IQtools ... )`
4. 将桥接层 cpp/h 文件纳入 target

## 10.2 模块命名建议

建议 QML 模块 URI：

```text
IQtools
```

版本：

```text
1.0
```

这样 QML 内部结构更清晰，未来若拆子模块也更方便。

---

## 11. 分阶段实施计划

## Phase 0：方案冻结与目录预留

### 目标

冻结迁移边界，避免一边写 QML 一边改架构。

### 任务

1. 建立 `app/shell/qml/` 目录
2. 建立 `app/shell/bridge/` 目录
3. 明确 QML 模块 URI
4. 确定桥接对象命名
5. 确认是否保留 Widgets fallback

### 输出物

- 目录骨架
- CMake 改造草案
- QML 模块命名规范

---

## Phase 1：跑通最小 QML 应用链路

### 目标

先让程序以 QML 成功显示一个基础窗口。

### 任务

1. CMake 引入 Quick/Qml
2. 新建 `main.qml`
3. Bootstrap 引入 `QQmlApplicationEngine`
4. 程序启动后显示基础空白/占位窗口
5. 保证日志初始化、核心初始化仍然有效

### 验收标准

- 程序可启动
- QML 根窗口能显示
- 日志仍正常写入
- 没有因 QML 引擎加载失败导致崩溃

---

## Phase 2：建立桥接层

### 目标

让 QML 可以读到核心状态并执行最基本交互。

### 任务

1. 新建 `ToolListModel`
2. 新建 `ThemeController`
3. 新建 `NavigationController`
4. 新建 `AppFacade`
5. 在 Bootstrap 中注入这些对象

### 验收标准

- QML 可读取工具列表
- QML 可切换主题状态
- QML 可根据路由切页
- 主题变化可实时反映到界面

---

## Phase 3：完成主框架迁移

### 目标

替换当前 Widgets 主窗口结构。

### 任务

1. 实现 `AppWindow.qml`
2. 实现 Sidebar
3. 实现页面容器
4. 接通导航控制器
5. 完成首页、设置页基本可用版本

### 验收标准

- 左导航 + 右内容布局完整
- 页面切换正常
- 深浅主题正常工作
- 首屏视觉达到可演示水平

---

## Phase 4：迁移 Bento 首页

### 目标

把现有首页升级为真正的 QML Bento 体验。

### 任务

1. 抽象 `BentoCard.qml`
2. 实现首页卡片矩阵
3. 增加 hover / 点击反馈
4. 响应不同窗口宽度
5. 将 ToolListModel 与首页数据绑定

### 验收标准

- 卡片布局清晰
- 多断点下无明显布局错乱
- 视觉风格统一
- 关键卡片支持强调样式

---

## Phase 5：清理遗留 Widgets 依赖

### 目标

在 QML 壳层稳定后，逐步下线旧 Widgets Shell。

### 任务

1. 移除 `MainWindow` 的主入口职责
2. 评估 `ThemeManager` 是否保留兼容能力
3. 清理未使用的 Widgets UI 代码
4. 评估是否从 `QApplication` 切换到 `QGuiApplication`

### 验收标准

- 主流程不再依赖 `QMainWindow`
- 无冗余 UI 初始化路径
- 构建链路清晰

---

## 12. 风险与应对

## 12.1 风险：一次性替换导致启动失败

### 应对

- 保留旧 Widgets 代码一段时间
- 分阶段提交
- 每一阶段先保证“能启动”再追求“完整迁移”

## 12.2 风险：主题系统重构范围失控

### 应对

- 新增 `ThemeController`，不要第一步就彻底推翻旧 `ThemeManager`
- 先只解决 QML token 和切换，不立即做用户自定义主题编辑器

## 12.3 风险：QML 直接依赖 core 对象，后续难维护

### 应对

- 通过 bridge/facade 隔离
- QML 不直接操作复杂后端服务

## 12.4 风险：Bento 布局过早追求复杂自适应，导致开发成本高

### 应对

- 先静态跨度网格
- 再逐步演进为动态布局

---

## 13. 验证与验收清单

每完成一个阶段，至少验证以下内容：

### 13.1 启动验证

- 应用可正常启动
- QML 根组件能正确加载
- 无空白窗口或加载错误

### 13.2 日志验证

- 启动日志仍正常输出
- 关键页面切换有日志记录

### 13.3 主题验证

- Dark / Light 可切换
- 切换后主要颜色、文字、背景同步更新
- 无明显不可读文本

### 13.4 导航验证

- 左侧导航选中状态正确
- 页面切换与路由一致
- 设置页和首页至少完整可用

### 13.5 模型验证

- ToolListModel 数据完整
- QML 中 role 名称可正确访问
- 无越界访问或空数据异常

### 13.6 工程验证

- CMake 配置清晰
- 资源不依赖易失的相对文件路径
- `lsp_diagnostics` 无新增严重错误

---

## 14. 建议的第一批实际落地任务

如果按执行优先级排序，建议下一步直接做以下 8 项：

1. 修改 `CMakeLists.txt`，加入 `Qt6::Qml` 与 `Qt6::Quick`
2. 建立 `src/app/shell/qml/main.qml`
3. 在 Bootstrap 中引入 `QQmlApplicationEngine`
4. 保留 `QApplication`，先不急于切换 `QGuiApplication`
5. 新建 `ThemeController`
6. 新建 `ToolListModel`
7. 新建 `NavigationController`
8. 实现最小版 `AppWindow.qml`（左导航 + 右占位内容区）

这样可以在最短路径内完成“从 Widgets 到 QML 的最小可运行闭环”。

---

## 15. 最终结论

这次迁移不是“把 QWidget 代码翻译成 QML”，而是一次**展示层重构**。最合适的方式不是直接重写一切，而是：

1. **保留核心后端**
2. **新增桥接层**
3. **以 QML 重建主壳层**
4. **以主题 token 重建视觉体系**
5. **分阶段清理旧 Widgets 依赖**

对于当前 IQtools 项目，最佳实施路径是：

> 先打通 QML 引擎与桥接层，再完成主框架迁移，最后做 Bento 视觉与自适应优化。

这条路径在质量、速度、风险和后续演进空间之间最平衡。
