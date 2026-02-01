// plugins/plugin_manager.h
#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include "plugin_common.h"

// 包含拆分后的模块
#include "plugin_loader.h"
#include "plugin_installer.h"
#include "plugin_repository.h"
#include "plugin_executor.h"

// 保持原有的PluginManager接口以维持向后兼容性
class PluginManager {
public:
    // 插件加载功能
    static void loadPlugins() { PluginLoader::load_plugins(); }
    static void buildCommandMap() { PluginLoader::build_command_map(); }
    static bool isPluginInstalled(const std::string& pluginName) { return PluginLoader::is_plugin_installed(pluginName); }
    static bool isPluginCommand(const std::string& command) { return PluginLoader::is_plugin_command(command); }
    static std::string resolvePluginName(const std::string& pluginName) { return PluginLoader::resolve_plugin_name(pluginName); }

    // 插件安装功能
    static void installPlugin(const std::string& pluginName) { PluginInstaller::install_plugin(pluginName); }
    static void uninstallPlugin(const std::string& pluginName) { PluginInstaller::uninstall_plugin(pluginName); }
    static void removePlugin(const std::string& pluginName) { PluginInstaller::remove_plugin(pluginName); }
    static void enablePlugin(const std::string& pluginName) { PluginInstaller::enable_plugin(pluginName); }
    static void disablePlugin(const std::string& pluginName) { PluginInstaller::disable_plugin(pluginName); }
    static void listAvailablePlugins() { PluginInstaller::list_available_plugins(); }
    static void listInstalledPlugins() { PluginInstaller::list_installed_plugins(); }
    static void installAllPlugins() { PluginInstaller::install_all_plugins(); }

    // 插件仓库功能
    static void setRepositoryUrl(const std::string& url) { PluginRepository::set_repository_url(url); }
    static void listRemotePlugins() { PluginRepository::list_remote_plugins(); }
    static void downloadPlugin(const std::string& pluginName) { PluginRepository::download_plugin(pluginName); }

    // 插件执行功能
    static void executeCommand(const std::string& command, const std::vector<std::string>& cmdArgs) { 
        PluginExecutor::execute_command(command, cmdArgs); 
    }
    static void executePluginWithCommand(const std::string& pluginName, const std::vector<std::string>& cmdArgs) { 
        PluginExecutor::execute_plugin_with_command(pluginName, cmdArgs); 
    }

    // 静态变量访问
    static std::string& repository_url() { return plugin_repository_url; }
    static std::map<std::string, bool>& installed_plugins() { return plugin_installed_plugins; }
    static std::map<std::string, std::string>& command_to_plugin_map() { return plugin_command_to_plugin_map; }
};

#endif // PLUGIN_MANAGER_H