#include "plugin_repository.h"
#include "plugin_loader.h"
#include "plugin_installer.h"
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
#else
#include <dlfcn.h>      // Unix 动态库加载
#endif

// 条件包含curl头文件，确保在没有curl开发包时也能编译
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#else
// 如果没有curl支持，提供空的函数实现
typedef void CURL;
typedef enum {
    CURLE_OK = 0,
} CURLcode;

static inline CURL* curl_easy_init() { return nullptr; }
static inline void curl_easy_cleanup(CURL* curl) {}
static inline CURLcode curl_easy_perform(CURL* curl) { return CURLE_OK; }
static inline CURLcode curl_easy_setopt(CURL* curl, int option, ...) { return CURLE_OK; }
static inline CURLcode curl_easy_getinfo(CURL* curl, int info, ...) { return CURLE_OK; }
static inline const char* curl_easy_strerror(CURLcode code) { return "CURL not available"; }

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

namespace {
    size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
        return fwrite(ptr, size, nmemb, stream);
    }
}

// 新增功能：设置仓库URL
void PluginRepository::set_repository_url(const std::string& url) {
    PluginLoader::repository_url() = url;
    println("Repository URL set to: " << url.c_str());
}

// 新增功能：列出远程插件
void PluginRepository::list_remote_plugins() {
    if (PluginLoader::repository_url().empty()) return;

    // 访问我们设计的 PyPI 风格索引页
    std::string index_url = PluginLoader::repository_url() + "/metadata/index.html";
    std::string temp_html = home_dir + "/duckshell/temp_index.html";

    if (internal_download(index_url, temp_html)) {
        std::ifstream file(temp_html);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        remove(temp_html.c_str());

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
void PluginRepository::download_plugin(const std::string& plugin_name) {
    if (PluginLoader::repository_url().empty()) return;

    // 1. 准备元数据路径
    std::string base_url = PluginLoader::repository_url() + "/packages/" + plugin_name;
    std::string meta_url = base_url + "/plugin.json";
    std::string temp_json = home_dir + "/duckshell/temp_meta.json";

    println("Fetching metadata: " << meta_url.c_str());

    // 2. 下载并解析 JSON
    if (!internal_download(meta_url, temp_json)) {
        println(RED << "Error: Could not reach repository or plugin not found." << RESET);
        return;
    }

    try {
        std::ifstream f(temp_json);
        json j = json::parse(f);
        f.close();
        // remove(temp_json.c_str()); // 调试阶段可以先不删

        // 3. 确定平台后缀
#ifdef _WIN32
        std::string current_platform = "windows";
        std::string ext = ".dll";
#else
        std::string current_platform = "linux";
        std::string ext = ".so";
#endif

        if (!j["platforms"].contains(current_platform) || j["platforms"][current_platform] == false) {
            println(RED << "Error: Plugin does not support " << current_platform << RESET);
            return;
        }

        // 4. 版本号与路径处理 (包含 'v' 前缀容错)
        std::string version = j["latest_version"];
        std::string version_path = version;
        if (!version_path.empty() && version_path[0] != 'v' && version_path[0] != 'V') {
            version_path = "v" + version;
        }

        // 5. 执行二进制下载
        // 结构: .../packages/PluginName/v1.0/PluginName.dll
        std::string binary_url = base_url + "/" + version_path + "/" + plugin_name + ext;
        std::string local_path = home_dir + "/duckshell/plugins/" + plugin_name + ext;

        println("Found version " << version << ". Downloading binary...");

        if (internal_download(binary_url, local_path)) {
            println(GREEN << "Successfully installed: " << plugin_name << RESET);
            PluginInstaller::install_all_plugins(); // 刷新内存中的插件映射
        }
        else {
            println(RED << "Error: Failed to download binary file." << RESET);
        }

    }
    catch (const std::exception& e) {
        println(RED << "JSON/Logic Error: " << e.what() << RESET);
    }
}

bool PluginRepository::internal_download(const std::string& url, const std::string& local_path) {
    println("Attempting to download: " << url);
    println("Saving to: " << local_path);
    
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

    FILE* fp = fopen(local_path.c_str(), "wb");
    if (!fp) {
        println(RED << "File Error: Cannot create " << local_path << RESET);
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
        std::remove(local_path.c_str()); // 失败了就清理掉空文件
        return false;
    }

    if (response_code != 200) {
        println(YELLOW << "HTTP Error: " << response_code << " (URL: " << url << ")" << RESET);
        std::remove(local_path.c_str());
        return false;
    }

    println(GREEN << "Download completed successfully!" << RESET);
    return true;
#endif // HAVE_LIBCURL
}