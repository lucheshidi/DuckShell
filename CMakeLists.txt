cmake_minimum_required(VERSION 3.15)
project(DuckShell VERSION 0.1.0)

# 生成版本头文件
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
)

set(CMAKE_CXX_STANDARD 17)
include(FetchContent)

# ================= Options =================
option(ENABLE_REMOTE_REPO "Enable remote plugin repository features" ON)

# ================= Dependencies (libcurl Only) =================

if(ENABLE_REMOTE_REPO)
    # 1. 强制统一配置 curl 编译选项 (无论是 Win 还是 WSL)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)

    if(WIN32)
        set(CMAKE_USE_SCHANNEL ON CACHE BOOL "" FORCE)
        set(CMAKE_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    else()
        # Linux 下确保开启 OpenSSL 支持
        set(CMAKE_USE_OPENSSL ON CACHE BOOL "" FORCE)
    endif()

    # 2. 直接使用 FetchContent，删掉下面的 find_package(CURL)
    FetchContent_Declare(
            curl
            URL      https://github.com/curl/curl/releases/download/curl-8_5_0/curl-8.5.0.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            # 为curl设置特定的构建选项
            OVERRIDE_FIND_PACKAGE
    )
    
    # 使用现代CMake方法处理FetchContent
    set(CURL_STATICLIB ON CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(CMAKE_USE_OPENSSL ON CACHE BOOL "" FORCE)
    
    # 使用FetchContent_MakeAvailable来处理依赖
    FetchContent_MakeAvailable(curl)

    add_definitions(-DHAVE_LIBCURL)
endif()

# ================= Target =================

add_executable(DuckShell
        src/main.cpp
        src/header.h
        src/shell/shell_main.cpp
        src/shell/shell_commands.cpp
        src/shell/shell_commands.h
        src/shell/shell_input.cpp
        src/shell/shell_input.h
        src/plugins/plugin_manager.cpp
        src/plugins/plugin_manager.h
        src/plugins/plugin_loader.cpp
        src/plugins/plugin_loader.h
        src/plugins/plugin_installer.cpp
        src/plugins/plugin_installer.h
        src/plugins/plugin_repository.cpp
        src/plugins/plugin_repository.h
        src/plugins/plugin_executor.cpp
        src/plugins/plugin_executor.h
        src/plugins/plugin_common.cpp
        src/plugins/plugin_common.h
        src/plugins/plugins_interface.h
        src/global_vars.cpp
        src/global_vars.h
        src/json.hpp
        src/plugin_system.h
        src/shell_system.h
        ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
)

target_include_directories(DuckShell PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

# ================= Linking =================

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    add_definitions(-D_WIN32_WINNT=0x0601)
elseif(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(WIN32)
    # libcurl 在 Windows 上需要的系统库
    target_link_libraries(DuckShell PRIVATE libcurl ws2_32 crypt32 cryptui wldap32 normaliz)
    target_compile_definitions(DuckShell PRIVATE _WIN32)
endif()

if(ENABLE_REMOTE_REPO)
    # 关键：链接 FetchContent 出来的 'libcurl' 目标
    target_link_libraries(DuckShell PRIVATE libcurl)

    if(UNIX)
        # Linux 还需要这几个基础库
        target_link_libraries(DuckShell PRIVATE dl pthread)
    endif()
endif()