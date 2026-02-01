// plugins/plugin_loader.h
#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include "plugin_common.h"

class PluginLoader {
public:
    static void load_plugins();
    static void build_command_map();
    static bool is_plugin_installed(const std::string& plugin_name);
    static bool is_plugin_command(const std::string& command);
    static std::string resolve_plugin_name(const std::string& plugin_name);
    static std::vector<std::string> get_directory_contents(const std::string& path);

    static std::string& repository_url() { return plugin_repository_url; }
    static std::map<std::string, bool>& installed_plugins() { return plugin_installed_plugins; }
    static std::map<std::string, std::string>& command_to_plugin_map() { return plugin_command_to_plugin_map; }
};

#endif // PLUGIN_LOADER_H