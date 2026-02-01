#include "plugin_common.h"

// 全局插件系统共享数据定义
std::string plugin_repository_url = "https://dsrepo.lucheshidi.dpdns.org";
std::map<std::string, bool> plugin_installed_plugins;
std::map<std::string, std::string> plugin_command_to_plugin_map;