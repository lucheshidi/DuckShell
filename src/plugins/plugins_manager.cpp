// plugins/plugins_manager.cpp
#include "plugins_manager.h"
#include "../global_vars.h"
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>

#include "plugins_interface.h"

#ifdef _WIN32
    #include <windows.h>
    #include <wininet.h>
    // 移除可能导致问题的头文件
    // #include <filesystem>  // 注释掉这行
    #include <vector>
#else
    #include <dirent.h>
    #include <vector>
    #if defined(HAVE_LIBCURL)
        #include <curl/curl.h>
    #endif
#endif

#if defined(HAVE_MINIZIP)
    #include <zlib.h>
    #include <unzip.h>
#endif


// 定义静态成员变量
std::map<std::string, bool> PluginManager::installed_plugins;
std::string PluginManager::repository_url = "";

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
        printf("Failed to open plugins list file.\n");
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

        printf("Plugin '%s' installed successfully.\n", pluginName.c_str());
    }
    else {
        printf("Plugin '%s' not found in repository.\n", pluginName.c_str());
    }
}

void PluginManager::uninstallPlugin(const std::string& pluginName) {
    auto it = installed_plugins.find(pluginName);
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
                if (line.find(pluginName + ":") != 0) {
                    outfile << line << "\n";
                }
            }
            infile.close();
            outfile.close();

            remove(pluginsListFile.c_str());
            rename(tempFile.c_str(), pluginsListFile.c_str());
        }

        printf("Plugin '%s' uninstalled successfully.\n", pluginName.c_str());
    }
    else {
        printf("Plugin '%s' is not installed.\n", pluginName.c_str());
    }
}

void PluginManager::listAvailablePlugins() {
    std::string pluginsDir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = getDirectoryContents(pluginsDir);

    if (contents.empty()) {
        printf("No available plugins found.\n");
        return;
    }

    printf("Available plugins:\n");
    for (const auto& item : contents) {
        std::cout << "  " << item << std::endl;
    }
}

void PluginManager::listInstalledPlugins() {
    if (installed_plugins.empty()) {
        printf("No plugins installed.\n");
        return;
    }

    printf("Installed plugins:\n");
    for (const auto& pair : installed_plugins) {
        std::cout << "  " << pair.first << ": " << (pair.second ? "enabled" : "disabled") << std::endl;
    }
}

// 新增功能：设置仓库URL
void PluginManager::setRepositoryUrl(const std::string& url) {
    repository_url = url;
    printf("Repository URL set to: %s\n", url.c_str());
}

// 新增功能：列出远程插件
void PluginManager::listRemotePlugins() {
    if (repository_url.empty()) {
        printf("No repository URL configured. Use 'plugin repo set <url>' first.\n");
        return;
    }

    std::string apiUrl = repository_url + "/api/v1/plugins.json";

#ifdef _WIN32
    std::string jsonData;
    if (downloadWithWinInet(apiUrl, "temp_plugins.json")) {
        // 读取临时文件内容
        std::ifstream tempFile("temp_plugins.json");
        if (tempFile.is_open()) {
            std::string line;
            while (std::getline(tempFile, line)) {
                jsonData += line + "\n";
            }
            tempFile.close();
            remove("temp_plugins.json");
        }
        printf("Available remote plugins:\n");
        std::cout << jsonData << std::endl;
    }
    else {
        printf("Failed to fetch plugin list from repository.\n");
    }
#else
    #if defined(HAVE_LIBCURL)
        std::string jsonData;
        if (downloadWithCurl(apiUrl, "temp_plugins.json")) {
            std::ifstream tempFile("temp_plugins.json");
            if (tempFile.is_open()) {
                std::string line;
                while (std::getline(tempFile, line)) {
                    jsonData += line + "\n";
                }
                tempFile.close();
                remove("temp_plugins.json");
            }
            printf("Available remote plugins:\n");
            std::cout << jsonData << std::endl;
        } else {
            printf("Failed to fetch plugin list from repository.\n");
        }
    #else
        printf("Remote repository feature is not available in this build (libcurl not enabled).\n");
    #endif
#endif
}

// 新增功能：下载插件
void PluginManager::downloadPlugin(const std::string& pluginName) {
    if (repository_url.empty()) {
        printf("No repository URL configured. Use 'plugin repo set <url>' first.\n");
        return;
    }

    // 构建下载URL
    std::string downloadUrl = repository_url + "/downloads/" + pluginName + ".zip";
    std::string localPath = home_dir + "/duckshell/plugins/" + pluginName + ".zip";

#ifdef _WIN32
    if (downloadWithWinInet(downloadUrl, localPath)) {
        printf("Plugin '%s' downloaded successfully.\n", pluginName.c_str());
    }
    else {
        printf("Failed to download plugin '%s'\n", pluginName.c_str());
        remove(localPath.c_str()); // 删除失败的下载文件
        return;
    }
#else
    #if defined(HAVE_LIBCURL)
        if (downloadWithCurl(downloadUrl, localPath)) {
            printf("Plugin '%s' downloaded successfully.\n", pluginName.c_str());
        } else {
            printf("Failed to download plugin '%s'\n", pluginName.c_str());
            remove(localPath.c_str());
            return;
        }
    #else
        printf("Remote repository download is not available in this build (libcurl not enabled).\n");
        return;
    #endif
#endif

    // 下载成功后尝试自动解压（需要 minizip）。
    const std::string pluginsDir = home_dir + "/duckshell/plugins";
#if defined(HAVE_MINIZIP)
    if (unzipFile(localPath, pluginsDir)) {
        // 解压成功后，删除 zip 包并刷新已安装列表
        remove(localPath.c_str());
        installAllPlugins();
    } else {
        printf("Downloaded but failed to extract '%s'. You can extract manually: %s\n",
               pluginName.c_str(), localPath.c_str());
    }
#else
    printf("Zip extraction is not enabled in this build (minizip not found). The archive remains at: %s\n",
           localPath.c_str());
#endif
}

void PluginManager::installAllPlugins() {
    std::string pluginsDir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = getDirectoryContents(pluginsDir);

    if (contents.empty()) {
        printf("No plugins found to install.\n");
        return;
    }

    printf("Scanning for plugins to install...\n");

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
                printf("Plugin '%s' added for installation.\n", item.c_str());
                hasNewPlugins = true;
            } else {
                printf("Plugin '%s' already installed.\n", item.c_str());
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
            printf("Plugins list updated successfully.\n");
        }
    } else {
        printf("No new plugins to install.\n");
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
#endif
        }
    }
}

void PluginManager::executeCommand(const std::string& command, const std::vector<std::string>& cmdArgs) {
    // 查找命令对应的插件
    auto it = command_to_plugin_map.find(command);
    if (it == command_to_plugin_map.end()) {
        printf("Unknown command: %s\n", command.c_str());
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
        printf("Plugin '%s' is not installed.\n", resolvedName.c_str());
        return;
    }

    if (!it->second) {
        printf("Plugin '%s' is disabled.\n", resolvedName.c_str());
        return;
    }

    std::string pluginPath = home_dir + "/duckshell/plugins/" + resolvedName;

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(pluginPath.c_str());
    if (!handle) {
        printf("Failed to load plugin '%s'. Error: %lu\n", resolvedName.c_str(), GetLastError());
        return;
    }

    CreatePluginFunc createFunc = (CreatePluginFunc)GetProcAddress(handle, "createPlugin");
    if (!createFunc) {
        printf("Plugin '%s' does not export createPlugin function.\n", resolvedName.c_str());
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

#ifdef _WIN32
// Windows平台使用WinINet下载文件
bool PluginManager::downloadWithWinInet(const std::string& url, const std::string& outputFile) {
    HINTERNET hInternet = InternetOpenA("DuckShell/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    URL_COMPONENTSA urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwHostNameLength = 1;
    urlComp.dwUrlPathLength = 1;

    if (!InternetCrackUrlA(url.c_str(), 0, 0, &urlComp)) {
        InternetCloseHandle(hInternet);
        return false;
    }

    HINTERNET hConnect = InternetConnectA(hInternet, urlComp.lpszHostName,
                                         urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", urlComp.lpszUrlPath,
                                         NULL, NULL, NULL, 0, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    if (!HttpSendRequestA(hRequest, NULL, 0, NULL, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    FILE* fp = fopen(outputFile.c_str(), "wb");
    if (!fp) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, fp);
    }

    fclose(fp);
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return true;
}
#endif

#if !defined(_WIN32) && defined(HAVE_LIBCURL)
// Linux/Unix 平台使用 libcurl 下载文件
namespace {
    size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
        FILE* fp = static_cast<FILE*>(stream);
        return fwrite(ptr, size, nmemb, fp);
    }
}

bool PluginManager::downloadWithCurl(const std::string& url, const std::string& outputFile) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    FILE* fp = fopen(outputFile.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DuckShell/1.0");
    // 合理的超时设置
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        remove(outputFile.c_str());
        return false;
    }
    return true;
}
#endif

// ZIP 解压实现（基于 minizip 经典 API）。
// 仅当定义了 HAVE_MINIZIP 时可用；否则返回 false。
bool PluginManager::unzipFile(const std::string& zipPath, const std::string& outDir) {
#if defined(HAVE_MINIZIP)
    auto ensure_dir = [](const std::string& path) {
#ifdef _WIN32
        // 递归创建目录（简单实现）
        std::string curr;
        for (size_t i = 0; i < path.size(); ++i) {
            char c = path[i];
            curr.push_back(c);
            if (c == '\\' || c == '/') {
                if (curr.size() > 1) CreateDirectoryA(curr.c_str(), nullptr);
            }
        }
        if (!curr.empty()) CreateDirectoryA(curr.c_str(), nullptr);
#else
        // POSIX 递归创建目录
        std::string curr;
        for (size_t i = 0; i < path.size(); ++i) {
            char c = path[i];
            curr.push_back(c);
            if (c == '/') {
                if (curr.size() > 1) mkdir(curr.c_str(), 0755);
            }
        }
        if (!curr.empty()) mkdir(curr.c_str(), 0755);
#endif
    };

    unzFile uf = unzOpen(zipPath.c_str());
    if (!uf) {
        printf("Failed to open zip: %s\n", zipPath.c_str());
        return false;
    }

    if (unzGoToFirstFile(uf) != UNZ_OK) {
        unzClose(uf);
        printf("Empty or invalid zip: %s\n", zipPath.c_str());
        return false;
    }

    do {
        char filename_inzip[512] = {0};
        unz_file_info file_info{};
        if (unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0) != UNZ_OK) {
            unzClose(uf);
            printf("Failed to read file info in zip: %s\n", zipPath.c_str());
            return false;
        }

        std::string fullPath = outDir + "/" + filename_inzip;
        // 目录条目
        if (filename_inzip[strlen(filename_inzip)-1] == '/' || filename_inzip[strlen(filename_inzip)-1] == '\\') {
            ensure_dir(fullPath);
        } else {
            // 创建父目录
            size_t pos = fullPath.find_last_of("/\\");
            if (pos != std::string::npos) ensure_dir(fullPath.substr(0, pos));

            if (unzOpenCurrentFile(uf) != UNZ_OK) {
                unzClose(uf);
                printf("Failed to open entry in zip: %s\n", filename_inzip);
                return false;
            }

            FILE* fp = fopen(fullPath.c_str(), "wb");
            if (!fp) {
                unzCloseCurrentFile(uf);
                unzClose(uf);
                printf("Failed to create file: %s\n", fullPath.c_str());
                return false;
            }

            char buf[8192];
            int readBytes = 0;
            while ((readBytes = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
                fwrite(buf, 1, static_cast<size_t>(readBytes), fp);
            }
            fclose(fp);
            unzCloseCurrentFile(uf);
        }
    } while (unzGoToNextFile(uf) == UNZ_OK);

    unzClose(uf);
    return true;
#else
    (void)zipPath; (void)outDir;
    return false;
#endif
}
