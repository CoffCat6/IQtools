# IQtools 日志系统设计

## 目标

日志系统作为 IQtools 的基础设施能力，服务于主程序、内置工具以及后续插件系统。

第一阶段以“简单、统一、可落地”为目标，优先满足：

- 控制台输出
- 文件落盘
- 日志级别管理
- 统一格式化
- 线程安全的基础写入能力

## 架构位置

```text
app -> core -> infra/logging
plugins -> plugin-api -> core/log service -> infra/logging
```

说明：

- `infra/logging` 负责日志底层实现
- `core` 负责对日志能力进行统一管理和暴露
- `plugins` 后续通过宿主提供的日志接口写日志，避免直接依赖复杂内部实现

## v1 范围

v1 只做以下能力：

1. 统一日志入口
2. Debug / Info / Warning / Error 四级日志
3. 控制台输出
4. 文件输出
5. Qt Message Handler 接管
6. 基本线程安全保护

## 暂不纳入 v1

- 日志滚动切分
- 异步日志队列
- 远程日志上报
- 崩溃转储集成
- 日志查询 UI
- 结构化 JSON 日志

## 推荐目录

```text
infra/
└─ logging/
   ├─ log_level.h
   ├─ log_entry.h
   ├─ logger.h
   └─ logger.cpp
```

## 职责划分

### Logger

负责：

- 初始化日志系统
- 安装 Qt 全局消息处理器
- 统一格式化日志
- 输出到控制台与文件

### 日志格式建议

建议格式：

```text
[2026-03-22 10:00:00.123] [INFO] [core.plugin] Plugin manager initialized
```

字段建议包括：

- 时间戳
- 日志级别
- 分类标签
- 正文消息

## 模块分类建议

建议统一使用分类字符串：

- `app.bootstrap`
- `app.shell`
- `core.plugin`
- `core.tools`
- `infra.logging`
- `tool.capture`
- `tool.translate`

## 对插件系统的支持

后续插件系统稳定后，建议宿主向插件提供统一日志接口，例如：

- `logDebug(category, message)`
- `logInfo(category, message)`
- `logWarning(category, message)`
- `logError(category, message)`

这样插件无需直接管理日志文件，也便于主程序统一控制输出策略。

## 建议落地顺序

1. 先完成 `infra/logging` 基础类
2. 在应用启动阶段初始化日志系统
3. 让 `core` 与后续模块统一通过日志接口输出
4. 再逐步引入分类日志、日志文件滚动与插件日志接入
