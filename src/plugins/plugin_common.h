#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <string>
#include <map>
#include <vector>

// 全局插件系统共享数据
extern std::string plugin_repository_url;
extern std::map<std::string, bool> plugin_installed_plugins;
extern std::map<std::string, std::string> plugin_command_to_plugin_map;

#endif // PLUGIN_COMMON_H