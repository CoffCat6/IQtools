# IQtools 项目进度与 QML 迁移计划

## 1. 目前已完成的工作 (What We've Accomplished)
* **基础架构与体系结构 (Foundation & Architecture)**: 完成了 Qt 6 + C++ 项目的初始脚手架搭建，包括 `src/` 目录结构和 CMake 配置。
* **后端服务 (Backend Services)**: 实现了核心后端系统（日志器 `Logger`、日志服务 `LogService`、应用上下文 `AppContext`、工具注册表 `ToolRegistry`、主题管理器 `ThemeManager`）。
* **插件系统 (Plugin System)**: 设计并完成了插件 API 接口（`IPlugin`、`IToolPlugin`），以支持模块化、可扩展的架构。
* **初始 UI (Initial UI - Widgets)**: 使用 Qt Widgets 和 QSS 构建了第一版 UI，实现了 Bento-grid（便当盒）网格风格、深色/浅色主题双轨制以及导航路由。

## 2. 当前项目调整重点 (Current Pivot)
我们目前的重点是将 UI 表示层从 **Qt Widgets 迁移至 QML / Qt Quick**。在保持坚实的 C++ 后端架构完好无损的前提下，以实现更加现代、流畅且美观的“便当盒 (Bento-grid)”风格界面。

## 3. 下一步工作计划 (What's Next on the Agenda)
* **架构策略 (Architectural Strategy)**: 为 QML 迁移制定详细的设计计划与架构策略。
* **构建系统更新 (Build Updates)**: 修改 `CMakeLists.txt`，引入并链接 `Qt6::Qml` 和 `Qt6::Quick` 模块。
* **应用启动更新 (Application Bootstrap)**: 更新 `main.cpp` 和应用引导（Bootstrap）流程，放弃原先的 `QMainWindow`，转而使用 `QGuiApplication` 和 `QQmlApplicationEngine`。
* **C++/QML 桥接 (C++/QML Bridge)**: 重构后端服务（如 `ThemeManager` 和 `ToolRegistry`），通过属性或模型（例如 `QAbstractListModel`）将必要的数据和方法暴露给 QML 层。
* **QML 界面实现 (QML Implementation)**: 使用纯 QML 全面重写 Bento-grid（便当盒）网格风格界面以及主题切换系统。
