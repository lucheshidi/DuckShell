// plugins/plugins_manager.cpp
#include "plugins_manager.h"
#include "../header.h"
#ifdef _WIN32
// Windows平台使用替代方案
#include <windows.h>
#include <tchar.h>
// 或者使用std::filesystem (需要C++17)
#include <filesystem>
#else
// Unix/Linux平台
#include <dirent.h>
#include <sys/stat.h>
#endif


// 定义静态成员变量
std::map<std::string, bool> PluginManager::installed_plugins;
std::string PluginManager::repository_url = "";

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

void PluginManager::loadPlugins() {
    std::string pluginsListFile = home_dir + "/duckshell/plugins.ls";
    std::ifstream infile(pluginsListFile);

    if (!infile.is_open()) {
        printf(RED << BOLD << "Failed to open plugins list file." << RESET);
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

        printf(GREEN << BOLD << "Plugin '" << pluginName << "' installed successfully." << RESET);
    }
    else {
        printf(RED << BOLD << "Plugin '" << pluginName << "' not found in repository." << RESET);
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

        printf(GREEN << BOLD << "Plugin '" << pluginName << "' uninstalled successfully." << RESET);
    }
    else {
        printf(RED << BOLD << "Plugin '" << pluginName << "' is not installed." << RESET);
    }
}

void PluginManager::listAvailablePlugins() {
    std::string pluginsDir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = getDirectoryContents(pluginsDir);

    if (contents.empty()) {
        printf(YELLOW << "No available plugins found." << RESET);
        return;
    }

    printf(BOLD << "Available plugins:" << RESET);
    for (const auto& item : contents) {
        std::cout << "  " << item << std::endl;
    }
}

void PluginManager::listInstalledPlugins() {
    if (installed_plugins.empty()) {
        printf(YELLOW << "No plugins installed." << RESET);
        return;
    }

    printf(BOLD << "Installed plugins:" << RESET);
    for (const auto& pair : installed_plugins) {
        std::cout << "  " << pair.first << ": " << (pair.second ? "enabled" : "disabled") << std::endl;
    }
}

// 新增功能：设置仓库URL
void PluginManager::setRepositoryUrl(const std::string& url) {
    repository_url = url;
    printf(GREEN << "Repository URL set to: " << url << RESET);
}

// 新增功能：列出远程插件
void PluginManager::listRemotePlugins() {
    if (repository_url.empty()) {
        printf(RED << "No repository URL configured. Use 'plugin repo set <url>' first." << RESET);
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
        printf(BOLD << "Available remote plugins:" << RESET);
        //printf("%s", jsonData.c_str());
        std::cout << jsonData << std::endl;
    }
    else {
        printf(RED << "Failed to fetch plugin list from repository." << RESET);
    }
#endif
}

// 新增功能：下载插件
void PluginManager::downloadPlugin(const std::string& pluginName) {
    if (repository_url.empty()) {
        printf(RED << "No repository URL configured. Use 'plugin repo set <url>' first." << RESET);
        return;
    }

    // 构建下载URL
    std::string downloadUrl = repository_url + "/downloads/" + pluginName + ".zip";
    std::string localPath = home_dir + "/duckshell/plugins/" + pluginName + ".zip";

#ifdef _WIN32
    if (downloadWithWinInet(downloadUrl, localPath)) {
        printf(GREEN << BOLD << "Plugin '" << pluginName << "' downloaded successfully." << RESET);
        // 这里可以添加解压逻辑
    }
    else {
        printf(RED << "Failed to download plugin '" << pluginName << "'" << RESET);
        remove(localPath.c_str()); // 删除失败的下载文件
    }
#endif
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
