#include <algorithm>
#include <iomanip>
#include <cctype>
#include <deque>
#include <vector>
#include <sstream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#endif

#include <cstdint>
#include <regex>

#include "../header.h"
#include "../plugins/plugins_manager.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

// 声明全局变量用于命令历史记录
static std::deque<std::string> command_history;
static size_t history_index = 0;
static std::string current_buffer;
static size_t cursor_pos = 0;  // 光标在缓冲区中的位置

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

// 交互式逐字符读取一行，支持在未回车时按下 Ctrl+L 清屏
static std::string read_line_interactive(const std::string& prompt_shown) {
    current_buffer.clear();
    cursor_pos = 0;

#ifdef _WIN32
    // Windows 实现
    for (;;) {
        int ch = _getch();

        // 特殊键处理 (功能键)
        if (ch == 0 || ch == 0xE0) {
            int ch2 = _getch(); // 获取第二个字节

            if (ch2 == 72) { // 上箭头键
                if (!command_history.empty() && history_index > 0) {
                    // 清除当前行
                    for (size_t i = 0; i < current_buffer.length(); ++i) {
                        std::cout << "\b \b";
                    }

                    // 加载历史命令
                    history_index--;
                    current_buffer = command_history[history_index];
                    cursor_pos = current_buffer.length();
                    std::cout << current_buffer;
                    std::cout.flush();
                }
                continue;
            }
            else if (ch2 == 80) { // 下箭头键
                if (!command_history.empty() && history_index < command_history.size()) {
                    // 清除当前行
                    for (size_t i = 0; i < current_buffer.length(); ++i) {
                        std::cout << "\b \b";
                    }

                    history_index++;
                    if (history_index < command_history.size()) {
                        current_buffer = command_history[history_index];
                    } else {
                        current_buffer.clear();
                    }
                    cursor_pos = current_buffer.length();
                    std::cout << current_buffer;
                    std::cout.flush();
                }
                continue;
            }
            else if (ch2 == 75) { // 左箭头键
                if (cursor_pos > 0) {
                    cursor_pos--;
                    std::cout << "\b";
                    std::cout.flush();
                }
                continue;
            }
            else if (ch2 == 77) { // 右箭头键
                if (cursor_pos < current_buffer.length()) {
                    cursor_pos++;
                    std::cout << current_buffer[cursor_pos-1];
                    std::cout.flush();
                }
                continue;
            }
        }

        // 回车 (CR)
        if (ch == '\r') {
            std::cout << "\n";
            break;
        }

        // 处理退格键
        if (ch == 8 /* BS */ || ch == 127) {
            if (!current_buffer.empty() && cursor_pos > 0) {
                current_buffer.erase(cursor_pos - 1, 1);
                cursor_pos--;

                // 重绘行：先清除整行，然后重新绘制
                std::cout << "\r" << prompt_shown << current_buffer << " "; // 添加一个空格清除最后的字符
                // 计算需要回退的距离
                size_t back_steps = current_buffer.length() - cursor_pos + 1; // +1 是为了清除添加的空格
                for (size_t i = 0; i < back_steps; ++i) {
                    std::cout << "\b";
                }
                std::cout.flush();
            }
            continue;
        }

        // 处理 Ctrl+L (FF, 0x0C)
        if (ch == 12) {
            system("cls");
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
            continue;
        }

        // 忽略不可打印控制字符
        if (ch >= 32 && ch != 127) {
            current_buffer.insert(cursor_pos, 1, static_cast<char>(ch));
            cursor_pos++;

            // 重绘行
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
        }
    }
#else
    // POSIX 版本
    termios oldt{};
    termios newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON); // 保留 ECHO 由我们自己输出
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    for (;;) {
        unsigned char ch = 0;
        ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n <= 0) {
            continue;
        }

        // ANSI 转义序列处理 (方向键)
        if (ch == 27) { // ESC
            unsigned char ch2 = 0;
            n = ::read(STDIN_FILENO, &ch2, 1);
            if (n > 0 && ch2 == '[') {
                unsigned char ch3 = 0;
                n = ::read(STDIN_FILENO, &ch3, 1);
                if (n > 0) {
                    if (ch3 == 'A') { // 上箭头
                        if (!command_history.empty() && history_index > 0) {
                            // 清除当前行
                            for (size_t i = 0; i < current_buffer.length(); ++i) {
                                std::cout << "\b \b";
                            }

                            history_index--;
                            current_buffer = command_history[history_index];
                            cursor_pos = current_buffer.length();
                            std::cout << current_buffer;
                            std::cout.flush();
                        }
                        continue;
                    }
                    else if (ch3 == 'B') { // 下箭头
                        if (!command_history.empty() && history_index < command_history.size()) {
                            // 清除当前行
                            for (size_t i = 0; i < current_buffer.length(); ++i) {
                                std::cout << "\b \b";
                            }

                            history_index++;
                            if (history_index < command_history.size()) {
                                current_buffer = command_history[history_index];
                            } else {
                                current_buffer.clear();
                            }
                            cursor_pos = current_buffer.length();
                            std::cout << current_buffer;
                            std::cout.flush();
                        }
                        continue;
                    }
                    else if (ch3 == 'D') { // 左箭头
                        if (cursor_pos > 0) {
                            cursor_pos--;
                            std::cout << "\b";
                            std::cout.flush();
                        }
                        continue;
                    }
                    else if (ch3 == 'C') { // 右箭头
                        if (cursor_pos < current_buffer.length()) {
                            cursor_pos++;
                            std::cout << current_buffer[cursor_pos-1];
                            std::cout.flush();
                        }
                        continue;
                    }
                }
            }
        }

        if (ch == '\n' || ch == '\r') {
            std::cout << "\n";
            break;
        }

        if (ch == 8 || ch == 127) { // backspace/delete
            if (!current_buffer.empty() && cursor_pos > 0) {
                current_buffer.erase(cursor_pos - 1, 1);
                cursor_pos--;

                // 重绘行：先清除整行，然后重新绘制
                std::cout << "\r" << prompt_shown << current_buffer << " "; // 添加一个空格清除最后的字符
                // 计算需要回退的距离
                size_t back_steps = current_buffer.length() - cursor_pos + 1; // +1 是为了清除添加的空格
                for (size_t i = 0; i < back_steps; ++i) {
                    std::cout << "\b";
                }
                std::cout.flush();
            }
            continue;
        }

        if (ch == 12) { // Ctrl+L
#ifdef _WIN32
            if (system("cls") != 0) {
                // 忽略错误
            }
#else
            if (system("clear") != 0) {
                // 忽略错误
            }
#endif
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
            continue;
        }

        if (ch >= 32 && ch != 127) {
            current_buffer.insert(cursor_pos, 1, static_cast<char>(ch));
            cursor_pos++;

            // 重绘行
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return current_buffer;
}

int execute(const std::string& input) {
    // 添加命令到历史记录（除非是空命令或与上一条相同）
    if (!input.empty() && (command_history.empty() || command_history.back() != input)) {
        command_history.push_back(input);
        if (command_history.size() > 100) {
            command_history.pop_front();
        }
    }
    history_index = command_history.size();

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
                println("Current Repository: " << PluginManager::repository_url.c_str());
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

/**
 * The startup function for DuckShell includes no input parameter and yes parameters and returns an exit code.
 * @param param
 * @return Exit Code
 */
int startup(const std::string &param) {
    // 初始化插件系统
    PluginManager::loadPlugins();

    if (param.empty()) {
        std::string command;
        while (true) {
            const std::string prompt = std::string("DUCKSHELL { ") + dir_now + " }> ";
            print(prompt);
            std::cout.flush();
            command = read_line_interactive(prompt);

            if (command == "exit" || command == "quit") {
                break;
            }

            if (!command.empty()) {
                // TODO: 处理命令执行逻辑
                println(BOLD << YELLOW << "Executing: " << command << RESET);
                if (const int exit_code = execute(command); exit_code != 0) {
                    // TODO
                }
            }
        }
    }
    else {
        execute(param);
    }
    return 0;
}