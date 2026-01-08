// plugins/plugins_manager.h
#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include <algorithm>
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

    static void installAllPlugins();  // 自动安装所有可用插件
    static void executePluginWithCommand(const std::string& pluginName, const std::vector<std::string>& cmdArgs);
    static bool isPluginInstalled(const std::string& pluginName);
    static bool isPluginCommand(const std::string& command);
    static std::string resolvePluginName(const std::string& pluginName);

    static void buildCommandMap();
    static void executeCommand(const std::string& command, const std::vector<std::string>& cmdArgs);

private:
    static std::map<std::string, bool> installed_plugins;
    static std::string repository_url;
    static std::vector<std::string> getDirectoryContents(const std::string& path);

#ifdef _WIN32
    static bool downloadWithWinInet(const std::string& url, const std::string& outputFile);
#endif
#ifndef _WIN32
    static bool downloadWithCurl(const std::string& url, const std::string& outputFile);
#endif
    // ZIP 解压（最小改动方案）。需要 HAVE_MINIZIP 才会有实际实现。
    static bool unzipFile(const std::string& zipPath, const std::string& outDir);
    static std::map<std::string, std::string> command_to_plugin_map; // command -> plugin_name 映射

};

#endif // PLUGINS_MANAGER_H
