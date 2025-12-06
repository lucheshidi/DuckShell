// plugins/plugins_manager.h
#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include <string>
#include <map>
#include <vector>

class PluginManager {
public:
    static void loadPlugins();
    static void installPlugin(const std::string& pluginName);
    static void uninstallPlugin(const std::string& pluginName);
    static void listAvailablePlugins();
    static void listInstalledPlugins();

    // 新增插件仓库功能
    static void setRepositoryUrl(const std::string& url);
    static void listRemotePlugins();
    static void downloadPlugin(const std::string& pluginName);

private:
    static std::map<std::string, bool> installed_plugins;
    static std::string repository_url;
    static std::vector<std::string> getDirectoryContents(const std::string& path);

#ifdef _WIN32
    static bool downloadWithWinInet(const std::string& url, const std::string& outputFile);
#endif
};

#endif // PLUGINS_MANAGER_H
