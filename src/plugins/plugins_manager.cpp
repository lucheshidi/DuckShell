// plugins/plugins_manager.cpp
#include "plugins_manager.h"
#include "../global_vars.h"
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>

#include "plugins_interface.h"
#include "../header.h"

#include "../json.hpp" // 引入头文件
using json = nlohmann::json;

#ifdef _WIN32
#include <windows.h>
// 移除可能导致问题的头文件
// #include <filesystem>  // 注释掉这行
#include <vector>
#else
#include <dirent.h>
#include <dlfcn.h>      // Unix 动态库加载
#include <vector>
#endif

#include <cstring>

// 条件包含curl头文件，确保在没有curl开发包时也能编译
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#else
// 如果没有curl支持，提供空的函数实现
#warning "libcurl development package not found. Plugin download functionality will be disabled."

// 提供假的curl类型定义以避免编译错误
typedef void CURL;
typedef enum {
    CURLE_OK = 0,
    // 添加其他常用的CURLcode值
} CURLcode;

// 提供假的函数声明
static inline CURL* curl_easy_init() { return nullptr; }
static inline void curl_easy_cleanup(CURL* curl) {}
static inline CURLcode curl_easy_perform(CURL* curl) { return CURLE_OK; }
static inline CURLcode curl_easy_setopt(CURL* curl, int option, ...) { return CURLE_OK; }
static inline CURLcode curl_easy_getinfo(CURL* curl, int info, ...) { return CURLE_OK; }
static inline const char* curl_easy_strerror(CURLcode code) { return "CURL not available"; }

// 常用的CURLOPT_*常量
#define CURLOPT_URL 0
#define CURLOPT_FOLLOWLOCATION 1
#define CURLOPT_WRITEFUNCTION 2
#define CURLOPT_WRITEDATA 3
#define CURLOPT_USERAGENT 4
#define CURLOPT_TIMEOUT 5
#define CURLOPT_CONNECTTIMEOUT 6
#define CURLOPT_VERBOSE 7
#define CURLOPT_SSL_OPTIONS 8
#define CURLOPT_CAINFO 9
#define CURLOPT_DEBUGFUNCTION 10
#define CURLINFO_RESPONSE_CODE 11
#define CURLSSLOPT_NATIVE_CA 12

#endif // HAVE_LIBCURL

#ifdef __APPLE__
#include <unistd.h>  // macOS需要这个头文件来使用unlink函数
#endif

// 辅助函数：更新插件列表文件
void PluginManager::updatePluginsListFile() {
    std::string pluginsListFile = home_dir + "/duckshell/plugins.ls";
    std::ofstream outfile(pluginsListFile);
    
    if (outfile.is_open()) {
        for (const auto& pair : installed_plugins) {
            outfile << pair.first << ":" << (pair.second ? "enabled" : "disabled") << "\n";
        }
        outfile.close();
    }
}


// 定义静态成员变量
std::map<std::string, bool> PluginManager::installed_plugins;
std::string PluginManager::repository_url = "https://dsrepo.lucheshidi.dpdns.org";

namespace {
    size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
        return fwrite(ptr, size, nmemb, stream);
    }
}

#ifdef _WIN32
std::vector<std::string> PluginManager::getDirectoryContents(const std::string& path) {
    std::vector<std::string> contents;

    // 确保路径格式正确
    std::string searchPath = path;
    if (searchPath.back() != '\\' && searchPath.back() != '/') {
        searchPath += "\\";
    }
    searchPath += "*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string name(findData.cFileName);
            if (name != "." && name != "..") {
                contents.push_back(name);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    return contents;
}
#else
std::vector<std::string> PluginManager::getDirectoryContents(const std::string& path) {
    std::vector<std::string> contents;
    DIR* dir = opendir(path.c_str());

    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name(entry->d_name);
            if (name != "." && name != "..") {
                contents.push_back(name);
            }
        }
        closedir(dir);
    }

    return contents;
}
#endif

void PluginManager::loadPlugins() {
    std::string pluginsListFile = home_dir + "/duckshell/plugins.ls";
    std::ifstream infile(pluginsListFile);

    if (!infile.is_open()) {
        println("Failed to open plugins list file.\n");
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty() && line[0] != '/' && line.find("{") == std::string::npos &&
            line.find("}") == std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string pluginName = line.substr(0, colonPos);
                std::string status = line.substr(colonPos + 1);
                installed_plugins[pluginName] = (status == "enabled");
            }
        }
    }
    infile.close();
}

void PluginManager::installPlugin(const std::string& pluginName) {
    std::string pluginPath = home_dir + "/duckshell/plugins/" + pluginName;
    struct stat buffer;

    if (stat(pluginPath.c_str(), &buffer) == 0) {
        installed_plugins[pluginName] = true;

        // 更新插件列表文件
        std::string pluginsListFile = home_dir + "/duckshell/plugins.ls";
        std::ofstream outfile(pluginsListFile, std::ios::app);
        if (outfile.is_open()) {
            outfile << pluginName << ":enabled\n";
            outfile.close();
        }

        println("Plugin '" << pluginName.c_str() << "' installed successfully.\n");
    }
    else {
        println("Plugin '" << pluginName.c_str() << "' not found in repository.\n");
    }
}

void PluginManager::uninstallPlugin(const std::string& pluginName) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolvedName = resolvePluginName(pluginName);
    
    auto it = installed_plugins.find(resolvedName);
    if (it != installed_plugins.end()) {
        installed_plugins.erase(it);

        // 重新写入插件列表文件（移除已卸载的插件）
        std::string pluginsListFile = home_dir + "/duckshell/plugins.ls";
        std::string tempFile = pluginsListFile + ".tmp";

        std::ifstream infile(pluginsListFile);
        std::ofstream outfile(tempFile);

        if (infile.is_open() && outfile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                // 修复删除逻辑：检查行是否以插件名开头且后面紧跟冒号
                if (line.substr(0, resolvedName.length()) != resolvedName ||
                    (line.length() > resolvedName.length() && line[resolvedName.length()] != ':')) {
                    outfile << line << "\n";
                }
            }
            infile.close();
            outfile.close();

            // 原子性替换文件
            if (remove(pluginsListFile.c_str()) != 0) {
                println("Warning: Failed to remove old plugins list file.\n");
            }
            if (rename(tempFile.c_str(), pluginsListFile.c_str()) != 0) {
                println("Error: Failed to rename temporary file.\n");
            }
        } else {
            println("Error: Failed to open plugins list files for update.\n");
        }

        println("Plugin '" << resolvedName.c_str() << "' uninstalled successfully.\n");
    } else {
        println("Plugin '" << resolvedName.c_str() << "' is not installed.\n");
    }
}

void PluginManager::removePlugin(const std::string& pluginName) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolvedName = resolvePluginName(pluginName);
    
    // 首先卸载插件（从内存和列表文件中移除）
    uninstallPlugin(resolvedName);
    
    // 然后删除插件文件
    std::string pluginPath = home_dir + "/duckshell/plugins/" + resolvedName;
    
#ifdef _WIN32
    if (DeleteFileA(pluginPath.c_str()) != 0) {
        println("Plugin file '" << resolvedName.c_str() << "' removed successfully.\n");
    } else {
        println("Failed to remove plugin file '" << resolvedName.c_str() << "'.\n");
    }
#else
    if (unlink(pluginPath.c_str()) == 0) {
        println("Plugin file '" << resolvedName.c_str() << "' removed successfully.\n");
    } else {
        println("Failed to remove plugin file '" << resolvedName.c_str() << "'.\n");
    }
#endif
}

void PluginManager::enablePlugin(const std::string& pluginName) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolvedName = resolvePluginName(pluginName);
    
    auto it = installed_plugins.find(resolvedName);
    if (it != installed_plugins.end()) {
        if (it->second) {
            println("Plugin '" << resolvedName.c_str() << "' is already enabled.\n");
        } else {
            installed_plugins[resolvedName] = true;
            
            // 更新插件列表文件
            updatePluginsListFile();
            
            println("Plugin '" << resolvedName.c_str() << "' enabled successfully.\n");
        }
    } else {
        println("Plugin '" << resolvedName.c_str() << "' is not installed.\n");
    }
}

void PluginManager::disablePlugin(const std::string& pluginName) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolvedName = resolvePluginName(pluginName);
    
    auto it = installed_plugins.find(resolvedName);
    if (it != installed_plugins.end()) {
        if (!it->second) {
            println("Plugin '" << resolvedName.c_str() << "' is already disabled.\n");
        } else {
            installed_plugins[resolvedName] = false;
            
            // 更新插件列表文件
            updatePluginsListFile();
            
            println("Plugin '" << resolvedName.c_str() << "' disabled successfully.\n");
        }
    } else {
        println("Plugin '" << resolvedName.c_str() << "' is not installed.\n");
    }
}

void PluginManager::listAvailablePlugins() {
    std::string pluginsDir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = getDirectoryContents(pluginsDir);

    if (contents.empty()) {
        println("No available plugins found.\n");
        return;
    }

    println("Available plugins:\n");
    for (const auto& item : contents) {
        std::cout << "  " << item << std::endl;
    }
}

void PluginManager::listInstalledPlugins() {
    if (installed_plugins.empty()) {
        println("No plugins installed.\n");
        return;
    }

    println("Installed plugins:\n");
    for (const auto& pair : installed_plugins) {
        std::cout << "  " << pair.first << ": " << (pair.second ? "enabled" : "disabled") << std::endl;
    }
}

// 新增功能：设置仓库URL
void PluginManager::setRepositoryUrl(const std::string& url) {
    repository_url = url;
    println("Repository URL set to: " << url.c_str());
}

// 新增功能：列出远程插件
void PluginManager::listRemotePlugins() {
    if (repository_url.empty()) return;

    // 访问我们设计的 PyPI 风格索引页
    std::string indexUrl = repository_url + "/simple/index.html";
    std::string tempHtml = home_dir + "/duckshell/temp_index.html";

    if (internalDownload(indexUrl, tempHtml)) {
        std::ifstream file(tempHtml);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        remove(tempHtml.c_str());

        println("--- Remote Plugin Repository ---");

        // 使用正则或简单的字符串查找抓取 <a> 标签
        // 格式预期：<a href="/packages/test-plugin/">test-plugin</a>
        size_t pos = 0;
        bool found = false;
        while ((pos = content.find("<a href=", pos)) != std::string::npos) {
            size_t start = content.find(">", pos) + 1;
            size_t end = content.find("</a>", start);
            if (start != std::string::npos && end != std::string::npos) {
                std::string name = content.substr(start, end - start);
                println("  * " << name.c_str());
                found = true;
            }
            pos = end;
        }
        if(!found) println(" (No plugins found in index)");
    } else {
        println("Failed to connect to repository index.");
    }
}

// 新增功能：下载插件
void PluginManager::downloadPlugin(const std::string& pluginName) {
    if (repository_url.empty()) return;

    // 1. 准备元数据路径
    std::string baseUrl = repository_url + "/packages/" + pluginName;
    std::string metaUrl = baseUrl + "/plugin.json";
    std::string tempJson = home_dir + "/duckshell/temp_meta.json";

    println("Fetching metadata: " << metaUrl.c_str());

    // 2. 下载并解析 JSON
    if (!internalDownload(metaUrl, tempJson)) {
        println(RED << "Error: Could not reach repository or plugin not found." << RESET);
        return;
    }

    try {
        std::ifstream f(tempJson);
        json j = json::parse(f);
        f.close();
        // remove(tempJson.c_str()); // 调试阶段可以先不删

        // 3. 确定平台后缀
#ifdef _WIN32
        std::string currentPlatform = "windows";
        std::string ext = ".dll";
#else
        std::string currentPlatform = "linux";
        std::string ext = ".so";
#endif

        if (!j["platforms"].contains(currentPlatform) || j["platforms"][currentPlatform] == false) {
            println(RED << "Error: Plugin does not support " << currentPlatform << RESET);
            return;
        }

        // 4. 版本号与路径处理 (包含 'v' 前缀容错)
        std::string version = j["latest_version"];
        std::string versionPath = version;
        if (!versionPath.empty() && versionPath[0] != 'v' && versionPath[0] != 'V') {
            versionPath = "v" + version;
        }

        // 5. 执行二进制下载
        // 结构: .../packages/PluginName/v1.0/PluginName.dll
        std::string binaryUrl = baseUrl + "/" + versionPath + "/" + pluginName + ext;
        std::string localPath = home_dir + "/duckshell/plugins/" + pluginName + ext;

        println("Found version " << version << ". Downloading binary...");

        if (internalDownload(binaryUrl, localPath)) {
            println(GREEN << "Successfully installed: " << pluginName << RESET);
            installAllPlugins(); // 刷新内存中的插件映射
        }
        else {
            println(RED << "Error: Failed to download binary file." << RESET);
        }

    }
    catch (const std::exception& e) {
        println(RED << "JSON/Logic Error: " << e.what() << RESET);
    }
}

void PluginManager::installAllPlugins() {
    std::string pluginsDir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = getDirectoryContents(pluginsDir);

    if (contents.empty()) {
        println("No plugins found to install.");
        return;
    }

    println("Scanning for plugins to install...");

    // 记录初始插件数量
    size_t initialPluginCount = installed_plugins.size();
    bool hasNewPlugins = false;

    for (const auto& item : contents) {
        // 检查是否为插件文件（根据扩展名判断）
        if (item.find(".dll") != std::string::npos ||
            item.find(".so") != std::string::npos) {
            // 检查插件是否已经安装
            auto it = installed_plugins.find(item);
            if (it == installed_plugins.end()) {
                // 只有未安装的插件才添加
                installed_plugins[item] = true;
                println("Plugin '" << item.c_str() << "' added for installation.");
                hasNewPlugins = true;
            } else {
                println("Plugin '"  << item.c_str() << "' already installed.");
            }
            }
    }

    // 只有在有新插件添加时才重写文件
    if (hasNewPlugins) {
        // 重新写入整个插件列表文件，而不是追加
        std::string pluginsListFile = home_dir + "/duckshell/plugins.ls";
        std::ofstream outfile(pluginsListFile);
        if (outfile.is_open()) {
            for (const auto& pair : installed_plugins) {
                outfile << pair.first << ":" << (pair.second ? "enabled" : "disabled") << "\n";
            }
            outfile.close();
            println("Plugins list updated successfully.");
        }
    } else {
        println("No new plugins to install.");
    }
}

std::map<std::string, std::string> PluginManager::command_to_plugin_map;

void PluginManager::buildCommandMap() {
    command_to_plugin_map.clear();

    for (const auto& pair : installed_plugins) {
        if (pair.second) { // 只处理启用的插件
            std::string pluginPath = home_dir + "/duckshell/plugins/" + pair.first;

#ifdef _WIN32
            HMODULE handle = LoadLibraryA(pluginPath.c_str());
            if (handle) {
                CreatePluginFunc createFunc = (CreatePluginFunc)GetProcAddress(handle, "createPlugin");
                if (createFunc) {
                    IPlugin* plugin = createFunc();
                    if (plugin) {
                        // 获取插件支持的命令别名
                        auto aliases = plugin->getCommandAliases();
                        for (const auto& alias : aliases) {
                            command_to_plugin_map[alias] = pair.first;
                        }

                        if (auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(GetProcAddress(handle, "destroyPlugin"))) {
                            destroyFunc(plugin);
                        }
                    }
                }
                FreeLibrary(handle);
            }
#else
            // Unix/Linux 系统使用 dlopen/dlsym
            void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
            if (handle) {
                CreatePluginFunc createFunc = (CreatePluginFunc)dlsym(handle, "createPlugin");
                if (createFunc && dlerror() == NULL) {  // 检查错误
                    IPlugin* plugin = createFunc();
                    if (plugin) {
                        // 获取插件支持的命令别名
                        auto aliases = plugin->getCommandAliases();
                        for (const auto& alias : aliases) {
                            command_to_plugin_map[alias] = pair.first;
                        }

                        DestroyPluginFunc destroyFunc = (DestroyPluginFunc)dlsym(handle, "destroyPlugin");
                        if (destroyFunc && dlerror() == NULL) {
                            destroyFunc(plugin);
                        }
                    }
                }
                dlclose(handle);
            }
#endif
        }
    }
}

void PluginManager::executeCommand(const std::string& command, const std::vector<std::string>& cmdArgs) {
    // 查找命令对应的插件
    auto it = command_to_plugin_map.find(command);
    if (it == command_to_plugin_map.end()) {
        println("Unknown command: " << command.c_str());
        return;
    }

    std::string pluginName = it->second;
    executePluginWithCommand(pluginName, cmdArgs);
}

// 在 plugins_manager.cpp 中添加实现
void PluginManager::executePluginWithCommand(const std::string& pluginName, const std::vector<std::string>& cmdArgs) {
    // 解析插件名称
    std::string resolvedName = resolvePluginName(pluginName);

    auto it = installed_plugins.find(resolvedName);
    if (it == installed_plugins.end()) {
        println("Plugin '" << resolvedName.c_str() << "' is not installed.\n");
        return;
    }

    if (!it->second) {
        println("Plugin '" << resolvedName.c_str() << "' is disabled.\n");
        return;
    }

    std::string pluginPath = home_dir + "/duckshell/plugins/" + resolvedName;

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(pluginPath.c_str());
    if (!handle) {
        println("Failed to load plugin '" << resolvedName.c_str() << "'. Error: " << GetLastError());
        return;
    }

    CreatePluginFunc createFunc = (CreatePluginFunc)GetProcAddress(handle, "createPlugin");
    if (!createFunc) {
        println("Plugin '" << resolvedName.c_str() << "' does not export createPlugin function.\n");
        FreeLibrary(handle);
        return;
    }

    IPlugin* plugin = createFunc();
    if (plugin) {
        // 将命令参数传递给插件
        std::map<std::string, std::string> params;
        for (size_t i = 0; i < cmdArgs.size(); ++i) {
            params["arg" + std::to_string(i)] = cmdArgs[i];
        }
        params["argc"] = std::to_string(cmdArgs.size());

        // 触发一个事件让插件处理命令参数
        plugin->onEvent("command_execute", params);

        // 同时执行标准的 execute 方法
        plugin->execute();

        DestroyPluginFunc destroyFunc = (DestroyPluginFunc)GetProcAddress(handle, "destroyPlugin");
        if (destroyFunc) {
            destroyFunc(plugin);
        }
        FreeLibrary(handle);
    }
#else
    // Unix/Linux 系统使用 dlopen/dlsym
    void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
    if (!handle) {
        println("Failed to load plugin '" << resolvedName.c_str() << "'. Error: " << dlerror());
        return;
    }

    // 清除之前的错误
    dlerror();
    CreatePluginFunc createFunc = (CreatePluginFunc)dlsym(handle, "createPlugin");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        println("Plugin '" << resolvedName.c_str() << "' does not export createPlugin function. Error: " << dlsym_error);
        dlclose(handle);
        return;
    }

    IPlugin* plugin = createFunc();
    if (plugin) {
        // 将命令参数传递给插件
        std::map<std::string, std::string> params;
        for (size_t i = 0; i < cmdArgs.size(); ++i) {
            params["arg" + std::to_string(i)] = cmdArgs[i];
        }
        params["argc"] = std::to_string(cmdArgs.size());

        // 触发一个事件让插件处理命令参数
        plugin->onEvent("command_execute", params);

        // 同时执行标准的 execute 方法
        plugin->execute();

        // 清除之前的错误
        dlerror();
        DestroyPluginFunc destroyFunc = (DestroyPluginFunc)dlsym(handle, "destroyPlugin");
        dlsym_error = dlerror();
        if (!dlsym_error && destroyFunc) {
            destroyFunc(plugin);
        }
        dlclose(handle);
    }
#endif
}



bool PluginManager::isPluginInstalled(const std::string& pluginName) {
    auto it = installed_plugins.find(pluginName);
    return (it != installed_plugins.end() && it->second);
}

bool PluginManager::isPluginCommand(const std::string& command) {
    // 可以维护一个特殊命令列表，避免与插件名冲突
    static std::set<std::string> builtinCommands = {"help", "exit", "cd", "ls", "plugin"};
    return builtinCommands.find(command) != builtinCommands.end();
}

// 在 PluginManager 类中添加声明
static std::string resolvePluginName(const std::string& pluginName);

// 在 plugins_manager.cpp 中实现
std::string PluginManager::resolvePluginName(const std::string& pluginName) {
    // 如果已经包含扩展名，直接返回
    if (pluginName.find(".dll") != std::string::npos ||
        pluginName.find(".so") != std::string::npos) {
        return pluginName;
        }

    // 尝试添加常见的扩展名
#ifdef _WIN32
    std::string fullName = pluginName + ".dll";
#else
    std::string fullName = pluginName + ".so";
#endif

    // 检查文件是否存在
    std::string pluginPath = home_dir + "/duckshell/plugins/" + fullName;
    struct stat buffer;
    if (stat(pluginPath.c_str(), &buffer) == 0) {
        return fullName;
    }

    // 如果找不到，返回原始名称
    return pluginName;
}

bool PluginManager::internalDownload(const std::string& url, const std::string& localPath) {
    println("Attempting to download: " << url);
    println("Saving to: " << localPath);
    
#ifndef HAVE_LIBCURL
    println(RED << "Error: libcurl development package not found. Plugin download functionality is disabled." << RESET);
    println("Please install libcurl development package:");
#ifdef __linux__
    println("  Ubuntu/Debian: sudo apt-get install libcurl4-openssl-dev");
    println("  CentOS/RHEL: sudo yum install libcurl-devel");
#endif
    return false;
#else
    CURL* curl = curl_easy_init();
    if (!curl) {
        println(RED << "Error: Failed to initialize CURL" << RESET);
        return false;
    }

    FILE* fp = fopen(localPath.c_str(), "wb");
    if (!fp) {
        println(RED << "File Error: Cannot create " << localPath << RESET);
        curl_easy_cleanup(curl);
        return false;
    }

    // 基本设置
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DuckShell/1.0");
    
    // 增加超时设置
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);  // 30秒超时
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);  // 10秒连接超时
    
    // 减少调试信息显示
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    
    // 只在出现错误时显示详细信息
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, [](CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) -> int {
        if (type == CURLINFO_TEXT && (strstr(data, "schannel:") == nullptr || strstr(data, "failed to decrypt") == nullptr)) {
            printf("* %.*s", (int)size, data);
        }
        return 0;
    });

    // HTTPS 关键设置 (针对不同平台自动适配)
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#else
    // Linux下可能需要指定CA证书路径
    curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");
#endif

    // 执行请求
    println("Starting download...");
    CURLcode res = curl_easy_perform(curl);

    // 获取 HTTP 状态码
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        println(RED << "Curl Error: " << curl_easy_strerror(res) << RESET);
        println(RED << "Error code: " << res << RESET);
        std::remove(localPath.c_str()); // 失败了就清理掉空文件
        return false;
    }

    if (response_code != 200) {
        println(YELLOW << "HTTP Error: " << response_code << " (URL: " << url << ")" << RESET);
        std::remove(localPath.c_str());
        return false;
    }

    println(GREEN << "Download completed successfully!" << RESET);
    return true;
#endif // HAVE_LIBCURL
}