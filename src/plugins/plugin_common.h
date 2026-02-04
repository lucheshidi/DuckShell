#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <string>
#include <map>
#include <vector>

class IPlugin;

// 全局插件系统共享数据
extern std::string plugin_repository_url;
extern std::map<std::string, bool> plugin_installed_plugins;
extern std::map<std::string, std::string> plugin_command_to_plugin_map;
extern std::vector<IPlugin*> loaded_plugin_instances;
extern std::map<std::string, IPlugin*> plugin_name_to_instance;

#ifdef _WIN32
#include <windows.h>
typedef HMODULE PluginHandle;
#else
typedef void* PluginHandle;
#endif
extern std::map<std::string, PluginHandle> plugin_name_to_handle;

#endif // PLUGIN_COMMON_H