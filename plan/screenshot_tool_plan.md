# IQtools 截图插件开发计划 v2.0

> **方案**：QML 主界面入口 + C++ Qt Widgets 截图覆盖层（方案 A）  
> **技术栈**：C++17 · Qt 6.5.3 LTS · QML · Qt Widgets · QHotkey · CMake 3.25+  
> **目标平台**：Windows 11 · Ubuntu 22.04 (X11 / Wayland) · macOS 13+  
> **总工期**：7 个工作日  
> **核心原则**：零改动现有 QML 主壳 · 最小侵入 · 原生稳定 · 插件化集成

## 当前落地进展（2026-04-10）

### 已完成（MVP）

- [x] 新增 `ScreenCaptureService`：支持多屏全屏拼接截图
- [x] 新增 `CaptureController`：支持立即截图、延时截图、状态反馈
- [x] 截图结果支持保存为 PNG（时间戳文件名）
- [x] 截图结果支持复制到系统剪贴板（可开关）
- [x] 启动流程已注入 `captureController` 并暴露给 QML
- [x] 默认快捷键 `Alt+C`（ID: `tool.capture`）已接入快捷键管理
- [x] `CapturePage.qml` 从占位页面升级为可操作页面
- [x] 新增截图设置持久化项：输出目录 / 自动复制 / 延时秒数
- [x] 新增区域截图覆盖层：拖拽框选 + 八锚点微调 + Enter 确认 / Esc 取消
- [x] 新增截图输出倍率配置（50%~200%）
- [x] 新增基础标注工具链：矩形 / 箭头 / 马赛克 / 文字
- [x] 新增标注增强：撤销/重做、样式调节、可点击工具栏
- [x] 插件化重构第一步：抽象 `ICaptureService` 并由 `CapturePlugin` 提供服务注入控制器

### 待完善（下一阶段）
- [x] 标注链路深度增强（样式持久化、快捷键自定义、工具栏图标化）
- [x] 标注体验打磨（快捷键冲突检测、工具栏矢量图标资源）
- [x] 标注设置体验优化（快捷键录制输入框、冲突高亮提示）
- [x] 插件化重构第二步（动态加载与插件生命周期托管）
- [x] Wayland Portal 与 macOS 权限适配
- [x] 贴图 Pin 窗口与托盘快速截图

---

## 目录

1. [方案定位与职责边界](#1-方案定位与职责边界)
2. [目录结构](#2-目录结构)
3. [阶段 0 · 集成环境搭建（0.5 天）](#3-阶段-0--集成环境搭建05-天)
4. [阶段 1 · 核心截图与框选（1.5 天）](#4-阶段-1--核心截图与框选15-天)
5. [阶段 2 · 标注工具与交互（2 天）](#5-阶段-2--标注工具与交互2-天)
6. [阶段 3 · 系统功能集成（1.5 天）](#6-阶段-3--系统功能集成15-天)
7. [阶段 4 · 多平台适配与发布（1 天）](#7-阶段-4--多平台适配与发布1-天)
8. [QML ↔ C++ 完整交互协议](#8-qml--c-完整交互协议)
9. [完整类设计与接口规范](#9-完整类设计与接口规范)
10. [数据流与状态机设计](#10-数据流与状态机设计)
11. [风险与对策](#11-风险与对策)
12. [进度检查点与验收标准](#12-进度检查点与验收标准)

---

## 1. 方案定位与职责边界

### 核心分工

| 模块 | 技术 | 职责 | 禁止操作 |
|---|---|---|---|
| 主程序入口 | QML (Qt Quick) | 主界面、触发按钮、设置页、状态 Toast | 不处理截图、绘制、鼠标/键盘事件 |
| 截图核心 | C++ Qt Widgets | 全屏覆盖、抓屏、八锚点框选、标注、热键 | 不渲染主界面 UI |
| 公共服务 | IQtools Core | 日志、配置、剪贴板、插件管理 | — |

### 为何选择方案 A（QML 入口 + Widget 覆盖层）

**Tauri/WebView 方案的核心问题**在于：全屏无边框透明窗口在 WebView 渲染上下文中，跨平台的事件穿透与 compositing 极不稳定。Qt Widgets 的 `paintEvent` 直接面向底层图形驱动，绕过所有中间层，是截图工具唯一合理的底层实现选择。

QML 主界面与 Widget 覆盖层的共存模式已在 Flameshot、Snipaste 等成熟项目中被验证：两者运行在同一进程、同一 Qt 事件循环中，通过信号/槽通信，**无需跨进程 IPC，延迟极低**。

### MVP 功能清单

| 功能 | 优先级 | 验收标准 |
|---|---|---|
| 多屏全屏截图 + 虚拟桌面拼接 | P0 | 双屏/三屏截图无黑边，坐标精确 |
| 矩形框选 + 八锚点微调 | P0 | 像素级精度，反向拖拽正常 |
| 标注：矩形 / 箭头 / 马赛克 / 文字 | P0 | 四种工具可用，撤销/重做正常 |
| 全局热键唤醒 | P0 | `Alt+C` 默认绑定，支持用户自定义 |
| 复制到剪贴板 / 保存文件 | P0 | PNG/JPEG，文件名含时间戳 |
| 系统托盘常驻 | P1 | 右键菜单快速截图，双击打开主界面 |
| 贴图（Pin）悬浮窗 | P2 | 置顶、鼠标拖动、滚轮缩放、右键关闭 |
| 截图延时功能 | P2 | 1 / 3 / 5 秒可选 |

---

## 2. 目录结构

严格适配 IQtools 现有结构，**无破坏性改动**：

```
IQtools/
├── CMakeLists.txt                      # 主构建文件（新增 capture 子目录引用）
├── src/
│   ├── main.cpp                        # QML 主程序入口（无改动）
│   ├── app/
│   │   ├── AppManager.{h,cpp}          # 单例，暴露 C++ 对象给 QML
│   │   └── PluginLoader.{h,cpp}        # 插件加载器
│   ├── qml/                            # 现有 QML 主界面（仅新增触发入口）
│   │   ├── Main.qml
│   │   └── pages/
│   │       └── CapturePage.qml         # ★ 新增：截图设置/入口页
│   ├── core/                           # 复用 IQtools 核心模块（无改动）
│   │   ├── Logger.{h,cpp}
│   │   ├── ConfigManager.{h,cpp}
│   │   └── ThemeManager.{h,cpp}
│   ├── plugin-api/
│   │   └── ICapturePlugin.h            # ★ 新增：插件接口定义
│   └── plugins/
│       └── capture/                    # ★ 新增：截图插件（纯 C++ Widget）
│           ├── CMakeLists.txt
│           ├── CapturePlugin.{h,cpp}   # 插件入口，实现 ICapturePlugin
│           ├── engine/
│           │   ├── CaptureEngine.{h,cpp}
│           │   └── backends/           # 跨平台截图后端
│           │       ├── ICaptureBackend.h
│           │       ├── WinBackend.{h,cpp}
│           │       ├── X11Backend.{h,cpp}
│           │       ├── WaylandBackend.{h,cpp}
│           │       └── MacBackend.{h,cpp}
│           ├── widget/
│           │   ├── ScreenshotWidget.{h,cpp}
│           │   ├── SelectionRect.{h,cpp}
│           │   └── AnchorHandle.{h,cpp}
│           ├── annotation/
│           │   ├── AnnotationLayer.{h,cpp}
│           │   ├── ToolStateMachine.{h,cpp}
│           │   ├── FloatToolBar.{h,cpp}
│           │   └── shapes/
│           │       ├── ShapeItem.h         # 抽象基类
│           │       ├── RectShape.{h,cpp}
│           │       ├── EllipseShape.{h,cpp}
│           │       ├── ArrowShape.{h,cpp}
│           │       ├── MosaicShape.{h,cpp}
│           │       ├── PenShape.{h,cpp}
│           │       └── TextShape.{h,cpp}
│           └── utils/
│               ├── HotkeyManager.{h,cpp}
│               ├── TrayIcon.{h,cpp}
│               ├── OutputManager.{h,cpp}
│               └── PinWindow.{h,cpp}
├── resources/
│   ├── icons/                          # SVG 图标（支持暗色模式）
│   └── resources.qrc
└── third_party/
    └── QHotkey/                        # FetchContent 自动拉取
```

---

## 3. 阶段 0 · 集成环境搭建（0.5 天）

### 目标

完成 QML ↔ C++ 插件桥接，三平台 CMake 编译通过，CI 骨架就绪。

### 任务清单

- [ ] `AppManager` 单例完成，`startCapture()` 暴露给 QML
- [ ] `ICapturePlugin` 接口定义完成
- [ ] `capture` 插件 CMakeLists.txt 配置完成，链接 `Qt6::Widgets` + QHotkey
- [ ] `FetchContent` 引入 QHotkey，确认三平台编译
- [ ] QML 新增截图触发按钮，绑定 C++ 方法
- [ ] `CapturePage.qml` 骨架创建（设置项占位）
- [ ] GitHub Actions 三平台 runner 配置完成

### CMakeLists.txt 关键片段

```cmake
cmake_minimum_required(VERSION 3.25)
project(IQtools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Quick Gui Concurrent)

include(FetchContent)
FetchContent_Declare(QHotkey
    GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
    GIT_TAG        1.5.0)
FetchContent_MakeAvailable(QHotkey)

# 主程序
add_executable(IQtools WIN32 MACOSX_BUNDLE
    src/main.cpp
    resources/resources.qrc)
target_link_libraries(IQtools PRIVATE Qt6::Quick Qt6::Widgets Qt6::Gui)

# 截图插件（编译为动态库）
add_subdirectory(src/plugins/capture)
```

### QML ↔ C++ 桥接骨架

```cpp
// plugin-api/ICapturePlugin.h
class ICapturePlugin {
public:
    virtual ~ICapturePlugin() = default;
    virtual void startCaptureSession() = 0;
    virtual void startCaptureDelayed(int seconds) = 0;
    virtual void setHotkey(const QString &sequence) = 0;
};

// app/AppManager.h
class AppManager : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void startCapture()                     { m_plugin->startCaptureSession(); }
    Q_INVOKABLE void startCaptureDelayed(int seconds)   { m_plugin->startCaptureDelayed(seconds); }
    Q_INVOKABLE void setHotkey(const QString &seq)      { m_plugin->setHotkey(seq); }

signals:
    void captureCompleted(const QString &imagePath);    // 通知 QML 显示 Toast
    void captureCancelled();

private:
    ICapturePlugin *m_plugin = nullptr;
};

// main.cpp（无改动，仅注册单例）
auto *appManager = new AppManager(&app);
qmlRegisterSingletonInstance<AppManager>("App", 1, 0, "AppManager", appManager);
```

```qml
// qml/pages/CapturePage.qml（新增，不改动现有文件）
import App 1.0

Column {
    Button {
        text: "截图"
        onClicked: AppManager.startCapture()
    }
    Button {
        text: "延时 3 秒截图"
        onClicked: AppManager.startCaptureDelayed(3)
    }
}

Connections {
    target: AppManager
    function onCaptureCompleted(path) {
        toast.show("截图已保存：" + path)
    }
}
```

### 交付物

- QML 按钮可触发 C++ 截图逻辑（日志可见）
- 插件动态库三平台编译通过
- CI 三平台 build 状态绿灯

---

## 4. 阶段 1 · 核心截图与框选（1.5 天）

### 目标

可用的跨平台截图引擎 + 全屏透明 Widget + 八锚点精准框选。

### 任务清单

- [ ] `ICaptureBackend` 接口 + 工厂方法，运行时检测平台切换实现
- [ ] `X11Backend`（优先）：`QScreen::grabWindow` 实现
- [ ] `WaylandBackend`：占位，阶段 4 完善
- [ ] `CaptureEngine`：多屏拼接、DPI 感知、物理像素对齐
- [ ] `ScreenshotWidget`：无边框、置顶、半透明遮罩 paintEvent
- [ ] `SelectionRect`：拖拽框选、反向选区、边界限制
- [ ] `AnchorHandle`：八方向锚点命中测试与拖拽调整
- [ ] 键盘事件：`Esc` 取消、`Enter` 确认、`Ctrl+C` 直接复制

### `ICaptureBackend` 接口与工厂

```cpp
// engine/backends/ICaptureBackend.h
class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;

    // 同步截图（X11 / Win / Mac）
    virtual QPixmap capture() = 0;
    // 异步截图（Wayland Portal），完成后 emit captured()
    virtual bool isAsync() const { return false; }

signals_compat:
    // 子类需要 Q_OBJECT 并 emit 此信号（异步后端用）
    // void captured(QPixmap px);

    // 工厂方法：运行时检测平台
    static ICaptureBackend* create(QObject *parent = nullptr);
};
```

```cpp
// 工厂实现
ICaptureBackend* ICaptureBackend::create(QObject *parent) {
#if defined(Q_OS_WIN)
    return new WinBackend(parent);
#elif defined(Q_OS_MACOS)
    return new MacBackend(parent);
#elif defined(Q_OS_LINUX)
    if (qgetenv("XDG_SESSION_TYPE") == "wayland")
        return new WaylandBackend(parent); // 阶段4完善
    return new X11Backend(parent);
#endif
}
```

### `CaptureEngine` 多屏拼接

```cpp
QPixmap CaptureEngine::captureAll() {
    // Step 1: 计算虚拟桌面总包围盒（处理负坐标/多屏错位）
    QRect total;
    for (auto *s : QGuiApplication::screens())
        total = total.united(s->geometry());
    m_virtualDesktopRect = total;

    // Step 2: 拼接各屏截图（物理像素尺寸）
    qreal maxDpr = 1.0;
    for (auto *s : QGuiApplication::screens())
        maxDpr = qMax(maxDpr, s->devicePixelRatio());

    QPixmap canvas(total.size() * maxDpr);
    canvas.setDevicePixelRatio(maxDpr);
    canvas.fill(Qt::black);

    QPainter p(&canvas);
    for (auto *s : QGuiApplication::screens()) {
        QPixmap shot = m_backend->capture(); // 由后端抓取本屏
        QPoint offset = s->geometry().topLeft() - total.topLeft();
        p.drawPixmap(offset, shot);
    }
    return canvas;
}
```

> **负坐标处理**：`QRect::united()` 自动处理所有屏幕几何的并集，offset 计算时减去 `total.topLeft()`，保证坐标统一为正。Windows 副屏在主屏左侧时坐标为负，此方案完全兼容。

### `ScreenshotWidget` 核心 paintEvent

```cpp
void ScreenshotWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);

    // 1. 背景：完整截图
    p.drawPixmap(0, 0, m_screenshot);

    // 2. 遮罩：整体半透明黑色
    p.fillRect(rect(), QColor(0, 0, 0, 110));

    // 3. 选区：擦出清晰背景（Porter-Duff Clear 合成）
    if (m_selection.hasSelection()) {
        QRect sel = m_selection.rect();
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(sel, m_screenshot, sel);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        // 4. 选区边框
        p.setPen(QPen(QColor(0, 174, 255), 1.5));
        p.drawRect(sel.adjusted(0, 0, -1, -1));

        // 5. 尺寸提示标签（选区左上角）
        drawSizeLabel(p, sel);

        // 6. 八个锚点
        m_selection.paintAnchors(p);
    }
}
```

### `SelectionRect` 锚点体系

```cpp
enum class AnchorPos {
    None,
    TopLeft, Top, TopRight, Right,
    BottomRight, Bottom, BottomLeft, Left
};

// 命中测试（逻辑像素坐标）
AnchorPos SelectionRect::hitTest(QPoint pos) const {
    const int R = 6; // 命中半径
    struct { QPoint pt; AnchorPos ap; } anchors[] = {
        { m_rect.topLeft(),                           AnchorPos::TopLeft     },
        { QPoint(m_rect.center().x(), m_rect.top()),  AnchorPos::Top         },
        { m_rect.topRight(),                          AnchorPos::TopRight    },
        { QPoint(m_rect.right(), m_rect.center().y()),AnchorPos::Right       },
        { m_rect.bottomRight(),                       AnchorPos::BottomRight },
        { QPoint(m_rect.center().x(), m_rect.bottom()),AnchorPos::Bottom     },
        { m_rect.bottomLeft(),                        AnchorPos::BottomLeft  },
        { QPoint(m_rect.left(), m_rect.center().y()), AnchorPos::Left        },
    };
    for (auto &a : anchors)
        if (QRect(a.pt - QPoint(R,R), QSize(R*2,R*2)).contains(pos))
            return a.ap;
    return AnchorPos::None;
}

// DPI 坐标换算（Widget 逻辑坐标 → 截图物理像素坐标）
QRect SelectionRect::toPhysicalPixels() const {
    qreal dpr = m_widget->devicePixelRatioF();
    return QRect(
        qRound(m_rect.x()      * dpr), qRound(m_rect.y()      * dpr),
        qRound(m_rect.width()  * dpr), qRound(m_rect.height() * dpr)
    );
}
```

### 交付物验收

- [ ] 双屏/三屏截图拼接无黑边，虚拟桌面坐标正确
- [ ] 框选拖拽流畅，锚点命中率 ≥ 95%（测试用例覆盖 8 个方向）
- [ ] 125% / 150% / 200% HiDPI 截图坐标精确到像素

---

## 5. 阶段 2 · 标注工具与交互（2 天）

### 目标

四种标注工具可用 + 悬浮工具栏 + 撤销/重做，为最终输出做好准备。

### 任务清单

**Day 1**
- [ ] `AnnotationLayer` Widget（透明，覆盖选区范围）
- [ ] `ToolStateMachine` 状态机，鼠标事件驱动
- [ ] `ShapeItem` 抽象基类 + `RectShape` + `ArrowShape`
- [ ] `QUndoStack` 集成，`AddShapeCommand` / `RemoveShapeCommand`

**Day 2**
- [ ] `MosaicShape`（预渲染缓存优化）
- [ ] `TextShape`（内嵌 `QTextEdit`，`Esc` 事件过滤）
- [ ] `FloatToolBar`（自适应吸附逻辑）
- [ ] 工具栏颜色选择器、线宽滑块、确认/取消按钮

### `ToolStateMachine` 状态机

```
┌─────────────────────────────────────────────────────┐
│                  ScreenshotWidget                   │
│                                                     │
│  Idle ──(press outside sel)──► Selecting            │
│           │                        │                │
│    (press on sel, no tool)         │ (release)      │
│           ▼                        ▼                │
│        Moving                   Selected            │
│                                    │                │
│                          ┌─────────┴──────────┐     │
│                   (press  │                    │     │
│                  on anchor)            (tool active) │
│                          ▼                    ▼     │
│                      Adjusting           Drawing    │
│                          │                    │     │
│                      (release)            (release) │
│                          └─────────┬──────────┘     │
│                                    ▼                │
│                                 Selected            │
└─────────────────────────────────────────────────────┘
```

```cpp
enum class ToolMode {
    None, Move, Rect, Ellipse, Arrow, Pen, Mosaic, Text, Eraser
};

class ToolStateMachine {
public:
    void onMousePress(QPoint pos, Qt::MouseButton btn);
    void onMouseMove(QPoint pos);
    void onMouseRelease(QPoint pos, Qt::MouseButton btn);

    void setMode(ToolMode mode)       { m_mode = mode; }
    ToolMode mode() const             { return m_mode; }

private:
    ShapeItem* createShape(QPoint startPt); // 工厂，根据 m_mode 创建
    void commitShape();                     // push 到 QUndoStack

    ToolMode              m_mode    = ToolMode::None;
    ShapeItem            *m_current = nullptr;  // 正在绘制的图形
    QUndoStack           *m_undoStack;
    AnnotationLayer      *m_layer;
};
```

### `ShapeItem` 抽象基类

```cpp
class ShapeItem {
public:
    virtual ~ShapeItem() = default;

    // 绘制（传入相对于选区的 QPainter）
    virtual void paint(QPainter *p) const = 0;

    // 几何
    virtual QRect   boundingRect() const = 0;
    virtual void    setStartPoint(QPoint pt) = 0;
    virtual void    updateEndPoint(QPoint pt) = 0;
    virtual bool    contains(QPoint pt) const { return boundingRect().contains(pt); }

    // 选中状态（移动/删除时使用）
    bool selected = false;

    // 样式属性
    QColor  penColor  = QColor("#F5292A");
    QColor  fillColor = Qt::transparent;
    int     penWidth  = 2;
    qreal   opacity   = 1.0;
};
```

### 各派生类实现要点

**`ArrowShape`**

```cpp
void ArrowShape::paint(QPainter *p) const {
    p->save();
    p->setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap));

    // 箭头杆
    p->drawLine(m_start, m_end);

    // 箭头头部：基于向量旋转
    QLineF line(m_end, m_start);
    double angle = std::atan2(line.dy(), line.dx());
    double headLen = qMax(10.0, penWidth * 4.0);
    double headAngle = M_PI / 7;

    QPointF p1 = m_end + QPointF(
        std::cos(angle - headAngle) * headLen,
        std::sin(angle - headAngle) * headLen);
    QPointF p2 = m_end + QPointF(
        std::cos(angle + headAngle) * headLen,
        std::sin(angle + headAngle) * headLen);

    QPainterPath head;
    head.moveTo(m_end); head.lineTo(p1); head.lineTo(p2); head.closeSubpath();
    p->fillPath(head, penColor);
    p->restore();
}
```

**`MosaicShape`（性能关键）**

```cpp
// AnnotationLayer 初始化时预生成马赛克缓存
void AnnotationLayer::initMosaicCache(const QPixmap &base) {
    int factor = 12; // 马赛克粗糙度
    QSize sz = base.size();
    QPixmap tiny = base.scaled(sz / factor, Qt::IgnoreAspectRatio,
                                Qt::FastTransformation);
    m_mosaicCache = tiny.scaled(sz, Qt::IgnoreAspectRatio,
                                 Qt::FastTransformation);
}

// MosaicShape::paint 直接贴缓存，不实时计算
void MosaicShape::paint(QPainter *p) const {
    p->drawPixmap(m_rect, m_layer->mosaicCache(), m_rect);
}
```

**`TextShape`（焦点处理）**

```cpp
// AnnotationLayer 安装 eventFilter 拦截 TextEdit 的 Esc
bool AnnotationLayer::eventFilter(QObject *obj, QEvent *e) {
    if (obj == m_currentTextEdit && e->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Escape) {
            finishTextEditing();  // 仅提交文字，不关闭截图窗口
            return true;          // 拦截，不向上传播
        }
    }
    return QObject::eventFilter(obj, e);
}
```

### `FloatToolBar` 自适应吸附算法

```cpp
void FloatToolBar::repositionNear(const QRect &selRect, const QRect &screenRect) {
    const int GAP = 8;
    int toolH = height(), toolW = width();

    // 计算候选位置（优先下方，次选上方）
    QPoint below(selRect.left(), selRect.bottom() + GAP);
    QPoint above(selRect.left(), selRect.top() - toolH - GAP);

    QPoint pos = (below.y() + toolH <= screenRect.bottom()) ? below : above;

    // 水平方向：优先左对齐，溢出则右对齐
    if (pos.x() + toolW > screenRect.right())
        pos.setX(selRect.right() - toolW);
    pos.setX(qMax(pos.x(), screenRect.left()));

    move(pos);
}
```

### 工具栏组件规划

| 区域 | 组件 | 说明 |
|---|---|---|
| 工具区 | `QButtonGroup`（互斥） | 框选移动 / 矩形 / 椭圆 / 箭头 / 画笔 / 马赛克 / 文字 / 橡皮擦 |
| 样式区 | 内联 10 色快选 + 自定义 | 点击自定义弹 `QColorDialog` |
| 线宽区 | `QSlider` (1–10px) | 实时预览 |
| 分隔符 | — | — |
| 操作区 | Undo / Redo / Pin | 图标按钮 |
| 确认区 | 复制 / 保存 / 取消 | 右侧分组 |

### 交付物验收

- [ ] 四种标注工具（矩形、箭头、马赛克、文字）渲染正确
- [ ] `Ctrl+Z` / `Ctrl+Y` 撤销/重做，最少支持 50 步
- [ ] 工具栏在选区靠近屏幕边缘时正确翻转，无超出屏幕
- [ ] `TextShape` 编辑时 `Esc` 仅退出文字模式，不关闭截图

---

## 6. 阶段 3 · 系统功能集成（1.5 天）

### 目标

全局热键、系统托盘、最终输出合并、贴图悬浮窗端到端可用。

### 任务清单

- [ ] `HotkeyManager`：默认 `Alt+A`，支持运行时重绑定，失败时 fallback 提示
- [ ] `TrayIcon`：右键菜单（截图 / 设置 / 退出），双击打开主界面
- [ ] `OutputManager`：最终图合并（底图 + 标注叠加），复制剪贴板 + 保存文件
- [ ] `PinWindow`（P2）：置顶、拖拽、滚轮缩放、右键菜单
- [ ] 截图完成后向 QML 发送 `captureCompleted` 信号

### `HotkeyManager` 全局热键

```cpp
class HotkeyManager : public QObject {
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr) {
        m_hotkey = new QHotkey(this);
        setSequence(QKeySequence("Alt+A")); // 默认绑定
    }

    bool setSequence(const QKeySequence &seq) {
        m_hotkey->setRegistered(false);
        m_hotkey->setShortcut(seq, true);
        if (!m_hotkey->isRegistered()) {
            emit registrationFailed(seq.toString());
            return false;
        }
        connect(m_hotkey, &QHotkey::activated,
                this, &HotkeyManager::captureTriggered,
                Qt::UniqueConnection);
        return true;
    }

signals:
    void captureTriggered();
    void registrationFailed(const QString &sequence);

private:
    QHotkey *m_hotkey;
};
```

> **触发顺序（关键）**：热键触发后，必须**先调 `CaptureEngine::captureAll()`** 截图（此时遮罩窗口尚未弹出，截图不会包含自身），再 `show()` `ScreenshotWidget`。若顺序颠倒，将把遮罩窗口截入背景图。

### `OutputManager` 最终图合并

```cpp
QPixmap OutputManager::render(
    const QPixmap           &base,      // 完整截图（物理像素）
    const QRect             &selection, // 选区（逻辑像素）
    qreal                    dpr,       // 设备像素比
    const QList<ShapeItem*> &shapes)    // 所有标注图形
{
    // Step 1: 按物理像素坐标裁剪选区
    QRect physSel(
        qRound(selection.x() * dpr), qRound(selection.y() * dpr),
        qRound(selection.width() * dpr), qRound(selection.height() * dpr));
    QPixmap result = base.copy(physSel);
    result.setDevicePixelRatio(dpr);

    // Step 2: 在底图上叠加标注
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(-selection.topLeft()); // 坐标对齐：标注使用选区坐标系
    for (auto *shape : shapes)
        shape->paint(&painter);

    return result;
}

void OutputManager::toClipboard(const QPixmap &px) {
    QGuiApplication::clipboard()->setPixmap(px);
}

void OutputManager::saveToFile(const QPixmap &px, QWidget *parent) {
    QString defaultName = QString("screenshot_%1.png")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString defaultDir = QStandardPaths::writableLocation(
        QStandardPaths::PicturesLocation);
    QString path = QFileDialog::getSaveFileName(
        parent, "保存截图", defaultDir + "/" + defaultName,
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)");
    if (!path.isEmpty())
        px.save(path);
}
```

### `PinWindow` 贴图悬浮窗

```cpp
class PinWindow : public QLabel {
    Q_OBJECT
public:
    explicit PinWindow(const QPixmap &px, QWidget *parent = nullptr)
        : QLabel(parent), m_originalPixmap(px) {
        setWindowFlags(Qt::FramelessWindowHint
                     | Qt::WindowStaysOnTopHint
                     | Qt::Tool);
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_TranslucentBackground);
        setPixmap(px);
        adjustSize();
        show();
    }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton)
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (e->buttons() & Qt::LeftButton)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    void wheelEvent(QWheelEvent *e) override {
        // 滚轮缩放（10% 步进，最小 50px）
        double factor = (e->angleDelta().y() > 0) ? 1.1 : 0.9;
        m_scale = qBound(0.1, m_scale * factor, 5.0);
        QSize newSize = m_originalPixmap.size() * m_scale
                      / m_originalPixmap.devicePixelRatioF();
        setPixmap(m_originalPixmap.scaled(
            newSize * m_originalPixmap.devicePixelRatioF(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        adjustSize();
    }
    void contextMenuEvent(QContextMenuEvent *e) override {
        QMenu menu(this);
        menu.addAction("复制",   [this]{ OutputManager::toClipboard(m_originalPixmap); });
        menu.addAction("保存",   [this]{ OutputManager::saveToFile(m_originalPixmap, this); });
        menu.addSeparator();
        menu.addAction("关闭",   this, &QWidget::close);
        menu.exec(e->globalPos());
    }

private:
    QPixmap m_originalPixmap;
    QPoint  m_dragPos;
    double  m_scale = 1.0;
};
```

### 交付物验收

- [ ] `Alt+A` 热键触发截图，截图不含自身遮罩
- [ ] 热键冲突时弹出提示，引导重新绑定
- [ ] 复制到剪贴板后在 Photoshop / Word 中粘贴格式正确
- [ ] 保存文件名含时间戳，格式可选 PNG / JPEG
- [ ] `PinWindow` 可拖拽、滚轮缩放，右键关闭

---

## 7. 阶段 4 · 多平台适配与发布（1 天）

### 目标

三平台兼容、打包交付、自动化构建全通。

### 任务清单

- [ ] `WaylandBackend`：`xdg-desktop-portal` D-Bus 异步截图完整实现
- [ ] macOS：`AXIsProcessTrusted()` 权限检测 + 引导 Dialog
- [ ] macOS：`NSScreenCaptureUsageDescription` 写入 `Info.plist`
- [ ] Windows：`PrintWindow` API 作为 UAC 窗口截图兜底
- [ ] Windows HiDPI：`SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)` 确认设置
- [ ] 三平台打包命令封装为 CMake targets
- [ ] GitHub Actions workflow 完整配置

### Wayland 后端实现骨架

```cpp
// 通过 DBus 调用 xdg-desktop-portal Screenshot 接口
class WaylandBackend : public QObject, public ICaptureBackend {
    Q_OBJECT
public:
    QPixmap capture() override {
        // Wayland 必须异步，同步调用会死锁
        // 使用 QEventLoop 在调用方伪同步等待
        QEventLoop loop;
        QPixmap result;
        startAsync([&](QPixmap px){ result = px; loop.quit(); });
        loop.exec();
        return result;
    }

private:
    void startAsync(std::function<void(QPixmap)> callback) {
        auto *iface = new QDBusInterface(
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Screenshot",
            QDBusConnection::sessionBus(), this);

        QVariantMap opts;
        opts["interactive"] = false;
        opts["handle_token"] = QString("iqtools_%1").arg(QRandomGenerator::global()->generate());

        auto reply = iface->asyncCall("Screenshot", "", opts);
        auto *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                [callback](QDBusPendingCallWatcher *w) {
                    QDBusPendingReply<QDBusObjectPath> r = *w;
                    // 实际图像通过 Response 信号回传，此处简化
                    // 完整实现需监听 /org/freedesktop/portal/desktop 的 Response 信号
                    w->deleteLater();
                });
    }
};
```

### 测试矩阵

| 测试项 | Windows 11 (125%) | Windows 11 (200%) | Ubuntu X11 | Ubuntu Wayland | macOS 13 (Retina) |
|---|:---:|:---:|:---:|:---:|:---:|
| 多屏截图拼接无黑边 | ✓ | ✓ | ✓ | ⚠ Portal | ✓ |
| 选区像素坐标精确 | ✓ | ✓ | ✓ | ✓ | ✓ |
| 全局热键注册 | ✓ | ✓ | ✓ | ✓ | 需辅助功能权限 |
| 剪贴板粘贴到第三方应用 | ✓ | ✓ | ✓ | XDG 剪贴板 | ✓ |
| 系统托盘显示 | ✓ | ✓ | ✓ | ✓ | ✓ |
| 马赛克标注性能（4K 屏） | ✓ | ✓ | ✓ | ✓ | ✓ |
| 贴图窗口置顶 | ✓ | ✓ | ✓ | ✓ | ✓ |

### 打包命令

```bash
# Windows（PowerShell）
windeployqt --release --no-translations build/IQtools.exe
cpack -G NSIS -C Release

# macOS
macdeployqt build/IQtools.app -dmg -always-overwrite

# Linux（AppImage）
export VERSION=$(git describe --tags)
linuxdeployqt build/IQtools -appimage -extra-plugins=iconengines
```

### GitHub Actions（关键配置）

```yaml
# .github/workflows/build.yml
name: Build & Test

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        include:
          - os: windows-latest
            preset: windows-release
          - os: ubuntu-22.04
            preset: linux-release
          - os: macos-13
            preset: macos-release

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - uses: jurplel/install-qt-action@v3
        with:
          version: '6.5.3'
          modules: 'qtimageformats qtmultimedia'
          cache: true

      - name: Configure
        run: cmake -B build --preset ${{ matrix.preset }}

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Test
        run: ctest --test-dir build --output-on-failure

      - name: Package
        run: cmake --build build --target package
```

---

## 8. QML ↔ C++ 完整交互协议

### 完整信号/方法表

| 方向 | 名称 | 类型 | 说明 |
|---|---|---|---|
| QML → C++ | `AppManager.startCapture()` | `Q_INVOKABLE` | 立即触发截图 |
| QML → C++ | `AppManager.startCaptureDelayed(int sec)` | `Q_INVOKABLE` | 延时截图 |
| QML → C++ | `AppManager.setHotkey(string seq)` | `Q_INVOKABLE` | 更新热键绑定 |
| QML → C++ | `AppManager.openSettings()` | `Q_INVOKABLE` | 打开设置 |
| C++ → QML | `captureCompleted(string path)` | `signal` | 截图成功，附带保存路径 |
| C++ → QML | `captureCancelled()` | `signal` | 用户按 Esc 取消 |
| C++ → QML | `hotkeyConflict(string seq)` | `signal` | 热键冲突，需用户重新绑定 |
| C++ → QML | `captureDelayTick(int remaining)` | `signal` | 延时倒计时（每秒触发） |

### 端到端调用时序图

```
用户点击 QML 按钮
    │
    ▼
AppManager::startCapture()
    │
    ▼
CapturePlugin::startCaptureSession()
    │
    ├─① CaptureEngine::captureAll()      ← 此时遮罩窗口未显示，截图干净
    │
    ├─② ScreenshotWidget::show()         ← 全屏遮罩弹出
    │       │
    │       ├─ 用户框选选区
    │       ├─ 用户在 FloatToolBar 进行标注
    │       └─ 用户点击「复制」或「保存」
    │
    ├─③ OutputManager::render()          ← 合并底图 + 标注
    │
    ├─④ toClipboard() / saveToFile()     ← 输出
    │
    └─⑤ emit AppManager::captureCompleted(path)
                │
                ▼
        QML Toast 通知用户
```

---

## 9. 完整类设计与接口规范

### 类关系总览

```
AppManager  (暴露给 QML)
    └── CapturePlugin  (implements ICapturePlugin)
            ├── CaptureEngine
            │       └── ICaptureBackend
            │               ├── WinBackend
            │               ├── X11Backend
            │               ├── WaylandBackend
            │               └── MacBackend
            ├── ScreenshotWidget  (QWidget, 全屏覆盖)
            │       ├── SelectionRect
            │       │       └── AnchorHandle × 8
            │       └── AnnotationLayer  (QWidget, 覆盖选区)
            │               ├── ToolStateMachine
            │               ├── FloatToolBar  (QWidget)
            │               ├── QUndoStack
            │               └── ShapeItem []
            │                       ├── RectShape
            │                       ├── EllipseShape
            │                       ├── ArrowShape
            │                       ├── MosaicShape
            │                       ├── PenShape
            │                       └── TextShape
            ├── HotkeyManager
            ├── TrayIcon
            ├── OutputManager
            └── PinWindow  (QLabel, 独立置顶窗口)
```

### 关键接口汇总

```cpp
// ICapturePlugin
class ICapturePlugin {
public:
    virtual void startCaptureSession() = 0;
    virtual void startCaptureDelayed(int seconds) = 0;
    virtual void setHotkey(const QString &sequence) = 0;
};

// ICaptureBackend
class ICaptureBackend {
public:
    virtual QPixmap capture() = 0;
    virtual bool isAsync() const { return false; }
    static ICaptureBackend *create(QObject *parent = nullptr);
};

// ShapeItem（标注图形基类）
class ShapeItem {
public:
    virtual void paint(QPainter *p) const = 0;
    virtual QRect boundingRect() const = 0;
    virtual void setStartPoint(QPoint pt) = 0;
    virtual void updateEndPoint(QPoint pt) = 0;
    virtual bool contains(QPoint pt) const;
    QColor penColor = QColor("#F5292A");
    int    penWidth = 2;
    bool   selected = false;
};

// OutputManager（静态工具类）
class OutputManager {
public:
    static QPixmap render(const QPixmap &base, const QRect &sel,
                           qreal dpr, const QList<ShapeItem*> &shapes);
    static void toClipboard(const QPixmap &px);
    static void saveToFile(const QPixmap &px, QWidget *parent = nullptr);
};
```

---

## 10. 数据流与状态机设计

### 截图会话完整状态机

```
App 空闲
    │
    ├──(热键 / QML 按钮)──────────────────────► 截图中
    │                                               │
    │                                         CaptureEngine 抓图
    │                                               │
    │                                    ScreenshotWidget 显示
    │                                               │
    │                           ┌──────────────────┤
    │                     (Esc) │            (拖拽鼠标)
    │                           ▼                  ▼
    │                        取消 ◄─────── 框选中 ──────► 框选完成
    │                           │                            │
    │                    (退出截图)                    (FloatToolBar 弹出)
    │                                                        │
    │                                           ┌────────────┤
    │                                  (标注)   │     (直接输出)
    │                                           ▼            ▼
    │                                       标注中 ──► 确认输出
    │                                                        │
    │                                        ┌───────────────┤
    │                               (复制)   │       (保存)  │  (Pin)
    │                                        ▼               ▼   ▼
    └────────────────────────────── App 空闲，Toast 通知 QML
```

### 马赛克数据流（性能关键路径）

```
CaptureEngine::captureAll()
    │
    └──► AnnotationLayer::initMosaicCache(fullshot)
              │
              │  scaled(1/12) → scaled(12x) = m_mosaicCache  [一次性，O(WH)]
              │
              ▼
        用户绘制马赛克时 (每帧 mouseMoveEvent)
              │
              └──► MosaicShape::paint(p)
                        │
                        └──► p->drawPixmap(rect, m_mosaicCache, rect)
                                    [O(1)，直接贴预渲染缓存]
```

---

## 11. 风险与对策

| # | 风险 | 影响 | 对策 | 责任人 |
|---|---|---|---|---|
| R1 | Linux Wayland 无法直接截图 | 高 | `ICaptureBackend` 接口隔离，`WaylandBackend` 使用 `xdg-desktop-portal` DBus 异步接口；X11 fallback 完整实现，Wayland 后端阶段 4 完善 | 阶段 1 设计接口，阶段 4 实现 |
| R2 | 马赛克大面积实时计算卡顿 | 中 | 预渲染整屏马赛克缓存（`initMosaicCache`），绘制时 O(1) 贴图，不实时计算 | 阶段 2 |
| R3 | macOS 屏幕录制/辅助功能权限缺失 | 高 | 启动时检测 `AXIsProcessTrusted()`，未授权弹引导 Dialog（附跳转「系统设置」按钮） | 阶段 4 |
| R4 | `TextShape` 内嵌 `QTextEdit`，`Esc` 关闭截图窗口 | 中 | `AnnotationLayer::eventFilter` 拦截 `QTextEdit` 的 `Esc`，`accept()` 后仅提交文字，不传播 | 阶段 2 |
| R5 | 多屏负坐标/坐标系差异 | 中 | `QRect::united()` 计算虚拟桌面包围盒，所有坐标相对于 `total.topLeft()` 偏移统一处理 | 阶段 1 |
| R6 | 热键冲突（被其他应用占用） | 低 | `QHotkey::isRegistered()` 检测，失败时 emit `hotkeyConflict` 通知 QML 展示重绑定 UI | 阶段 3 |
| R7 | 截图包含自身遮罩窗口 | 高 | 严格保证触发顺序：**先截图，后 show()** | 阶段 1/3 |
| R8 | Windows UAC 窗口截不到 | 低 | `WinBackend` 集成 `PrintWindow` API 作为兜底 | 阶段 4 |
| R9 | QML Widget 同进程 OpenGL 上下文冲突 | 低 | `ScreenshotWidget` 使用 `Qt::Tool` flag 作为独立顶层窗口，不嵌入 QML 层级；若出现冲突，在 `main.cpp` 设置 `QQuickWindow::setGraphicsApi(QSGRendererInterface::Software)` 作为应急 | 阶段 0 |

---

## 12. 进度检查点与验收标准

| 里程碑 | 完成标准 | 目标日期 |
|---|---|---|
| **M0** 桥接就绪 | QML 按钮可触发 C++ 截图（日志可见）；三平台 CMake build 通过；CI 绿灯 | Day 0.5 |
| **M1** 截图框选可用 | 多屏拼接无黑边；DPI 125%/200% 坐标精确；八锚点拖拽流畅 | Day 2 |
| **M2** 标注完整可用 | 四种标注工具渲染正确；Undo/Redo ≥50 步；工具栏自适应无超出屏幕 | Day 4 |
| **M3** 系统集成完毕 | 热键触发截图干净；复制/保存端到端正常；Pin 窗口可拖拽缩放 | Day 5.5 |
| **M4** 发布就绪 | 测试矩阵全绿（见第 7 章表格）；三平台安装包打包完成；CI 自动构建通过 | Day 7 |

---

## 附录：关键 Qt API 速查

| 功能 | API |
|---|---|
| 枚举所有屏幕 | `QGuiApplication::screens()` |
| 截取指定屏幕 | `QScreen::grabWindow(0)` |
| 设备像素比 | `QScreen::devicePixelRatio()` / `QWidget::devicePixelRatioF()` |
| 全局热键 | `QHotkey::setShortcut(QKeySequence, true)` |
| 系统托盘 | `QSystemTrayIcon` + `QMenu` |
| 剪贴板输出 | `QGuiApplication::clipboard()->setPixmap(px)` |
| 撤销/重做 | `QUndoStack` + `QUndoCommand` |
| Porter-Duff 合成 | `QPainter::setCompositionMode(CompositionMode_Source)` |
| 无边框置顶窗口 | `Qt::FramelessWindowHint \| Qt::WindowStaysOnTopHint \| Qt::Tool` |
| 透明背景 | `Qt::WA_TranslucentBackground` |
| DBus 调用（Wayland） | `QDBusInterface` + `QDBusPendingCallWatcher` |
| macOS 辅助功能检测 | `AXIsProcessTrusted()` (AppKit) |

---

*文档版本：v2.0 · 项目：IQtools · 最后更新：2026-04-10*