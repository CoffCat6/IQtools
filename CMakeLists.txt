cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(IQTOOLS_SOURCES
    src/app/main.cpp
    src/app/bootstrap/application_bootstrap.cpp
    src/app/shell/main_window.cpp
    src/core/appcontext/app_context.cpp
    src/core/tools/tool_registry.cpp
    src/core/services/log_service.cpp
    src/core/plugin/plugin_manager.cpp
    src/infra/logging/logger.cpp
)

set(IQTOOLS_HEADERS
    src/app/bootstrap/application_bootstrap.h
    src/app/shell/main_window.h
    src/core/appcontext/app_context.h
    src/core/tools/tool_registry.h
    src/core/tools/tool_metadata.h
    src/core/services/log_service.h
    src/core/plugin/plugin_manager.h
    src/plugin-api/iplugin.h
    src/plugin-api/itool_plugin.h
    src/plugin-api/iplugin_context.h
    src/plugin-api/plugin_constants.h
    src/infra/logging/logger.h
    src/infra/logging/log_level.h
    src/infra/logging/log_entry.h
)

qt_add_executable(IQtools WIN32
    ${IQTOOLS_SOURCES}
)

target_include_directories(IQtools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(IQtools PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

enable_testing()
add_subdirectory(tests)
