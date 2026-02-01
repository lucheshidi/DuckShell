#ifndef PLUGIN_SYSTEM_H
#define PLUGIN_SYSTEM_H

#include <string>
#include <vector>
#include <map>

// 包含插件系统的模块
#include "plugins/plugin_loader.h"
#include "plugins/plugin_installer.h"
#include "plugins/plugin_repository.h"
#include "plugins/plugin_executor.h"

// 新的命名空间风格的接口，使用下划线命名法
namespace plugin_system {
    // 插件加载功能
    inline void load_plugins() { PluginLoader::load_plugins(); }
    inline void build_command_map() { PluginLoader::build_command_map(); }
    inline bool is_plugin_installed(const std::string& plugin_name) { return PluginLoader::is_plugin_installed(plugin_name); }
    inline bool is_plugin_command(const std::string& command) { return PluginLoader::is_plugin_command(command); }
    inline std::string resolve_plugin_name(const std::string& plugin_name) { return PluginLoader::resolve_plugin_name(plugin_name); }

    // 插件安装功能
    inline void install_plugin(const std::string& plugin_name) { PluginInstaller::install_plugin(plugin_name); }
    inline void uninstall_plugin(const std::string& plugin_name) { PluginInstaller::uninstall_plugin(plugin_name); }
    inline void remove_plugin(const std::string& plugin_name) { PluginInstaller::remove_plugin(plugin_name); }
    inline void enable_plugin(const std::string& plugin_name) { PluginInstaller::enable_plugin(plugin_name); }
    inline void disable_plugin(const std::string& plugin_name) { PluginInstaller::disable_plugin(plugin_name); }
    inline void list_available_plugins() { PluginInstaller::list_available_plugins(); }
    inline void list_installed_plugins() { PluginInstaller::list_installed_plugins(); }
    inline void install_all_plugins() { PluginInstaller::install_all_plugins(); }

    // 插件仓库功能
    inline void set_repository_url(const std::string& url) { PluginRepository::set_repository_url(url); }
    inline void list_remote_plugins() { PluginRepository::list_remote_plugins(); }
    inline void download_plugin(const std::string& plugin_name) { PluginRepository::download_plugin(plugin_name); }

    // 插件执行功能
    inline void execute_command(const std::string& command, const std::vector<std::string>& cmd_args) { 
        PluginExecutor::execute_command(command, cmd_args); 
    }
    inline void execute_plugin_with_command(const std::string& plugin_name, const std::vector<std::string>& cmd_args) { 
        PluginExecutor::execute_plugin_with_command(plugin_name, cmd_args); 
    }

    // 静态变量访问
    inline std::string& repository_url() { return PluginLoader::repository_url; }
    inline std::map<std::string, bool>& installed_plugins() { return PluginLoader::installed_plugins; }
    inline std::map<std::string, std::string>& command_to_plugin_map() { return PluginLoader::command_to_plugin_map; }
}

#endif // PLUGIN_SYSTEM_H