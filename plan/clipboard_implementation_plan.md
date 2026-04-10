# IQtools 剪贴板功能实施计划

**版本**：v1.0  
**日期**：2026-04-10  
**状态**：草案  

---

## 一、背景与需求

### 1.1 需求概述

为 IQtools 应用构建跨平台剪贴板功能模块，支持：
- **读取剪贴板**：获取文本、图片、文件列表
- **写入剪贴板**：写入文本、图片
- **监听剪贴板变化**：实时感知剪贴板内容更新
- **跨平台支持**：Windows、Linux、macOS

### 1.2 现状分析

**现有架构优势**：
- 已规划 `src/infra/clipboard/` 目录（当前为空）
- 已规划 `src/infra/platform/` 作为平台抽象层（当前为空）
- 已建立 Qt 服务/插件模式：`AppContext` → 服务注册、`IPluginContext` → 插件访问
- 已有统一日志系统：`LogService`
- 已有设置管理系统：`SettingsManager`

**设计原则**：
- 遵循现有的**服务定位器 + 接口抽象** 模式
- 平台特定代码集中在 `src/infra/platform/`
- 核心业务逻辑在 `src/core/`，提供抽象接口
- UI 层通过 `bridge` 控制器调用

---

## 二、架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────────┐
│ UI 层 (QML / Widgets)                                               │
│ ├─ ClipboardPage.qml / ClipboardController                          │
└─────────────────────────────────────────────────────────────────────┘
                              │
                    (通过 AppFacade)
                              │
┌─────────────────────────────────────────────────────────────────────┐
│ Bridge 层 (src/app/shell/bridge/)                                   │
│ ├─ clipboard_controller.h/cpp  (QObject，信号/槽暴露给 QML)        │
│ └─ 与 CoreClipboardService 通信                                      │
└─────────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────────┐
│ Core 层 (src/core/clipboard/)                                       │
│ ├─ i_clipboard_service.h       (抽象接口)                           │
│ ├─ clipboard_service.h/cpp     (平台无关的业务逻辑)                 │
│ └─ clipboard_monitor.h/cpp     (剪贴板变化监听，信号发送)           │
└─────────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────────┐
│ Platform 层 (src/infra/platform/)                                   │
│ ├─ i_platform_clipboard.h      (平台抽象接口)                       │
│ ├─ windows/clipboard_windows.h/cpp                                  │
│ ├─ linux/clipboard_linux.h/cpp                                      │
│ └─ macos/clipboard_macos.h/cpp                                      │
└─────────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────────┐
│ Infrastructure 层 (src/infra/clipboard/)                            │
│ ├─ clipboard_data_types.h      (数据结构定义)                       │
│ ├─ clipboard_constants.h        (常量定义)                          │
│ └─ clipboard_utils.h/cpp        (工具函数)                          │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 关键接口定义

#### 2.2.1 基础数据结构（`clipboard_data_types.h`）

```cpp
namespace iqtools::infra::clipboard {

// 剪贴板内容类型枚举
enum class ClipboardContentType {
    Text,       // 纯文本
    Html,       // HTML 文本
    Image,      // 图片
    Files,      // 文件列表
    Custom,     // 自定义格式
    Unknown     // 未知
};

// 剪贴板项目结构体
struct ClipboardItem {
    ClipboardContentType type;
    QByteArray data;           // 原始数据
    QString mimeType;          // MIME 类型
    QDateTime timestamp;       // 获取时间戳
    
    // 便利方法
    QString getTextData() const;
    QPixmap getImageData() const;
    QStringList getFileListData() const;
};

// 剪贴板内容快照（用于变化检测）
struct ClipboardSnapshot {
    ClipboardContentType type;
    QString textHash;          // 文本内容的哈希，用于快速对比
    QString imageHash;         // 图片的哈希
    QDateTime timestamp;
};

// 剪贴板配置
struct ClipboardConfig {
    bool monitorEnabled {true};
    int monitorIntervalMs {500};    // 监听间隔
    int maxHistorySize {100};       // 历史记录最大条数
    bool autoTrimLargeImage {true}; // 自动裁剪超大图片
    int maxImageWidth {4096};
    int maxImageHeight {4096};
    bool persistHistory {false};     // 是否持久化历史
};

}
```

#### 2.2.2 平台抽象接口（`src/infra/platform/i_platform_clipboard.h`）

```cpp
namespace iqtools::infra::platform {

class IPlatformClipboard {
public:
    virtual ~IPlatformClipboard() = default;

    /// 获取剪贴板内容
    virtual ClipboardItem getClipboardContent() const = 0;

    /// 设置文本到剪贴板
    virtual bool setTextToClipboard(const QString& text) = 0;

    /// 设置图片到剪贴板
    virtual bool setImageToClipboard(const QPixmap& pixmap) = 0;

    /// 设置文件列表到剪贴板
    virtual bool setFilesToClipboard(const QStringList& filePaths) = 0;

    /// 清空剪贴板
    virtual bool clearClipboard() = 0;

    /// 获取剪贴板是否为空
    virtual bool isEmpty() const = 0;

    /// 获取剪贴板当前内容的快照（用于变化检测）
    virtual ClipboardSnapshot getSnapshot() const = 0;

    /// 返回平台标识符
    virtual QString platformId() const = 0;
};

}
```

#### 2.2.3 Core 层抽象接口（`src/core/clipboard/i_clipboard_service.h`）

```cpp
namespace iqtools::core {

class IClipboardService : public QObject {
    Q_OBJECT

public:
    virtual ~IClipboardService() = default;

    /// 获取剪贴板当前内容
    virtual ClipboardItem getContent() const = 0;

    /// 设置文本到剪贴板
    virtual bool setText(const QString& text) = 0;

    /// 设置图片到剪贴板
    virtual bool setImage(const QPixmap& pixmap) = 0;

    /// 设置文件列表到剪贴板
    virtual bool setFiles(const QStringList& filePaths) = 0;

    /// 清空剪贴板
    virtual bool clear() = 0;

    /// 获取剪贴板历史记录
    virtual QVector<ClipboardItem> getHistory(int limit = 10) const = 0;

    /// 恢复历史项目到剪贴板
    virtual bool restoreFromHistory(int index) = 0;

    /// 启动/停止监听
    virtual void startMonitoring() = 0;
    virtual void stopMonitoring() = 0;

    /// 配置
    virtual void setConfig(const ClipboardConfig& config) = 0;
    virtual ClipboardConfig config() const = 0;

signals:
    /// 剪贴板内容变化时触发
    void contentChanged(const ClipboardItem& item);

    /// 监听状态变化
    void monitoringStatusChanged(bool enabled);

    /// 错误信息
    void errorOccurred(const QString& error);
};

}
```

#### 2.2.4 Core 业务实现（`src/core/clipboard/clipboard_service.h`）

```cpp
namespace iqtools::core {

class ClipboardService : public IClipboardService {
    Q_OBJECT

public:
    ClipboardService(std::unique_ptr<infra::platform::IPlatformClipboard> platformImpl,
                     LogService* logService = nullptr,
                     QObject* parent = nullptr);

    ClipboardItem getContent() const override;
    bool setText(const QString& text) override;
    bool setImage(const QPixmap& pixmap) override;
    bool setFiles(const QStringList& filePaths) override;
    bool clear() override;
    QVector<ClipboardItem> getHistory(int limit = 10) const override;
    bool restoreFromHistory(int index) override;
    void startMonitoring() override;
    void stopMonitoring() override;
    void setConfig(const ClipboardConfig& config) override;
    ClipboardConfig config() const override;

private slots:
    void onMonitorTick();

private:
    std::unique_ptr<infra::platform::IPlatformClipboard> m_platformImpl;
    std::unique_ptr<ClipboardMonitor> m_monitor;
    QVector<ClipboardItem> m_history;
    ClipboardConfig m_config;
    LogService* m_logService;
};

}
```

#### 2.2.5 监听组件（`src/core/clipboard/clipboard_monitor.h`）

```cpp
namespace iqtools::core {

class ClipboardMonitor : public QObject {
    Q_OBJECT

public:
    ClipboardMonitor(infra::platform::IPlatformClipboard* platformImpl,
                     QObject* parent = nullptr);

    void start(int intervalMs = 500);
    void stop();
    bool isRunning() const;

signals:
    void contentChanged(const ClipboardItem& item);
    void monitoringStatusChanged(bool running);

private slots:
    void checkClipboard();

private:
    bool isContentChanged(const infra::clipboard::ClipboardSnapshot& newSnapshot) const;

    infra::platform::IPlatformClipboard* m_platformImpl;
    QTimer* m_timer;
    infra::clipboard::ClipboardSnapshot m_lastSnapshot;
    bool m_isRunning;
};

}
```

#### 2.2.6 Bridge 控制器（`src/app/shell/bridge/clipboard_controller.h`）

```cpp
namespace iqtools::app::bridge {

class ClipboardController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool monitoringEnabled READ isMonitoringEnabled WRITE setMonitoringEnabled NOTIFY monitoringEnabledChanged)
    Q_PROPERTY(QString lastContentType READ lastContentType NOTIFY contentTypeChanged)

public:
    ClipboardController(iqtools::core::IClipboardService* service, QObject* parent = nullptr);

    bool isMonitoringEnabled() const;
    void setMonitoringEnabled(bool enabled);

    QString lastContentType() const;

    // 供 QML 调用的方法
    Q_INVOKABLE QString getTextContent();
    Q_INVOKABLE bool setTextContent(const QString& text);
    Q_INVOKABLE bool setImageContent(const QString& imagePath);
    Q_INVOKABLE QStringList getFileListContent();
    Q_INVOKABLE bool setFileListContent(const QStringList& filePaths);
    Q_INVOKABLE bool clearClipboard();
    Q_INVOKABLE QStringList getHistory(int limit = 10);
    Q_INVOKABLE bool restoreFromHistory(int index);

signals:
    void contentChanged(const QString& type, const QString& preview);
    void contentTypeChanged(const QString& type);
    void monitoringEnabledChanged(bool enabled);
    void errorOccurred(const QString& message);

private slots:
    void onServiceContentChanged(const ClipboardItem& item);
    void onServiceError(const QString& error);

private:
    QString describeContent(const ClipboardItem& item) const;

    iqtools::core::IClipboardService* m_service;
    QString m_lastContentType;
};

}
```

---

## 三、实施步骤

### 阶段 1：基础设施建设（第1-2周）

#### 步骤 1.1：定义数据结构与常量
**文件**：
- `src/infra/clipboard/clipboard_data_types.h`
- `src/infra/clipboard/clipboard_constants.h`

**工作内容**：
- 定义 `ClipboardContentType` 枚举
- 定义 `ClipboardItem`、`ClipboardSnapshot`、`ClipboardConfig` 结构体
- 定义常量（MIME 类型、错误码等）

**验收标准**：
- 数据结构完整、无编译错误
- 文档清晰

---

#### 步骤 1.2：实现工具函数
**文件**：
- `src/infra/clipboard/clipboard_utils.h/cpp`

**工作内容**：
- 内容哈希计算（用于变化检测）
- 数据序列化/反序列化
- MIME 类型判断
- 日志辅助函数

**验收标准**：
- 单元测试覆盖主要场景
- 跨平台兼容性验证

---

#### 步骤 1.3：定义平台抽象接口
**文件**：
- `src/infra/platform/i_platform_clipboard.h`

**工作内容**：
- 定义 `IPlatformClipboard` 抽象接口
- 定义异常/错误处理机制
- 添加接口文档

**验收标准**：
- 接口设计符合项目架构
- 能覆盖所有平台需求

---

### 阶段 2：平台实现（第2-3周）

#### 步骤 2.1：Windows 平台实现
**文件**：
- `src/infra/platform/windows/clipboard_windows.h/cpp`

**技术方案**：
- 使用 Qt 内置 `QClipboard`（主要）
- 使用 Windows API `OpenClipboard()` / `GetClipboardData()` 等（特定场景，如高级格式支持）

**工作内容**：
- 实现 `IPlatformClipboard` 接口
- 处理文本、图片、文件列表读写
- 错误处理（剪贴板被锁定等）

**验收标准**：
- 所有核心方法实现
- 单元测试通过
- 在 Windows 10+ 环境测试

---

#### 步骤 2.2：Linux 平台实现
**文件**：
- `src/infra/platform/linux/clipboard_linux.h/cpp`

**技术方案**：
- 使用 Qt 内置 `QClipboard`（优先）
- 必要时支持 XClipboard（选择缓冲区）

**工作内容**：
- 实现 `IPlatformClipboard` 接口
- 处理文本、图片、文件列表读写
- 选择缓冲区支持（可选）

**验收标准**：
- 所有核心方法实现
- 在 Ubuntu/Debian 环境测试

---

#### 步骤 2.3：macOS 平台实现
**文件**：
- `src/infra/platform/macos/clipboard_macos.h/cpp`

**技术方案**：
- 使用 Qt 内置 `QClipboard`（主要）
- 使用 Cocoa API（特殊需求）

**工作内容**：
- 实现 `IPlatformClipboard` 接口
- 处理文本、图片、文件列表读写

**验收标准**：
- 所有核心方法实现
- 在 macOS 11+ 环境测试

---

#### 步骤 2.4：平台工厂与注册
**文件**：
- `src/infra/platform/clipboard_platform_factory.h/cpp`

**工作内容**：
- 实现工厂函数 `createPlatformClipboard()`
- 根据编译平台选择实现
- 注册到 `AppContext`

**验收标准**：
- 工厂函数正确返回平台实现
- 集成测试通过

---

### 阶段 3：Core 层实现（第3-4周）

#### 步骤 3.1：实现 ClipboardService
**文件**：
- `src/core/clipboard/i_clipboard_service.h`
- `src/core/clipboard/clipboard_service.h/cpp`

**工作内容**：
- 实现 `IClipboardService` 接口
- 集成 `IPlatformClipboard`
- 实现历史记录管理
- 集成 `LogService`
- 错误处理与日志记录

**验收标准**：
- 所有接口方法实现
- 单元测试覆盖 >80%
- 历史记录功能正常

---

#### 步骤 3.2：实现 ClipboardMonitor
**文件**：
- `src/core/clipboard/clipboard_monitor.h/cpp`

**工作内容**：
- 实现后台监听线程/定时器
- 实现变化检测逻辑
- 发送 `contentChanged` 信号
- 可配置的监听间隔

**验收标准**：
- 能正确检测剪贴板变化
- 不造成过度 CPU 占用（<5% 在空闲时）
- 单元测试通过

---

#### 步骤 3.3：集成到 AppContext
**文件**：
- `src/core/appcontext/app_context.h/cpp`

**工作内容**：
- 在 `AppContext` 中注册 `ClipboardService`
- 在 `application_bootstrap.cpp` 中初始化

**验收标准**：
- `AppContext` 可正确访问 `ClipboardService`
- 应用启动时自动初始化

---

### 阶段 4：UI 层实现（第4-5周）

#### 步骤 4.1：实现 ClipboardController
**文件**：
- `src/app/shell/bridge/clipboard_controller.h/cpp`

**工作内容**：
- 实现 `ClipboardController` QObject
- 暴露信号/槽给 QML
- 实现便利方法（文本/图片/文件操作）
- 监听 `ClipboardService` 的信号

**验收标准**：
- 所有 `Q_INVOKABLE` 方法正常工作
- 信号正确发送
- 与 QML 集成测试通过

---

#### 步骤 4.2：创建 ClipboardPage UI
**文件**：
- `src/app/shell/qml/pages/ClipboardPage.qml`
- 相关组件

**工作内容**：
- 设计 UI 布局
- 实现文本输入/显示
- 实现图片预览
- 实现历史记录列表
- 实现监听开关

**验收标准**：
- UI 响应式设计
- 所有功能可交互
- 与 `ClipboardController` 正确通信

---

#### 步骤 4.3：集成到 AppFacade
**文件**：
- `src/app/shell/bridge/app_facade.h/cpp`

**工作内容**：
- 在 `AppFacade` 中添加 `ClipboardController`
- 在主窗口初始化时创建实例
- 暴露给 QML

**验收标准**：
- QML 中可访问 `clipboardController`
- 集成测试通过

---

### 阶段 5：测试与优化（第5-6周）

#### 步骤 5.1：单元测试
**文件**：
- `src/tests/core/test_clipboard_service.cpp`
- `src/tests/infra/test_platform_clipboard.cpp`
- `src/tests/app/test_clipboard_controller.cpp`

**工作内容**：
- 编写单元测试
- 测试覆盖率 >80%
- 测试场景包括：
  - 基础读写操作
  - 变化检测
  - 历史记录
  - 错误处理
  - 跨平台兼容性

**验收标准**：
- 所有测试通过
- 覆盖率 >80%

---

#### 步骤 5.2：集成测试
**工作内容**：
- 端到端测试（UI → Core → Platform）
- 跨平台测试
- 性能测试（内存占用、CPU、响应时间）
- 压力测试（大量数据、频繁变化）

**验收标准**：
- 所有集成测试通过
- 性能指标达标

---

#### 步骤 5.3：代码审查与文档
**工作内容**：
- 代码审查
- 编写使用文档
- 编写开发文档
- 更新 `architecture.md`

**验收标准**：
- 代码质量满足项目规范
- 文档完整清晰

---

#### 步骤 5.4：优化与调整
**工作内容**：
- 性能优化
- 内存泄漏检查
- 用户体验改进
- Bug 修复

**验收标准**：
- 无内存泄漏
- 性能指标达标
- 用户体验良好

---

## 四、关键技术决策

### 4.1 为什么使用 Qt 内置 QClipboard？

**优点**：
- ✅ 跨平台一致性最好
- ✅ 集成度高，无额外依赖
- ✅ 维护成本低
- ✅ 性能稳定

**缺点**：
- ⚠️ 高级功能支持有限
- ⚠️ 某些特殊格式需要平台 API

**决策**：
- 优先使用 `QClipboard`
- 需要时通过平台 API 补充

---

### 4.2 监听机制：定时轮询 vs 事件通知

**方案对比**：

| 方案 | 优点 | 缺点 | 适用场景 |
|------|------|------|---------|
| 定时轮询 | 简单可靠、跨平台 | CPU 占用、响应延迟 | 通用、简单 |
| 原生通知 | 响应快、节能 | 平台相关、复杂 | 特定平台优化 |

**决策**：
- **当前**：使用定时轮询（简单、可靠）
- **后续优化**：支持平台原生通知（可选）

---

### 4.3 历史记录存储

**方案对比**：

| 方案 | 优点 | 缺点 | 选择 |
|------|------|------|------|
| 内存 | 快速、简单 | 应用关闭丢失 | ✅ 当前 |
| 文件 | 持久化 | 性能、隐私 | 未来可选 |
| 数据库 | 灵活、强大 | 复杂度高 | 未来考虑 |

**决策**：
- 当前仅保存到内存（应用运行期间）
- 配置项支持后续扩展

---

## 五、接口契约与集成指南

### 5.1 Service 获取

```cpp
// 从 AppContext 获取 ClipboardService
auto& context = app_context;  // 通过 DI 或单例模式获取
auto* clipboardService = dynamic_cast<iqtools::core::ClipboardService*>(
    context.getService("clipboard")  // 假设已注册
);
```

### 5.2 QML 中的使用

```qml
// 访问 ClipboardController（通过 AppFacade）
Text {
    text: appFacade.clipboardController.lastContentType
}

// 调用方法
Button {
    onClicked: {
        var success = appFacade.clipboardController.setTextContent("Hello")
        if (success) {
            console.log("Text copied to clipboard")
        }
    }
}

// 监听信号
Connections {
    target: appFacade.clipboardController
    onContentChanged: (type, preview) => {
        console.log("Clipboard changed: " + type)
    }
}
```

### 5.3 C++ 中的使用

```cpp
// 获取当前剪贴板内容
auto item = clipboardService->getContent();
if (item.type == ClipboardContentType::Text) {
    auto text = item.getTextData();
    // 处理文本
}

// 写入剪贴板
bool success = clipboardService->setText("Copy this text");

// 监听变化
connect(clipboardService, &IClipboardService::contentChanged,
        this, [](const ClipboardItem& item) {
            qDebug() << "Clipboard changed to:" << item.mimeType;
        });
```

---

## 六、跨平台特殊处理

### 6.1 Windows

**特殊考虑**：
- 文件路径格式（反斜杠）
- Unicode 编码（UTF-16）
- 文件关联与拖拽

**实现位置**：
- `src/infra/platform/windows/clipboard_windows.cpp`

---

### 6.2 Linux

**特殊考虑**：
- X11 vs Wayland（需兼容）
- 多个剪贴板（Clipboard / Primary / Secondary）
- 文件管理器集成

**实现位置**：
- `src/infra/platform/linux/clipboard_linux.cpp`

---

### 6.3 macOS

**特殊考虑**：
- Cocoa 事件循环集成
- 文件权限（macOS 13+）
- URL 类型处理

**实现位置**：
- `src/infra/platform/macos/clipboard_macos.cpp`

---

## 七、风险与缓解

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 平台兼容性问题 | 高 | 中 | 提前跨平台测试、预留适配空间 |
| 性能问题（监听） | 中 | 低 | 可配置轮询间隔、异步处理 |
| 大数据处理 | 中 | 低 | 大小限制、智能缓存 |
| 权限问题 | 低 | 低 | 错误处理、用户提示 |

---

## 八、文件清单

### 新增文件

```
src/
├── infra/
│   ├── clipboard/
│   │   ├── clipboard_data_types.h
│   │   ├── clipboard_constants.h
│   │   └── clipboard_utils.h/cpp
│   └── platform/
│       ├── i_platform_clipboard.h
│       ├── clipboard_platform_factory.h/cpp
│       ├── windows/
│       │   └── clipboard_windows.h/cpp
│       ├── linux/
│       │   └── clipboard_linux.h/cpp
│       └── macos/
│           └── clipboard_macos.h/cpp
├── core/
│   └── clipboard/
│       ├── i_clipboard_service.h
│       ├── clipboard_service.h/cpp
│       └── clipboard_monitor.h/cpp
└── app/
    └── shell/
        ├── bridge/
        │   └── clipboard_controller.h/cpp
        └── qml/
            └── pages/
                └── ClipboardPage.qml
tests/
├── core/
│   └── test_clipboard_service.cpp
├── infra/
│   └── test_platform_clipboard.cpp
└── app/
    └── test_clipboard_controller.cpp

plan/
└── clipboard_implementation_plan.md (本文件)
```

### 修改文件

```
src/
├── core/
│   ├── appcontext/
│   │   ├── app_context.h (添加 clipboard service)
│   │   └── app_context.cpp
│   └── services/ (可能需要依赖关系调整)
├── app/
│   ├── bootstrap/
│   │   └── application_bootstrap.cpp (初始化 clipboard service)
│   └── shell/
│       ├── bridge/
│       │   ├── app_facade.h (添加 clipboard controller)
│       │   ├── app_facade.cpp
│       │   └── tool_list_model.h (可能需要添加 clipboard tool)
│       └── qml/
│           └── main.qml (添加 ClipboardPage 导航)
CMakeLists.txt (添加新文件编译规则)
```

---

## 九、验收标准

### 功能验收

- ✅ 所有平台（Windows/Linux/macOS）剪贴板读写正常
- ✅ 文本、图片、文件列表支持完整
- ✅ 剪贴板变化监听实时响应
- ✅ 历史记录功能可用
- ✅ 配置项生效
- ✅ UI 交互流畅

### 质量验收

- ✅ 单元测试覆盖率 >80%
- ✅ 集成测试全通过
- ✅ 无内存泄漏
- ✅ CPU 占用 <5% 空闲时
- ✅ 代码审查通过
- ✅ 文档完整

### 性能验收

- ✅ 文本操作 <100ms
- ✅ 图片操作 <500ms
- ✅ 监听延迟 <1s
- ✅ 历史列表加载 <200ms

---

## 十、后续规划

### Phase 2（可选）

- [ ] 持久化历史记录到文件
- [ ] 压缩/加密历史数据
- [ ] 剪贴板搜索功能
- [ ] 快速预设（常用文本）
- [ ] 浮窗快速访问

### Phase 3（可选）

- [ ] 同步到云端
- [ ] 跨设备剪贴板
- [ ] 高级格式支持（Rich Text、表格等）
- [ ] 自动分类和标签

---

## 十一、相关文档

- `src/docs/architecture.md` - 项目架构
- `src/docs/plugin-system.md` - 插件系统
- 本计划文件位置：`plan/clipboard_implementation_plan.md`

---

**编写人**：开发团队  
**最后更新**：2026-04-10  
**审核状态**：待审核
