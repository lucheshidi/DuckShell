#include <algorithm>
#include <iomanip>
#include <cctype>
#include <deque>
#include <vector>
#include <sstream>
#include <cstdint>
#include <regex>

#include "../header.h"
#include "../plugins/plugin_manager.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

// 字符串分割辅助函数
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

int execute_command(const std::string& input) {
    if (const std::vector<std::string> cmd = split(input, ' '); cmd.empty()) return 1;

    else if (cmd[0] == "cls" || cmd[0] == "clear") {
#ifdef _WIN32
        if (system("cls") != 0) {
            // 忽略错误
        }
#else
        if (system("clear") != 0) {
            // 忽略错误
        }
#endif
    }

    else if (cmd[0] == "cd") {
        if (cmd.size() > 3) {
            println(RED << BOLD << "Too many arguments." << RESET);
            return 1;
        }

        if (cmd.size() == 1) {
            // cd without arguments
#ifdef _WIN32
            char* env = getenv("USERPROFILE");
            dir_now = env ? std::string(env) : "C:\\"; 
#else
            char* env = getenv("HOME");
            dir_now = env ? std::string(env) : "/";
#endif
            return 0;
        }

        std::string new_path;
        std::string path_separator;
#ifdef _WIN32
        path_separator = "\\";
        // Check if it's an absolute path (e.g., "C:\", "D:\")
        if (cmd[1].length() >= 2 && cmd[1][1] == ':') {
            new_path = cmd[1];
        }
#else
        path_separator = "/";
        if (cmd[1][0] == '/') {
            // Absolute path in Unix
            new_path = cmd[1];
        }
#endif
        else {
            // Relative path
            if (cmd[1] == "..") {
                size_t last_sep = dir_now.find_last_of("/\\");
                if (last_sep != std::string::npos && last_sep > 0) {
                    new_path = dir_now.substr(0, last_sep);
                    // Keep drive letter for Windows
#ifdef _WIN32
                    if (new_path.length() == 2 && new_path[1] == ':') {
                        new_path += path_separator;
                    }
#endif
                } else {
#ifdef _WIN32
                    new_path = "C:";
#else
                    new_path = "/";
#endif
                }
            }
            else if (cmd[1] == ".") {
                new_path = dir_now;
            }
            else {
                // Handle path concatenation
                if (dir_now.back() == '/' || dir_now.back() == '\\') {
                    new_path = dir_now + cmd[1];
                }
                else {
                    new_path = dir_now + path_separator + cmd[1];
                }
            }
        }

        // Normalize path separators for the current platform
        std::replace(new_path.begin(), new_path.end(), '/', path_separator[0]);
        std::replace(new_path.begin(), new_path.end(), '\\', path_separator[0]);

        // Canonicalize path to remove "." and ".." segments and produce a full path style
        auto normalize_path = [&](const std::string &p) -> std::string {
            if (p.empty()) return p;

            const char sep = path_separator[0];
            std::vector<std::string> segments;

            // Detect Windows drive like "C:" and whether rooted (e.g., C:\)
            std::string drivePrefix;
            bool rooted = false;

#ifdef _WIN32
            if (p.size() >= 2 && p[1] == ':') {
                drivePrefix = p.substr(0, 2); // e.g., "C:"
                // Check if after drive there is a separator -> rooted at drive
                if (p.size() >= 3 && (p[2] == '\\' || p[2] == '/')) {
                    rooted = true;
                }
            } else if (!p.empty() && (p[0] == '\\' || p[0] == '/')) {
                rooted = true;
            }
#else
            if (!p.empty() && p[0] == '/') {
                rooted = true;
            }
#endif

            // Split by both separators
            std::string token;
            auto flush_token = [&]() {
                if (!token.empty()) {
                    if (token == ".") {
                        // skip
                    }
                    else if (token == "..") {
                        if (!segments.empty()) {
                            segments.pop_back();
                        } else {
                            // At root: keep as root, ignore further ".."
                        }
                    }
                    else {
                        segments.emplace_back(token);
                    }
                    token.clear();
                }
            };

            for (size_t i = 0; i < p.size(); ++i) {
                char c = p[i];
                if (c == '/' || c == '\\') {
                    flush_token();
                }
                else {
                    // Skip drive letters already captured
#ifdef _WIN32
                    if (i < 2 && p.size() >= 2 && p[1] == ':') {
                        // part of drive, skip storing
                        continue;
                    }
                    if (i == 2 && p.size() >= 3 && (p[2] == '\\' || p[2] == '/')) {
                        // skip the separator after drive when rooted
                        continue;
                    }
#endif
                    token.push_back(c);
                }
            }
            flush_token();

            // Rebuild
            std::string result;
#ifdef _WIN32
            result += drivePrefix;
#endif
            if (rooted) result.push_back(sep);
            for (size_t i = 0; i < segments.size(); ++i) {
                if (!(result.empty() || result.back() == sep)) {
                    result.push_back(sep);
                }
                result += segments[i];
            }

#ifdef _WIN32
            // Ensure a root like "C:" becomes "C:\" to match previous behavior
            if (!drivePrefix.empty() && rooted && segments.empty()) {
                if (result.size() == 2) result.push_back(sep);
            }
#endif
            return result;
        };

        new_path = normalize_path(new_path);

            if (is_directory_exists(new_path)) {
#ifdef _WIN32
                // On Windows, correct the case of each path component and uppercase drive letter
                // using WinAPI. This ensures paths like "c:\windows" or "C:\WINDOWS" display as
                // "C:\Windows" consistently.
                #ifndef MAX_PATH
                #define MAX_PATH 260
                #endif
                char fullBuf[MAX_PATH];
                DWORD flen = GetFullPathNameA(new_path.c_str(), MAX_PATH, fullBuf, nullptr);
                std::string full = (flen > 0 && flen < MAX_PATH) ? std::string(fullBuf, flen) : new_path;

                char longBuf[MAX_PATH];
                DWORD llen = GetLongPathNameA(full.c_str(), longBuf, MAX_PATH);
                std::string cased = (llen > 0 && llen < MAX_PATH) ? std::string(longBuf, llen) : full;

                if (!cased.empty() && std::isalpha(static_cast<unsigned char>(cased[0]))) {
                    cased[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(cased[0])));
                }
                dir_now = cased;
#else
                dir_now = new_path;
#endif
            }
            else {
                println(RED << BOLD << "Directory does not exist." << RESET);
            }
        }

    else if (cmd[0] == "ls" || cmd[0] == "dir" || cmd[0] == "ListFiles") {
#ifdef _WIN32
        try {
            std::string searchPath = dir_now + "\\*";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::string name(findData.cFileName);
                    std::string type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>   " : "<FILE>  ";
                    std::cout << std::left << std::setw(6) << type
                             << " " << name << std::endl;
                }
                while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        }
        catch (...) {
            std::cerr << RED << BOLD << "Error accessing directory." << RESET << std::endl;
        }
#else
        // Linux/Unix 实现
        DIR* dir = opendir(dir_now.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name(entry->d_name);
                if (name != "." && name != "..") {
                    struct stat statbuf;
                    std::string fullPath = dir_now + "/" + name;
                    if (stat(fullPath.c_str(), &statbuf) == 0) {
                        std::string type = S_ISDIR(statbuf.st_mode) ? "<DIR>   " : "<FILE>  ";
                        std::cout << std::left << std::setw(6) << type
                                 << " " << name << std::endl;
                    }
                }
            }
            closedir(dir);
        }
#endif
    }

    // 在 execute 函数中添加插件管理命令处理
    else if (cmd[0] == "plugin" || cmd[0] == "plugins") {
        if (cmd.size() < 2) {
            printf("Usage: plugin <command> [args...]\n");
            printf("Commands: install-all, list, run, install, uninstall, remove, enable, disable, available, download, repo\n");
            return 1;
        }

        if (cmd[1] == "run") {
            if (cmd.size() < 3) {
                printf("Usage: plugin run <plugin_name> [args...]\n");
                return 1;
            }

            std::string pluginName = cmd[2];
            std::vector<std::string> pluginArgs(cmd.begin() + 3, cmd.end());
            PluginManager::executePluginWithCommand(pluginName, pluginArgs);
        }
        else if (cmd[1] == "install-all") {
            PluginManager::installAllPlugins();
        }
        else if (cmd[1] == "list") {
            PluginManager::listInstalledPlugins();
        }
        else if (cmd[1] == "install") {
            if (cmd.size() < 3) {
                println("Usage: plugin install <plugin_name>");
                return 1;
            }
            PluginManager::installPlugin(cmd[2]);
        }
        else if (cmd[1] == "uninstall") {
            if (cmd.size() < 3) {
                println("Usage: plugin uninstall <plugin_name>");
                return 1;
            }
            PluginManager::uninstallPlugin(cmd[2]);
        }
        else if (cmd[1] == "remove") {
            if (cmd.size() < 3) {
                println("Usage: plugin remove <plugin_name>");
                return 1;
            }
            PluginManager::removePlugin(cmd[2]);
        }
        else if (cmd[1] == "enable") {
            if (cmd.size() < 3) {
                println("Usage: plugin enable <plugin_name>");
                return 1;
            }
            PluginManager::enablePlugin(cmd[2]);
        }
        else if (cmd[1] == "disable") {
            if (cmd.size() < 3) {
                println("Usage: plugin disable <plugin_name>");
                return 1;
            }
            PluginManager::disablePlugin(cmd[2]);
        }
        else if (cmd[1] == "available") {
            if (cmd.size() > 2) {
                println("Usage: plugin available");
                return 1;
            }
            PluginManager::listAvailablePlugins();
        }
        else if (cmd[1] == "download") {
            if (cmd.size() < 3) {
                println("Usage: plugin download <plugin_name>");
                return 1;
            }
            PluginManager::downloadPlugin(cmd[2]);
        }
        else if (cmd[1] == "repo") {
            if (cmd.size() < 3) {
                println("Current Repository: " << PluginManager::repository_url().c_str());
            }
            else {
                PluginManager::setRepositoryUrl(cmd[2]);
            }
        }
        else {
            println("Unknown plugin command: " << cmd[1].c_str());
            println("Usage: plugin <command> [args...]\n");
            println("Commands: install-all, list, run, install, uninstall, remove, enable, disable, available, download, repo\n");
        }
        return 1;
    }

    // 文件系统操作
    // 删除物品
    else if (cmd[0] == "rmv" || cmd[0] == "rm" || cmd[0] == "RemoveItem" || cmd[0] == "del") {
        if (cmd.size() < 2) {
            println("Usage: { rm | rmv | RemoveItem | del } [options] <filename>\n");
            return 1;
        }

        std::string filepath;
        if (cmd[1][0] == '/' || (cmd[1].length() >= 2 && cmd[1][1] == ':')) {
            // Absolute path
            filepath = cmd[1];
        } else {
            // Relative path
            filepath = dir_now +
#ifdef _WIN32
                       "\\" +
#else
                       "/" +
#endif
                       cmd[1];
        }

#ifdef _WIN32
        if (DeleteFileA(filepath.c_str()) == 0) {
            println(RED << BOLD << "Failed to delete file: " << filepath << RESET);
        } else {
            println(GREEN << "File deleted successfully: " << filepath << RESET);
        }
#else
        if (unlink(filepath.c_str()) != 0) {
            println(RED << BOLD << "Failed to delete file: " << filepath << RESET);
        } else {
            println(GREEN << "File deleted successfully: " << filepath << RESET);
        }
#endif
    }

    // 新建物品
    else if (cmd[0] == "new" || cmd[0] == "crt" || cmd[0] == "mk") {
        if (cmd.size() < 2) {
            println("Usage: { new | crt | mk } [options] <name>\n"
                  "Options:\n"
                  "    -f      Create a file.\n"
                  "    -d      Create a directory.");
        }
        else {
            if (cmd[1] == "-f" || cmd[1] == "file" || cmd[1] == "File") {

            }
        }
    }

    // 经典echo命令
    else if (cmd[0] == "echo" || cmd[0] == "print") {
        if (cmd.size() < 2) {
            println("");
        }
        else {
            println(std::regex_replace(cmd[1], std::regex("\"", "\'"), ""));
        }
    }

    // set命令，用于设置变量
    else if (cmd[0] == "set" || cmd[0] == "var") {
        if (cmd.size() < 2) {
            println(RED << BOLD << "Missing arguments. Usage: set key=value" << RESET);
        }
        else {
            std::vector<std::string> var;
            var = split(cmd[1], '=');
        }
    }

    // 2. 【新增关键逻辑】检查是否是插件注册的自定义命令
    else if (PluginManager::isPluginCommand(cmd[0])) {
        // 提取参数 (去掉命令名本身)
        std::vector<std::string> args(cmd.begin() + 1, cmd.end());
        PluginManager::executeCommand(cmd[0], args);
        return 0;
    }

    // 如果是系统命令，尝试执行
    else {
        // 构造完整的命令路径
        const std::string& full_command = input;

#ifdef _WIN32
        // Windows系统命令执行
        int result = system(full_command.c_str());
        if (result != 0) {
            println(YELLOW << "DuckShell: " << BOLD << "COMMAND NOT FOUND! Please specify another command." << RESET);
            return 127;
        }
#else
        // Unix/Linux系统命令执行
        int result = system(full_command.c_str());
        if (result != 0) {
            println(YELLOW << "DuckShell: " << BOLD << "COMMAND NOT FOUND! Please specify another command." << RESET);
            return 127;
        }
#endif
    }
    return 0;
}