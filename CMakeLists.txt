# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(DuckShell)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_EXE_LINKER_FLAGS -static)
# 或者改为条件设置，排除Apple平台
if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS -static)
endif()

add_executable(DuckShell
        src/main.cpp
        src/header.h
        src/shell/main_shell.cpp
        src/plugins/plugins_manager.cpp
        src/plugins/plugins_manager.h
        src/plugins/plugins_interface.h
        src/global_vars.cpp
        src/global_vars.h
)

target_link_libraries(DuckShell wininet)


# 在CMakeLists.txt中添加
if(MINGW)
    # 禁用可能引起问题的特性
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7及以上
endif()


# 添加预处理器定义
if(WIN32)
    add_definitions(-D_WIN32)
endif()
