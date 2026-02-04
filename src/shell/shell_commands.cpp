#include <algorithm>
#include <iomanip>
#include <cctype>
#include <deque>
#include <vector>
#include <sstream>
#include <cstdint>
#include <regex>
#include <cstring>
#include <cerrno>

#include "../header.h"
#include "../plugins/plugin_manager.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

// 跨平台执行外部程序
int execute_external_command(const std::vector<std::string>& args) {
    if (args.empty()) return 0;

#ifdef _WIN32
    // Windows 实现: 使用 CreateProcess
    std::string command_line;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string arg = args[i];
        // 如果参数包含空格且没被引号包裹，则包裹它
        if (arg.find(' ') != std::string::npos && arg.front() != '\"') {
            arg = "\"" + arg + "\"";
        }
        command_line += arg + (i == args.size() - 1 ? "" : " ");
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess 需要可修改的字符串缓冲
    std::vector<char> cmd_buf(command_line.begin(), command_line.end());
    cmd_buf.push_back('\0');

    if (!CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        return -1; // 创建进程失败
    }

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exit_code;
#else
    // Unix 实现: 使用 fork + execvp
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        std::vector<char*> c_args;
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        // 如果 execvp 返回，说明执行失败
        std::cerr << "DuckShell: failed to execute " << c_args[0] << ": " << strerror(errno) << std::endl;
        exit(127);
    } else if (pid > 0) {
        // 父进程
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return -1;
    } else {
        // fork 失败
        return -1;
    }
#endif
}

// 辅助函数：处理字符串变换逻辑，如 .replace()
std::string transform_string(std::string text) {
    // 处理变量替换 ${var} 或 ${var.replace("old", "new")}
    std::regex var_regex(R"(\$\{([^}]+)\})");
    std::smatch match;
    std::string result = text;

    while (std::regex_search(result, match, var_regex)) {
        std::string expression = match[1].str();
        std::string replacement;

        // 检查是否有 .replace() 调用
        size_t dot_pos = expression.find(".replace(");
        if (dot_pos != std::string::npos) {
            std::string var_name = expression.substr(0, dot_pos);
            std::string call = expression.substr(dot_pos + 9); // 去掉 ".replace("
            
            // 简单解析参数，例如 "old", "new")
            size_t comma_pos = call.find(',');
            size_t end_paren = call.find(')');
            
            if (comma_pos != std::string::npos && end_paren != std::string::npos) {
                std::string old_str = call.substr(0, comma_pos);
                std::string new_str = call.substr(comma_pos + 1, end_paren - comma_pos - 1);
                
                // 移除可能的引号
                auto clean = [](std::string s) {
                    s.erase(std::remove(s.begin(), s.end(), '\"'), s.end());
                    s.erase(std::remove(s.begin(), s.end(), '\''), s.end());
                    // 移除首尾空白
                    size_t f = s.find_first_not_of(" ");
                    size_t l = s.find_last_not_of(" ");
                    if (f != std::string::npos && l != std::string::npos) return s.substr(f, l - f + 1);
                    return s;
                };
                
                old_str = clean(old_str);
                new_str = clean(new_str);
                
                std::string base_val = shell_global_vars.count(var_name) ? shell_global_vars[var_name] : "";
                
                // 执行替换
                if (!old_str.empty()) {
                    size_t start_pos = 0;
                    while((start_pos = base_val.find(old_str, start_pos)) != std::string::npos) {
                        base_val.replace(start_pos, old_str.length(), new_str);
                        start_pos += new_str.length();
                    }
                }
                replacement = base_val;
            }
        } else {
            // 普通变量替换
            replacement = shell_global_vars.count(expression) ? shell_global_vars[expression] : "";
        }

        result.replace(match.position(0), match.length(0), replacement);
    }

    return result;
}

int execute_command(const std::string& input) {
    // 移除首尾空白符及不可见的 \r 等
    std::string trimmed = input;
    size_t start = trimmed.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return 0;
    size_t end = trimmed.find_last_not_of(" \t\n\r");
    trimmed = trimmed.substr(start, end - start + 1);

    // 应用全局变量替换和字符串转换逻辑 (如 .replace())
    std::string transformed = transform_string(trimmed);
    
    if (const std::vector<std::string> cmd_inner = split(transformed, ' '); cmd_inner.empty()) return 1;
    const std::vector<std::string> cmd = split(transformed, ' ');

    // 恢复用户要求的 Executing 消息
    println("Executing: " << transformed);
    std::cout.flush(); // 立即刷新，确保在命令执行前显示

    if (cmd[0] == "cls" || cmd[0] == "clear") {
#ifdef _WIN32
        // 对于 cls, cmd.exe 是必需的，因为它是一个 cmd 内置命令
        execute_external_command({"cmd", "/c", "cls"});
#else
        execute_external_command({"clear"});
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
        return 0;
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
            return 1;
        }
        else {
            println(YELLOW << "Feature not fully implemented yet." << RESET);
            return 0;
        }
    }

    // 经典echo命令
    else if (cmd[0] == "echo" || cmd[0] == "print") {
        if (cmd.size() < 2) {
            println("");
        }
        else {
            // 由于现在 transformed 已经包含了所有的变量替换和 replace 逻辑，
            // 且 cmd 是基于 transformed 分割的，我们可以直接组合 cmd[1...]
            std::string content;
            for (size_t i = 1; i < cmd.size(); ++i) {
                content += cmd[i] + (i == cmd.size() - 1 ? "" : " ");
            }
            
            // 移除最外层的引号（如果有）
            if (content.length() >= 2 && 
               ((content.front() == '\"' && content.back() == '\"') || 
                (content.front() == '\'' && content.back() == '\''))) {
                content = content.substr(1, content.length() - 2);
            }
            
            println(content);
        }
    }

    // set命令，用于设置变量
    else if (cmd[0] == "set" || cmd[0] == "var") {
        if (cmd.size() < 2) {
            println(RED << BOLD << "Missing arguments. Usage: set key=value" << RESET);
        }
        else {
            size_t pos = cmd[1].find('=');
            if (pos != std::string::npos) {
                std::string key = cmd[1].substr(0, pos);
                std::string value = cmd[1].substr(pos + 1);
                shell_global_vars[key] = value;
                // println(GREEN << "Variable set: " << key << " = " << value << RESET);
            } else {
                println(RED << BOLD << "Invalid format. Usage: set key=value" << RESET);
            }
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
        // 确保在执行系统命令前刷新输出，避免乱序
        std::cout.flush();

        // 使用自定义的执行函数代替 system()
        // system() 会调用 cmd.exe /c，而 execute_external_command 直接启动进程
        int result = execute_external_command(cmd);

        // 确保子进程的所有输出都已经打印出来
        std::cout.flush();
        std::cerr.flush();

        if (result != 0) {
            // 如果返回 -1，说明进程创建失败（找不到文件等）
            // 如果返回 1 (Windows) 或 127 (Unix)，通常也表示命令未找到
#ifdef _WIN32
            if (result == -1 || result == 1) {
                println(RED << BOLD << "DuckShell: COMMAND NOT FOUND! Please specify another command." << RESET);
            }
#else
            if (result == -1 || result == 127) {
                println(RED << BOLD << "DuckShell: COMMAND NOT FOUND! Please specify another command." << RESET);
            }
#endif
            return 127;
        }
    }
    return 0;
}