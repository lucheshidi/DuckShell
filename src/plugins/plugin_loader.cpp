#include "plugin_loader.h"
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

// 定义静态成员变量


void PluginLoader::load_plugins() {
    std::string plugins_list_file = home_dir + "/duckshell/plugins.ls";
    std::ifstream infile(plugins_list_file);

    if (!infile.is_open()) {
        println("Failed to open plugins list file.\n");
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty() && line[0] != '/' && line.find("{") == std::string::npos &&
            line.find("}") == std::string::npos) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string plugin_name = line.substr(0, colon_pos);
                std::string status = line.substr(colon_pos + 1);
                PluginLoader::installed_plugins()[plugin_name] = (status == "enabled");
            }
        }
    }
    infile.close();
}

void PluginLoader::build_command_map() {
    PluginLoader::command_to_plugin_map().clear();

    for (const auto& pair : PluginLoader::installed_plugins()) {
        if (pair.second) { // 只处理启用的插件
            std::string plugin_path = home_dir + "/duckshell/plugins/" + pair.first;

#ifdef _WIN32
            HMODULE handle = LoadLibraryA(plugin_path.c_str());
            if (handle) {
                CreatePluginFunc create_func = (CreatePluginFunc)GetProcAddress(handle, "createPlugin");
                if (create_func) {
                    IPlugin* plugin = create_func();
                    if (plugin) {
                        // 获取插件支持的命令别名
                        auto aliases = plugin->getCommandAliases();
                        for (const auto& alias : aliases) {
                            PluginLoader::command_to_plugin_map()[alias] = pair.first;
                        }

                        if (auto destroy_func = reinterpret_cast<DestroyPluginFunc>(GetProcAddress(handle, "destroyPlugin"))) {
                            destroy_func(plugin);
                        }
                    }
                }
                FreeLibrary(handle);
            }
#else
            // Unix/Linux 系统使用 dlopen/dlsym
            void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
            if (handle) {
                CreatePluginFunc create_func = (CreatePluginFunc)dlsym(handle, "createPlugin");
                if (create_func && dlerror() == NULL) {  // 检查错误
                    IPlugin* plugin = create_func();
                    if (plugin) {
                        // 获取插件支持的命令别名
                        auto aliases = plugin->getCommandAliases();
                        for (const auto& alias : aliases) {
                            PluginLoader::command_to_plugin_map()[alias] = pair.first;
                        }

                        DestroyPluginFunc destroy_func = (DestroyPluginFunc)dlsym(handle, "destroyPlugin");
                        if (destroy_func && dlerror() == NULL) {
                            destroy_func(plugin);
                        }
                    }
                }
                dlclose(handle);
            }
#endif
        }
    }
}

bool PluginLoader::is_plugin_installed(const std::string& plugin_name) {
    auto it = PluginLoader::installed_plugins().find(plugin_name);
    return (it != PluginLoader::installed_plugins().end() && it->second);
}

bool PluginLoader::is_plugin_command(const std::string& command) {
    // 可以维护一个特殊命令列表，避免与插件名冲突
    static std::set<std::string> builtin_commands = {"help", "exit", "cd", "ls", "plugin"};
    return builtin_commands.find(command) != builtin_commands.end();
}

// 在 PluginLoader 类中添加声明
static std::string resolve_plugin_name(const std::string& plugin_name);

// 在 plugin_loader.cpp 中实现
std::string PluginLoader::resolve_plugin_name(const std::string& plugin_name) {
    // 如果已经包含扩展名，直接返回
    if (plugin_name.find(".dll") != std::string::npos ||
        plugin_name.find(".so") != std::string::npos) {
        return plugin_name;
    }

    // 尝试添加常见的扩展名
#ifdef _WIN32
    std::string full_name = plugin_name + ".dll";
#else
    std::string full_name = plugin_name + ".so";
#endif

    // 检查文件是否存在
    std::string plugin_path = home_dir + "/duckshell/plugins/" + full_name;
    struct stat buffer;
    if (stat(plugin_path.c_str(), &buffer) == 0) {
        return full_name;
    }

    // 如果找不到，返回原始名称
    return plugin_name;
}

std::vector<std::string> PluginLoader::get_directory_contents(const std::string& path) {
#ifdef _WIN32
    std::vector<std::string> contents;

    // 确保路径格式正确
    std::string search_path = path;
    if (search_path.back() != '\\' && search_path.back() != '/') {
        search_path += "\\";
    }
    search_path += "*";

    WIN32_FIND_DATAA find_data;
    HANDLE h_find = FindFirstFileA(search_path.c_str(), &find_data);

    if (h_find != INVALID_HANDLE_VALUE) {
        do {
            std::string name(find_data.cFileName);
            if (name != "." && name != "..") {
                contents.push_back(name);
            }
        } while (FindNextFileA(h_find, &find_data));
        FindClose(h_find);
    }

    return contents;
#else
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
#endif
}