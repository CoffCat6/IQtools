cmake_minimum_required(VERSION 3.21)

project(IQtools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_PREFIX_PATH "D:/Qt/6.9.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2)

set(IQTOOLS_SOURCES
    src/app/main.cpp
    src/app/bootstrap/application_bootstrap.cpp
    src/app/shell/main_window.cpp
    src/app/shell/bridge/tool_list_model.cpp
    src/app/shell/bridge/theme_controller.cpp
    src/app/shell/bridge/navigation_controller.cpp
    src/app/shell/bridge/logging_settings_controller.cpp
    src/app/shell/bridge/log_console_controller.cpp
    src/app/shell/bridge/app_facade.cpp
    src/core/appcontext/app_context.cpp
    src/core/tools/tool_registry.cpp
    src/core/services/log_service.cpp
    src/core/services/theme_manager.cpp
    src/core/plugin/plugin_manager.cpp
    src/infra/logging/log_manager.cpp
    src/infra/logging/logger.cpp
)

set(IQTOOLS_HEADERS
    src/app/bootstrap/application_bootstrap.h
    src/app/shell/main_window.h
    src/app/shell/bridge/tool_list_model.h
    src/app/shell/bridge/theme_controller.h
    src/app/shell/bridge/navigation_controller.h
    src/app/shell/bridge/logging_settings_controller.h
    src/app/shell/bridge/log_console_controller.h
    src/app/shell/bridge/app_facade.h
    src/core/appcontext/app_context.h
    src/core/tools/tool_registry.h
    src/core/tools/tool_metadata.h
    src/core/services/log_service.h
    src/core/services/theme_manager.h
    src/core/plugin/plugin_manager.h
    src/plugin-api/iplugin.h
    src/plugin-api/itool_plugin.h
    src/plugin-api/iplugin_context.h
    src/plugin-api/plugin_constants.h
    src/infra/logging/logger.h
    src/infra/logging/log_manager.h
    src/infra/logging/log_level.h
    src/infra/logging/log_entry.h
)

qt_add_executable(IQtools WIN32
    ${IQTOOLS_SOURCES}
)

qt_add_qml_module(IQtools
    URI IQtools
    VERSION 1.0
    QML_FILES
        src/app/shell/qml/main.qml
        src/app/shell/qml/AppWindow.qml
        src/app/shell/qml/components/AppSidebar.qml
        src/app/shell/qml/components/NavItem.qml
        src/app/shell/qml/components/BentoCard.qml
        src/app/shell/qml/components/ThemedButton.qml
        src/app/shell/qml/pages/HomePage.qml
        src/app/shell/qml/pages/CapturePage.qml
        src/app/shell/qml/pages/TranslatePage.qml
        src/app/shell/qml/pages/PluginPage.qml
        src/app/shell/qml/pages/LogPage.qml
        src/app/shell/qml/pages/SettingsPage.qml
)

target_include_directories(IQtools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(IQtools PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)
