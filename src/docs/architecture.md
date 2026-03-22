# IQtools 架构说明

## 总体架构

IQtools 采用分层架构，核心目标是：

- 先稳定应用内核
- 再逐步扩展工具能力
- 为后续插件化预留清晰边界

## 分层结构

```text
app
core
plugin-api
plugins
infra
```

### app

负责应用入口、主窗口、导航和工具页面承载。

### core

负责应用内核能力，包括：

- AppContext
- ToolRegistry
- 插件管理预留
- 日志服务
- 配置和任务系统预留

### plugin-api

定义插件接入协议与宿主上下文接口。

### plugins

承载具体工具模块，v1 先预留截图和翻译工具目录。

### infra

负责日志、网络、存储、平台适配等基础设施能力。

## v1 当前落地范围

- Qt Widgets 主窗口壳
- AppContext
- ToolRegistry
- LogService
- Logger
- 插件接口预留

## 后续演进方向

1. 接入截图工具
2. 接入翻译工具
3. 完善 PluginManager
4. 增加 Provider 插件机制
5. 增加设置页与插件管理页
