// plugins/plugin_installer.h
#ifndef PLUGIN_INSTALLER_H
#define PLUGIN_INSTALLER_H

#include <string>
#include <vector>
#include "plugin_common.h"

class PluginInstaller {
public:
    static void install_plugin(const std::string& plugin_name);
    static void uninstall_plugin(const std::string& plugin_name);
    static void remove_plugin(const std::string& plugin_name);
    static void enable_plugin(const std::string& plugin_name);
    static void disable_plugin(const std::string& plugin_name);
    static void list_available_plugins();
    static void list_installed_plugins();
    static void install_all_plugins();

private:
    static void update_plugins_list_file();
};

#endif // PLUGIN_INSTALLER_H