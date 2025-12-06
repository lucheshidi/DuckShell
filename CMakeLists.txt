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

# Windows平台需要链接WinINet库
if(WIN32)
    # 确保正确的库链接顺序
    target_link_libraries(DuckShell wininet ws2_32)
endif()

if(MINGW)
   # 可能需要额外的链接库
   target_link_libraries(DuckShell stdc++fs)
endif()
