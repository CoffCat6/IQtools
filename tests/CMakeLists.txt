cmake_minimum_required(VERSION 3.21)

project(IQtoolsTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(test_core
    test_core.cpp
)

target_link_libraries(test_core PRIVATE
    Qt6::Core
)
