#include "plugin_executor.h"
#include "plugin_loader.h"
#include "../global_vars.h"
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include "plugins_interface.h"
#include "../header.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>      // Unix 动态库加载
#endif

void PluginExecutor::execute_command(const std::string& command, const std::vector<std::string>& cmd_args) {
    // 查找命令对应的插件
    auto it = PluginLoader::command_to_plugin_map().find(command);
    if (it == PluginLoader::command_to_plugin_map().end()) {
        println("Unknown command: " << command.c_str());
        return;
    }

    std::string plugin_name = it->second;
    execute_plugin_with_command(plugin_name, cmd_args);
}

// 在 plugin_executor.cpp 中添加实现
void PluginExecutor::execute_plugin_with_command(const std::string& plugin_name, const std::vector<std::string>& cmd_args) {
    // 解析插件名称
    std::string resolved_name = PluginLoader::resolve_plugin_name(plugin_name);

    auto it = PluginLoader::installed_plugins().find(resolved_name);
    if (it == PluginLoader::installed_plugins().end()) {
        println("Plugin '" << resolved_name.c_str() << "' is not installed.\n");
        return;
    }

    if (!it->second) {
        println("Plugin '" << resolved_name.c_str() << "' is disabled.\n");
        return;
    }

    std::string plugin_path = home_dir + "/duckshell/plugins/" + resolved_name;

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(plugin_path.c_str());
    if (!handle) {
        println("Failed to load plugin '" << resolved_name.c_str() << "'. Error: " << GetLastError());
        return;
    }

    CreatePluginFunc create_func = (CreatePluginFunc)GetProcAddress(handle, "createPlugin");
    if (!create_func) {
        println("Plugin '" << resolved_name.c_str() << "' does not export createPlugin function.\n");
        FreeLibrary(handle);
        return;
    }

    IPlugin* plugin = create_func();
    if (plugin) {
        // 执行新接口的 on_execute
        plugin->on_execute(cmd_args);

        DestroyPluginFunc destroy_func = (DestroyPluginFunc)GetProcAddress(handle, "destroyPlugin");
        if (destroy_func) {
            destroy_func(plugin);
        }
        FreeLibrary(handle);
    }
#else
    // Unix/Linux 系统使用 dlopen/dlsym
    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
    if (!handle) {
        println("Failed to load plugin '" << resolved_name.c_str() << "'. Error: " << dlerror());
        return;
    }

    // 清除之前的错误
    dlerror();
    CreatePluginFunc create_func = (CreatePluginFunc)dlsym(handle, "createPlugin");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        println("Plugin '" << resolved_name.c_str() << "' does not export createPlugin function. Error: " << dlsym_error);
        dlclose(handle);
        return;
    }

    IPlugin* plugin = create_func();
    if (plugin) {
        // 执行新接口的 on_execute
        plugin->on_execute(cmd_args);

        // 清除之前的错误
        dlerror();
        DestroyPluginFunc destroy_func = (DestroyPluginFunc)dlsym(handle, "destroyPlugin");
        const char* dlsym_error = dlerror();
        if (!dlsym_error && destroy_func) {
            destroy_func(plugin);
        }
        dlclose(handle);
    }
#endif
}