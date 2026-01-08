# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(DuckShell VERSION 0.1.0)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# ================= Options =================
option(ENABLE_REMOTE_REPO "Enable remote plugin repository features" ON)

# ================= Dependencies =================

# 1. ZLIB & Minizip (Always needed for plugin extraction)
if(ENABLE_REMOTE_REPO)
    FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        v1.3.1
    )
    # zlib's CMake is a bit old-school, sometimes needs help
    FetchContent_MakeAvailable(zlib)

    # Minizip (Download source and compile manually to avoid dependency hell)
    FetchContent_Declare(
            minizip_src
            GIT_REPOSITORY https://github.com/sebastiandev/minizip.git
            GIT_TAG        master
    )
    FetchContent_GetProperties(minizip_src)
    if(NOT minizip_src_POPULATED)
        FetchContent_MakeAvailable(minizip_src)
        
        set(MINIZIP_SOURCES
            ${minizip_src_SOURCE_DIR}/unzip.c
            ${minizip_src_SOURCE_DIR}/ioapi.c
        )
        if(WIN32)
            list(APPEND MINIZIP_SOURCES ${minizip_src_SOURCE_DIR}/iowin32.c)
        endif()
        
        add_library(minizip STATIC ${MINIZIP_SOURCES})
        target_include_directories(minizip PUBLIC ${minizip_src_SOURCE_DIR})
        target_link_libraries(minizip PRIVATE zlibstatic)
    endif()
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
        src/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
)

target_include_directories(DuckShell PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

# ================= Linking =================

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

if(WIN32)
    target_link_libraries(DuckShell PRIVATE wininet ws2_32)
    target_compile_definitions(DuckShell PRIVATE _WIN32)
endif()

if(ENABLE_REMOTE_REPO)
    # Link ZLIB and Minizip
    target_link_libraries(DuckShell PRIVATE minizip zlibstatic)
    target_compile_definitions(DuckShell PRIVATE HAVE_MINIZIP)

    if(UNIX AND NOT APPLE)
        find_package(CURL REQUIRED)
        target_link_libraries(DuckShell PRIVATE CURL::libcurl)
        target_compile_definitions(DuckShell PRIVATE HAVE_LIBCURL)
    elseif(APPLE)
        find_package(CURL REQUIRED)
        target_link_libraries(DuckShell PRIVATE CURL::libcurl)
        target_compile_definitions(DuckShell PRIVATE HAVE_LIBCURL)
    endif()
    # Windows uses WinInet, already linked above.
endif()
