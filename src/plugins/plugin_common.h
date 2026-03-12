#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <string>
#include <map>
#include <vector>
#include <list>

class IPlugin;

// 全局插件系统共享数据
extern std::string plugin_repository_url;
extern std::list<std::string> plugin_repository_urls;
extern std::map<std::string, bool> plugin_installed_plugins;
extern std::map<std::string, std::string> plugin_command_to_plugin_map;
extern std::vector<IPlugin*> loaded_plugin_instances;
extern std::map<std::string, IPlugin*> plugin_name_to_instance;

#ifdef _WIN32
#include <windows.h>
#include <functional>
typedef HMODULE PluginHandle;
#else
#include <functional>
typedef void* PluginHandle;
#endif
extern std::map<std::string, PluginHandle> plugin_name_to_handle;

// 工具函数声明
void load_repository_urls_from_file();
std::string get_primary_repository_url();
bool try_repository_with_fallback(const std::function<bool(const std::string&)>& operation);

#endif // PLUGIN_COMMON_H