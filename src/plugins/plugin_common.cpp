#include "plugin_common.h"
#include "../global_vars.h"
#include <fstream>
#include <functional>

// 全局插件系统共享数据定义
std::string plugin_repository_url = "https://dsrepo.lucheshidi.dpdns.org";
std::list<std::string> plugin_repository_urls = {"https://dsrepo.lucheshidi.dpdns.org"};
std::map<std::string, bool> plugin_installed_plugins;
std::map<std::string, std::string> plugin_command_to_plugin_map;
std::vector<IPlugin*> loaded_plugin_instances;
std::map<std::string, IPlugin*> plugin_name_to_instance;
std::map<std::string, PluginHandle> plugin_name_to_handle;

// 工具函数实现
void load_repository_urls_from_file() {
    std::string repo_file = home_dir + "/duckshell/repo.ls";
    std::ifstream file(repo_file);
    
    if (file.is_open()) {
        plugin_repository_urls.clear();
        std::string line;
        while (std::getline(file, line)) {
            // 忽略空行和注释
            if (!line.empty() && line[0] != '#' && line.substr(0, 8) == "https://") {
                // 去除首尾空白字符
                size_t start = line.find_first_not_of(" \t\r\n");
                size_t end = line.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    std::string url = line.substr(start, end - start + 1);
                    plugin_repository_urls.push_back(url);
                }
            }
        }
        file.close();
        
        // 如果文件中没有有效的URL，使用默认URL
        if (plugin_repository_urls.empty()) {
            plugin_repository_urls.push_back("https://dsrepo.lucheshidi.dpdns.org");
        }
        
        // 更新主仓库URL为第一个URL
        plugin_repository_url = plugin_repository_urls.front();
    } else {
        // 如果文件不存在，保持默认值
        plugin_repository_urls.clear();
        plugin_repository_urls.push_back("https://dsrepo.lucheshidi.dpdns.org");
        plugin_repository_url = plugin_repository_urls.front();
    }
}

std::string get_primary_repository_url() {
    if (!plugin_repository_urls.empty()) {
        return plugin_repository_urls.front();
    }
    return "https://dsrepo.lucheshidi.dpdns.org";
}

bool try_repository_with_fallback(const std::function<bool(const std::string&)>& operation) {
    for (const auto& url : plugin_repository_urls) {
        if (operation(url)) {
            // 如果操作成功，更新当前使用的仓库URL
            plugin_repository_url = url;
            return true;
        }
    }
    return false;
}