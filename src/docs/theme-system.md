# IQtools 主题系统说明（QSS）

## 当前目标

已提供两套默认主题：

- `src/resources/qss/dark.qss`
- `src/resources/qss/light.qss`

并提供 `ThemeManager` 统一调用入口，后续可扩展自定义主题。

## 接口

位置：`src/core/services/theme_manager.h`

### 主题枚举

```cpp
enum class AppTheme {
    Light = 0,
    Dark,
    Custom
};
```

### 主要方法

- `ThemeManager::applyTheme(QApplication*, AppTheme)`
- `ThemeManager::applyCustomTheme(QApplication*, const QString& qssFilePath)`
- `ThemeManager::themeName(AppTheme)`
- `ThemeManager::themeFilePath(AppTheme)`

## QSS 命名约定（便于后续扩展）

### 主体结构

- `QMainWindow#MainWindow`
- `QWidget#CentralRoot`
- `QWidget#NavPanel`
- `QWidget#ContentPanel`
- `QWidget#BentoGrid`

### 导航

- `QLabel#NavTitle`
- `QPushButton#NavButton`

### 卡片（Bento）

- `QFrame#BentoCard`
- `QFrame#BentoCard[variant="highlight"]`
- `QFrame#BentoCard[variant="accent"]`
- `QLabel#CardTitle`
- `QLabel#CardText`
- `QLabel#CardMeta`

### 滚动区域

- `QScrollArea#MainScroll`

## 后续扩展建议

1. 将主题路径改为配置项读取（SettingsService）
2. 在设置页增加“主题切换”
3. 支持外部主题目录（如 `themes/*.qss`）
4. 扩展主题变量体系（颜色 token / 间距 token）
